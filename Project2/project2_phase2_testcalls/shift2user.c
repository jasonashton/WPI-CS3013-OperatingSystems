#include <sys/syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define __NR_css3013_syscall2 356

// The main program starts here
int main(int argc, char **argv) {
  // Count the number of parameters sent
  if(argc != 3) {
    printf("Usage: %s target_pid target_uid\n", *argv);
    exit(1); // Exit with a failure
  }
  // We got the correct number of arguments to run the program
  // Create the variables to be sent to the syscall
  unsigned short *target_pid = (unsigned short*)malloc(sizeof(unsigned short));
  unsigned short *target_uid = (unsigned short*)malloc(sizeof(unsigned short));
  // Populate the values into these fields now
  *target_pid = atoi(argv[1]);
  *target_uid = atoi(argv[2]);
  // Now attempt to call the system call and process the return information
  long returnValue = (long)syscall(__NR_css3013_syscall2, target_pid, target_uid);
  // Process the return value now
  if(returnValue == 0)
    printf("Successfully changed the loginuid of process with PID %d to user ID %d\n", *target_pid, *target_uid);
  else if(returnValue == ESRCH)
    printf("No process with process ID %d was found\n", *target_pid);
  else if(returnValue == EPERM)
    printf("You are not authorized to make the necessary user shift changes to process with PID %d\n", *target_pid);
  else if(returnValue == EFAULT)
    printf("Failed to allocate memory in the kernel space during the kernel syscall. Please try again!\n");
  // Exit the function
  return 0;
}
