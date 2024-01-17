import os
import filecmp
import subprocess
import re
import pickle
from multiprocessing import Pool, Lock
import signal
import datetime
import math

snapshots_dir = '/home/yurun/P3/GRADING/p3-grader/snapshots/P3'
result_dir = '/home/yurun/P3/GRADING/p3-grader/grading_result_int_makefile'
wsh_base_dir = '/home/yurun/P3/GRADING/p3-grader/wsh_dir'
grader_dir = '/home/yurun/P3/GRADING/p3-grader'
snapshot_dates = ['23-10-10-23-59', '23-10-11-23-59', '23-10-12-23-59', '23-10-13-23-59', '23-10-14-23-59', '23-10-15-23-59', '23-10-20-09-27']

full_points = 15
scaled_full_points = 4

def execute_bash_command(cs_login, cmd, timeout_length=180, do_binary=False, **kwargs):
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True, preexec_fn=os.setsid, **kwargs)
    time_out = False
    try:
        std_output, std_error = proc.communicate(timeout=timeout_length)
    except subprocess.TimeoutExpired:
        print(f'{cs_login} WARNING test timed out (i.e., it took too long)')
        # proc.kill()
        os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
        std_output, std_error = proc.communicate()
        time_out = True
    error_code = proc.returncode
    if not do_binary:
        # Python3 subprocess exec() sometimes returns bytes not a string -
        # make sure to turn it into ascii before treating as string
        output_actual = std_output.decode('utf-8', 'ignore').replace('…', '...')
        error_actual = std_error.decode('utf-8', 'ignore')
    else:
        output_actual = std_output
        error_actual = std_error
    rc_actual = int(error_code)
    return output_actual, error_actual, rc_actual, time_out

class TestResult:
    def __init__(self):
        self.result = None

class StudentResult:
    def __init__(self) -> None:
        self.handin = None
        self.results = None
        self.compile = None
        self.comment = None

class Handin:
    def __init__(self) -> None:
        self.first_handin = None
        self.changes = []
        self.final_snapshot = None
        self.tar_file_name = None
        self.acc_slipdays = None
        self.slipdays = None
        self.extension_days = None

class StudentResult:
    def __init__(self) -> None:

        # Handin | None
        self.handin = None

        # {test_name: [(bool, str), ...] | None}
        self.results = None

        # bool
        self.compile = None

        # str | None
        self.comment = None

def convert(test_result_dict):
    ret = {}
    for test_name, test_result in test_result_dict.items():
        if type(test_result) is list or test_result is None:
            ret[test_name] = test_result
        else:
            ret[test_name] = test_result.result
    return ret


cs_logins = [f for f in os.listdir('/home/yurun/P3/GRADING/p3-grader/integrated')]

def log(msg):
    current_datetime = datetime.datetime.now()
    with open('grade-makefile-log', '+a') as f:
        f.write(msg)
        f.write(f'\t{current_datetime.strftime("%Y-%m-%d %H:%M:%S")}')
        f.write('\n')
        f.flush()

for cs_login in cs_logins:
    makefile_path = None
    with open(f'/home/yurun/P3/GRADING/p3-grader/integrated/{cs_login}', 'rb') as f:
        grading_result = pickle.load(f)
    student_dir = f'{snapshots_dir}/{snapshot_dates[grading_result.handin.final_snapshot]}/{cs_login}'
    if grading_result.handin.tar_file_name.endswith('.c'):
        if os.path.exists(f'{student_dir}/Makefile'):
            makefile_path = f'{student_dir}/Makefile'
        elif os.path.exists(f'{student_dir}/makefile'):
            makefile_path = f'{student_dir}/makefile'
        else:
            log(f'[WARNING] {cs_login} Makefile not found')
    elif grading_result.handin.tar_file_name.endswith('.tar.gz'):
        wsh_dir = f'{wsh_base_dir}/0'
        execute_bash_command(cs_login, f'rm -rf {wsh_dir}/*')
        stdout, stderr, rc, time_out = execute_bash_command(cs_login, f'tar -xzf {student_dir}/{grading_result.handin.tar_file_name} -C {wsh_dir}')
        if rc != 0 or time_out:
            log(f'[ERROR] {cs_login} untar failed')
        else:
            if os.path.exists(f'{wsh_dir}/Makefile'):
                makefile_path = f'{wsh_dir}/Makefile'
            elif os.path.exists(f'{wsh_dir}/makefile'):
                makefile_path = f'{wsh_dir}/makefile'
            else:
                log(f'[WARNING] {cs_login} Makefile not found')
    else:
        log(f'[ERROR] {cs_login} handin file {grading_result.handin.tar_file_name}')

    makefile_result = None
    if makefile_path is not None:
        stdout, stderr, rc, time_out = execute_bash_command(cs_login, f'{grader_dir}/bin/makefile-grader.sh {makefile_path}')
        if rc != 0:
            log(f'[ERROR] {cs_login} makefile path: {makefile_path}. I thought I\'ve checked the existence of makefile? ')
        else:
            try:
                score = int(stdout.split()[0])
            except Exception as e:
                log(f'[ERROR] {cs_login} failed to parse makefile score: {stdout} Error: {e}')
                score = 0
            #scale
            score = math.ceil(score/full_points*scaled_full_points)
            makefile_result = [(score, '')]
            log(f'[NOTE] {cs_login} got {score} for makefile')
    grading_result.results['makefile'] = makefile_result
    with open(f'{result_dir}/{cs_login}', 'wb') as f:
        pickle.dump(grading_result, f)
