Programmed by:
Akshit (Axe) Soota - asoota

1) runCommand
Using the simple fork() command to create a child from a parent so that the child executes the required process to be run and the parent can keep track.

SAMPLE TRACE:
axe@axe-cs3013box:~/Downloads$ ./runCommand
No command given to be executed. Please try again.
axe@axe-cs3013box:~/Downloads$ ./runCommand ls
makefile    README.txt~  runCommand.c  shell   shell2.c  shell.c  testfile_fordelay
README.txt  runCommand	 runCommand.o  shell2  shell2.o  shell.o  testfile_shell

====== EXECUTION STATISTICS ======

Elapsed "wall-clock" time for the command execution in milliseconds: 0 ms
Amount of CPU time used by the user in milliseconds: 0 ms
Amount of CPU time used by the system in milliseconds: 0 ms
Total amount of CPU time used by the user and system combined in milliseconds: 0 ms
Number of times the process was preempted involuntarily: 0
Number of times the process gave up the CPU voluntarily: 1
Number of page faults: 0
Number of page faults that could be satisfied by using unreclaimed pages: 288
axe@axe-cs3013box:~/Downloads$ ./runCommand ls -alh
total 268K
drwxr-xr-x  2 axe axe 4.0K Jan 26 21:30 .
drwx------ 19 axe axe 4.0K Jan 26 21:21 ..
-rwxrwxr-x  1 axe axe  436 Jan 26 21:19 makefile
-rw-rw-r--  1 axe axe   71 Jan 26 21:30 README.txt
-rw-rw-r--  1 axe axe    0 Jan 26 21:30 README.txt~
-rwxrwxr-x  1 axe axe  11K Jan 26 21:21 runCommand
-rwxrwxr-x  1 axe axe 5.7K Jan 26 21:18 runCommand.c
-rw-rw-r--  1 axe axe 6.8K Jan 26 21:21 runCommand.o
-rwxrwxr-x  1 axe axe  17K Jan 26 21:21 shell
-rwxrwxr-x  1 axe axe  22K Jan 26 21:21 shell2
-rwxrwxr-x  1 axe axe  17K Jan 26 21:20 shell2.c
-rw-rw-r--  1 axe axe  16K Jan 26 21:21 shell2.o
-rwxrwxr-x  1 axe axe 8.5K Jan 26 21:21 shell.c
-rw-rw-r--  1 axe axe  11K Jan 26 21:21 shell.o
-rwxrwxrwx  1 axe axe   35 Jan 26 14:09 testfile_fordelay
-rwxrwxr-x  1 axe axe   41 Jan 26 14:06 testfile_shell

====== EXECUTION STATISTICS ======

Elapsed "wall-clock" time for the command execution in milliseconds: 1 ms
Amount of CPU time used by the user in milliseconds: 0 ms
Amount of CPU time used by the system in milliseconds: 0 ms
Total amount of CPU time used by the user and system combined in milliseconds: 0 ms
Number of times the process was preempted involuntarily: 0
Number of times the process gave up the CPU voluntarily: 1
Number of page faults: 0
Number of page faults that could be satisfied by using unreclaimed pages: 410
axe@axe-cs3013box:~/Downloads$ ./runCommand doesntexist
Unknown command - doesntexist

2) shell
Created a basic shell that supports basic shell based commands like "exit" and "cd".
It prompts user for a command over and over till the user exits from the program. Using a similar idea of runCommand, we fork to create a child off the parent and wait till the child exits to track the child's usage stats.

SAMPLE TRACE:
axe@axe-cs3013box:~/Downloads$ ./shell
==> ./shell
==> ls 
makefile    README.txt~  runCommand.c  shell   shell2.c  shell.c  testfile_fordelay
README.txt  runCommand	 runCommand.o  shell2  shell2.o  shell.o  testfile_shell

===== EXECUTION STATISTICS =====

