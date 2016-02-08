Coded by:
Akshit (Axe) Soota - asoota

=============================== PHASE 1 ==================================
Steps to compile and load:
cd project2/project2_phase1_lkm
sudo rmmod cs3013_project2
make clean
make
sudo insmod cs3013_project2.ko

You should immediately start logging in the syslog something like this:
Feb  5 23:14:16 axe-cs3013box kernel: [22036.034518] User 1000 is opening file: /etc/passwd
Feb  5 23:14:16 axe-cs3013box kernel: [22036.030382] User 1000 read from file descriptor 8 and the read was clean!
Feb  5 23:14:16 axe-cs3013box kernel: [22036.030385] User 1000 is closing file descriptor: 8

To test the calls specifically, run the following commands:
cd project2/project2_phase1_testcalls/
./readexample

To test syscall1 kernel call, run the following:
cd project2/project2_phase1_testcalls/
./test_syscall1

You should log something like this in syslog:
Feb  5 23:13:18 axe-cs3013box kernel: [21973.133729] "'Hello world?!' More like 'Goodbye, world!' EXTERMINATE!" -- Dalek

=============================== PHASE 2 ==================================
Steps to compile and load:
cd project2/project2_phase2_lkm
sudo rmmod cs3013_project2
make clean
make
sudo insmod cs3013_project2.ko

You should a program like:
sleep 600 &
gedit &
And use the PIDs for the below functions.

./shift2user <pid> <uid>
To test this:
cd project2/project2_phase2_testcalls/
./shift2user <pid> <uid>
Sample calls could look like:
axe@axe-cs3013box:~/project2/project2_phase2_testcalls$ sleep 600 &
[1] 6376
axe@axe-cs3013box:~/project2/project2_phase2_testcalls$ ./shift2user 6376 1001
Successfully changed the loginuid of process with PID 6376 to user ID 1001
axe@axe-cs3013box:~/project2/project2_phase2_testcalls$ ./shift2user 6376 1002
You are not authorized to make the necessary user shift changes to process with PID 6376
axe@axe-cs3013box:~/project2/project2_phase2_testcalls$ sudo ./shift2user 6376 1002
Successfully changed the loginuid of process with PID 6376 to user ID 1002

./getloginuid <pid>
To test this:
cd project2/project2_phase2_testcalls/
./getloginuid <pid>
Sample calls could look like:
axe@axe-cs3013box:~/project2/project2_phase2_testcalls$ ./getloginuid 6376
loginuid of process with process ID 6376 is 1002
axe@axe-cs3013box:~/project2/project2_phase2_testcalls$ ./getloginuid 9999
No process with process ID 9999 was found
