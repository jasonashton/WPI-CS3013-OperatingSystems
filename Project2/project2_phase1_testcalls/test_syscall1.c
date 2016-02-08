#include <sys/syscall.h>
#include <stdio.h>

#define __NR_css3013_syscall1 355

// The main program starts here
int main(int argc, char **argv) {
  // Now attempt to call the system call and process the return information
  long returnValue = (long)syscall(__NR_css3013_syscall1);
  // Output the required information
  printf("\tKernel call to cs3013_syscall1() returned: %ld\n", returnValue);
  // Exit the function
  return 0;
}
