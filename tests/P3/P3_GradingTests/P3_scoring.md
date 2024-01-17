- path, fork/exec/wait -- ~ 44 pts
  exec: 10*4, path: 4
- cd, exit -- 6
  builtin: 2*3
- ctrl-c ctrl-z -- 6
  signal: 3*2
- batch mode -- 5
- Makefile -- 4
- pipe -- 15
  pipe: 5*3
- job control -- 20
  4+3*4+2*2
- extra credit -- pipes with job control zombie -- 10
  2*2+3*2