Elapsed "wall-clock" time for the command execution in milliseconds: 1 ms
Amount of CPU time used by the user in milliseconds: 0 ms
Amount of CPU time used by the system in milliseconds: 0 ms
Total amount of CPU time used by the user and the system combined in milliseconds: 0 ms
Number of times the process was preeempted involuntarily: 6
Number of times the process gave up the CPU voluntarily: 1
Number of page faults: 0
Number of page faults that could be satisfied by using unreclaimed pages: 264
==> ls -ali
total 268
912746 drwxr-xr-x  2 axe axe  4096 Jan 26 21:30 .
940543 drwx------ 19 axe axe  4096 Jan 26 21:21 ..
926330 -rwxrwxr-x  1 axe axe   436 Jan 26 21:19 makefile
926193 -rw-rw-r--  1 axe axe    71 Jan 26 21:30 README.txt
926181 -rw-rw-r--  1 axe axe     0 Jan 26 21:30 README.txt~
926194 -rwxrwxr-x  1 axe axe 10659 Jan 26 21:21 runCommand
926331 -rwxrwxr-x  1 axe axe  5790 Jan 26 21:18 runCommand.c
926184 -rw-rw-r--  1 axe axe  6900 Jan 26 21:21 runCommand.o
926196 -rwxrwxr-x  1 axe axe 16523 Jan 26 21:21 shell
926220 -rwxrwxr-x  1 axe axe 22040 Jan 26 21:21 shell2
926289 -rwxrwxr-x  1 axe axe 17184 Jan 26 21:20 shell2.c
926197 -rw-rw-r--  1 axe axe 15896 Jan 26 21:21 shell2.o
926288 -rwxrwxr-x  1 axe axe  8677 Jan 26 21:21 shell.c
926195 -rw-rw-r--  1 axe axe 10696 Jan 26 21:21 shell.o
926199 -rwxrwxrwx  1 axe axe    35 Jan 26 14:09 testfile_fordelay
926200 -rwxrwxr-x  1 axe axe    41 Jan 26 14:06 testfile_shell

===== EXECUTION STATISTICS =====

Elapsed "wall-clock" time for the command execution in milliseconds: 2 ms
Amount of CPU time used by the user in milliseconds: 0 ms
Amount of CPU time used by the system in milliseconds: 0 ms
Total amount of CPU time used by the user and the system combined in milliseconds: 0 ms
Number of times the process was preeempted involuntarily: 34
Number of times the process gave up the CPU voluntarily: 1
Number of page faults: 0
Number of page faults that could be satisfied by using unreclaimed pages: 386
==> pwd
/home/axe/Downloads

===== EXECUTION STATISTICS =====

Elapsed "wall-clock" time for the command execution in milliseconds: 0 ms
Amount of CPU time used by the user in milliseconds: 0 ms
Amount of CPU time used by the system in milliseconds: 0 ms
Total amount of CPU time used by the user and the system combined in milliseconds: 0 ms
Number of times the process was preeempted involuntarily: 0
Number of times the process gave up the CPU voluntarily: 1
Number of page faults: 0
Number of page faults that could be satisfied by using unreclaimed pages: 184
==> cat makefile
all: runCommand shell shell2

runCommand: runCommand.o
	gcc -g runCommand.o --std=gnu89 -o runCommand

runCommand.o: runCommand.c
	gcc -g --std=gnu89 -c runCommand.c

shell: shell.o
	gcc -g shell.o -o shell

shell.o: shell.c
	gcc -g --std=gnu89 -c shell.c

shell2: shell2.o
	gcc -g shell2.o -o shell2

shell2.o: shell2.c
	gcc -g --std=gnu89 -c shell2.c

clean:
	rm -f runCommand.o runCommand
	rm -f shell.o shell
	rm -f shell2.o shell2

===== EXECUTION STATISTICS =====

Elapsed "wall-clock" time for the command execution in milliseconds: 0 ms
Amount of CPU time used by the user in milliseconds: 0 ms
Amount of CPU time used by the system in milliseconds: 0 ms
Total amount of CPU time used by the user and the system combined in milliseconds: 0 ms
Number of times the process was preeempted involuntarily: 0
Number of times the process gave up the CPU voluntarily: 1
Number of page faults: 0
Number of page faults that could be satisfied by using unreclaimed pages: 187
==> doesn'texist
Unknown command - doesn'texist
==> exit

