// For calloc()
#include <stdlib.h>
// For standard IO
#include <stdio.h>
// For fork(), exec...()
#include <sys/types.h>
#include <unistd.h>
// For gettimeofday()
#include <sys/time.h>
// For getrusage()
#include <sys/time.h>
#include <sys/resource.h>
// For mmap()
#include <sys/mman.h>

// REFERENCE: http://stackoverflow.com/questions/13274786/how-to-share-memory-between-process-fork 
// Global variable to be maintained across the fork processes
static int *badCommand; // 0 => Not a bad command; 1 => It was a bad command

int main(const int argc, char **argv) {
  // Count the number of arguments we received
  if( argc < 2 ) {
    printf("No command given to be executed. Please try again.\n");
    exit(1); // Exit with failure
  }

  // Code to maintain data across the forked process
  badCommand = mmap(NULL, sizeof(*badCommand), PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  *badCommand = 0; // Defaulting to not a bad command
  // Start by forking the current process to yeild in a child
  // (that'll run the  required program) and the parent (that'll
  // wait for the child to end and then calculate the required
  // statistics).
  pid_t childProcess = fork(); // Attempt to create the child
  // Check if it was successful
  if( childProcess >= 0 ) {
    // Successful fork. Now this part of the code will re-run
    // If childProcess = 0 => It is the child process.
    // If childProcess not = 0 => It is the parent process.

    if( childProcess == 0 ) {
      // We're in the child process
      // STEP 1: Create an array with the parameters to call the program with
      char **argp = (char **)calloc(argc, sizeof(char *)); // Initialize the memory 
           // argc because we are excluding the running program name 
	   // AND we are ending the array of pointers with a NULL
      int loopCtr = 0; // Loop counter
      for( ; loopCtr < (argc - 1); loopCtr++ )
        *(argp + loopCtr) = *(argv + (loopCtr + 1));
      *(argp + loopCtr) = NULL; // Have to end the array with a NULL
      // STEP 2: Call the program with the required arguments
      if( execvp( *(argv + 1),   // Grab the thing to be executed
                    argp )       // Call the program with those parameters
	  < 0 )                  // If exec...() function returns a negative
			         // value, it failed to run
      {
        printf("Unknown command - %s\n", *(argv + 1));
	*badCommand = 1; // We encountered a bad command
	exit(1); // Exit with failure
      }
      // If we've reached here that means the command successfully executed
    } else {
      // We're in the parent process. Wait for the child to END
      // But now, the child process must've started running. So start by recording
      //  the time
      struct timeval *beforeTime = (struct timeval*)malloc(sizeof(struct timeval*)); gettimeofday(beforeTime, NULL);
      // Continue with the parent process
      int childReturnStatus; // Will hold the return status code for the child
                             // process
      // Keep waiting for the child process to end
      while( ( wait(&childReturnStatus) ) != childProcess ) ;
      // Once ended, collect the statistics
      // Collect time:
      struct timeval *afterTime = (struct timeval*)malloc(sizeof(struct timeval*)); gettimeofday(afterTime, NULL);
      // Check if we had a bad command, checking now so that we don't chew up wall-clock
      // time of the command executed
      if( *badCommand == 1 ) {
	munmap(badCommand, sizeof(*badCommand)); // Un-map it
        exit(childReturnStatus); // We had a bad command and now exit with the child's
                                 //  return status
      }
      // Collect resources for the child process:
      struct rusage *resourceUsageStats = (struct rusage*)malloc(sizeof(struct rusage*)); getrusage(RUSAGE_CHILDREN, resourceUsageStats);
      // Now calculate the time difference
      long millisecondsTimeDiff;
      millisecondsTimeDiff  = ( afterTime->tv_sec -  beforeTime->tv_sec) * 1000;
      millisecondsTimeDiff += (afterTime->tv_usec - beforeTime->tv_usec) / 1000;
      // Calculate the user and system time in milliseconds
      long userTimeMilliseconds =  resourceUsageStats->ru_utime.tv_sec * 1000 +
                                  resourceUsageStats->ru_utime.tv_usec / 1000;
      long systemTimeMilliseconds =  resourceUsageStats->ru_stime.tv_sec * 1000 + 
                                    resourceUsageStats->ru_stime.tv_usec / 1000;
      // Print the required statistics now
      printf("\n====== EXECUTION STATISTICS ======\n\n");
      printf("Elapsed \"wall-clock\" time for the command execution in milliseconds: %ld ms\n", millisecondsTimeDiff);
      printf("Amount of CPU time used by the user in milliseconds: %ld ms\n", userTimeMilliseconds);
      printf("Amount of CPU time used by the system in milliseconds: %ld ms\n", systemTimeMilliseconds);
      printf("Total amount of CPU time used by the user and system combined in milliseconds: %ld ms\n", userTimeMilliseconds +
                                                                                                       systemTimeMilliseconds);
      printf("Number of times the process was preempted involuntarily: %ld\n", resourceUsageStats->ru_nivcsw);
      printf("Number of times the process gave up the CPU voluntarily: %ld\n", resourceUsageStats->ru_nvcsw);
      printf("Number of page faults: %ld\n", resourceUsageStats->ru_majflt);
      printf("Number of page faults that could be satisfied by using unreclaimed pages: %ld\n", resourceUsageStats->ru_minflt);
      // De-allocate the memory shared
      munmap(badCommand, sizeof(*badCommand));
    }
  }
}
