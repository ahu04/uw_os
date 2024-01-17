#! /bin/csh -f
set TEST_HOME = /home/cs537-1/tests/P3/P3_GradingTests
set source_file = wsh.c
set binary_file = wsh
set bin_dir = ${TEST_HOME}/bin
set test_dir = ${TEST_HOME}/tests/exec

env TEST_HOME=${TEST_HOME} PROMPT='wsh> ' ${bin_dir}/output-cmp-based-p3-tester.py -s $source_file -b $binary_file -t $test_dir $argv[*]