3) shell2
Similar to the shell implemented in the previous part but we've added background execution functionality which allows us to run multiple background processes. Using wait4, we take care of any background process that exits and we then print out the required statistics. We implemented a data structure to keep track of on-going and past, finished background processes requested by the user. In this data structure, I took note of the process ID, if the process had finished or not, the return status from the process, the arguments sent to call the command, the timeval struct recorded at the start of the execution of the command.

SAMPLE TRACE
axe@axe-cs3013box:~/Downloads$ ./shell2
==> ./testfile_fordelay 30 &
[1] 4243
==> ./testfile_fordelay 5 &
[2] 4245
==> ls &
[3] 4247
==> makefile    README.txt~  runCommand.c  shell   shell2.c  shell.c  testfile_fordelay
README.txt  runCommand	 runCommand.o  shell2  shell2.o  shell.o  testfile_shell

==> ls -alh
[3]: ls completed.
=====> START OF EXECUTION STATISTICS <=====
Elapsed "wall-clock" time for the command execution in milliseconds: 1952 ms
Amount of CPU time used by the user in milliseconds: 0 ms
Amount of CPU time used by the system in milliseconds: 0 ms
Total amount of CPU time used by the user and the system combined in milliseconds: 0 ms
Number of times the process was preeempted involuntarily: 0
Number of times the process gave up the CPU voluntarily: 1
Number of page faults: 0
Number of page faults that could be satisfied by using unreclaimed pages: 265
======> END OF EXECUTION STATISTICS <======

total 268K
drwxr-xr-x  2 axe axe 4.0K Jan 26 21:30 .
drwx------ 19 axe axe 4.0K Jan 26 21:21 ..
-rwxrwxr-x  1 axe axe  436 Jan 26 21:19 makefile
-rw-rw-r--  1 axe axe   71 Jan 26 21:30 README.txt
-rw-rw-r--  1 axe axe    0 Jan 26 21:30 README.txt~
-rwxrwxr-x  1 axe axe  11K Jan 26 21:21 runCommand
-rwxrwxr-x  1 axe axe 5.7K Jan 26 21:18 runCommand.c
-rw-rw-r--  1 axe axe 6.8K Jan 26 21:21 runCommand.o
-rwxrwxr-x  1 axe axe  17K Jan 26 21:21 shell
-rwxrwxr-x  1 axe axe  22K Jan 26 21:21 shell2
-rwxrwxr-x  1 axe axe  17K Jan 26 21:20 shell2.c
-rw-rw-r--  1 axe axe  16K Jan 26 21:21 shell2.o
-rwxrwxr-x  1 axe axe 8.5K Jan 26 21:21 shell.c
-rw-rw-r--  1 axe axe  11K Jan 26 21:21 shell.o
-rwxrwxrwx  1 axe axe   35 Jan 26 14:09 testfile_fordelay
-rwxrwxr-x  1 axe axe   41 Jan 26 14:06 testfile_shell

===== EXECUTION STATISTICS =====

Elapsed "wall-clock" time for the command execution in milliseconds: 1 ms
Amount of CPU time used by the user in milliseconds: 0 ms
Amount of CPU time used by the system in milliseconds: 0 ms
Total amount of CPU time used by the user and the system combined in milliseconds: 0 ms
Number of times the process was preeempted involuntarily: 0
Number of times the process gave up the CPU voluntarily: 1
Number of page faults: 0
Number of page faults that could be satisfied by using unreclaimed pages: 388
==> Hello after 5 sec

==> jobs
[2]: ./testfile_fordelay completed.
=====> START OF EXECUTION STATISTICS <=====
Elapsed "wall-clock" time for the command execution in milliseconds: 7608 ms
Amount of CPU time used by the user in milliseconds: 0 ms
Amount of CPU time used by the system in milliseconds: 0 ms
Total amount of CPU time used by the user and the system combined in milliseconds: 0 ms
Number of times the process was preeempted involuntarily: 0
Number of times the process gave up the CPU voluntarily: 4
Number of page faults: 0
Number of page faults that could be satisfied by using unreclaimed pages: 385
======> END OF EXECUTION STATISTICS <======

[1]: ./testfile_fordelay 30
[2]: ./testfile_fordelay completed.
[3]: ls completed.
==> exit
We've got running background processes. Type "jobs" to list all the running background processes.
==> 
