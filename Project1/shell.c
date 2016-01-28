// For calloc()
#include <stdlib.h>
// For strlen() and strcpy()
#include <string.h>
// For standard IO
#include <stdio.h>
// For fork(), exec...(), chdir()
#include <sys/types.h>
#include <unistd.h>
// For gettimeofday()
#include <sys/time.h>
// For getrusage()
#include <sys/time.h>
#include <sys/resource.h>
// For errno codes
#include <errno.h>
// For mmap()
#include <sys/mman.h>

// Some constants
#define MAX_INPUT_BUFFER (1000)

// Some variables to be maintained across the fork
static char **argp; // The arguments to be sent to execvp
static int *badCommand; // 0 => NOT a bad command, 1 => It was a bad command

// The program
int main(const int argc, char **argv) {
  // We will be ignoring any arguments given.
  while( 1 ) {
    // Start the infinite loop of asking prompts from the user
    printf("==> ");
    // Read the input
    char *inputCommand = (char *)malloc( MAX_INPUT_BUFFER * sizeof(char) );
    fgets( inputCommand, MAX_INPUT_BUFFER, stdin ); // Get the input
    // Print the command once again if the command came through a file
    if( !isatty(fileno(stdin)) ) printf("%s", inputCommand);
    // Check for EOF 
    if( inputCommand[strlen(inputCommand) - 1] == '\0' ) {
      printf("exit\n");
      break; // Exit the loop and no longer prompt the user
    }
    // Check for an empty line
    if( inputCommand[0] == '\n' ) continue; // Force another iteration and ask the user 
    
    // Clear out the \n from the string
    int loopCounter = 0; // Loop counter
    for( ; loopCounter < MAX_INPUT_BUFFER ; loopCounter++ )
      if( inputCommand[loopCounter] == '\n' )
        inputCommand[loopCounter] = '\0';

    // Backup the string
    char *backupString = strdup(inputCommand);

    // Check if the input violated the conditions
    // CONDITION 1: Input cannot exceed 128 characters
    if( strlen(inputCommand) > 128 ) {
      printf("Shell Error: Cannot process commands exceeding 128 characters\n");
      continue; // Force another iteration to re-prompt the user
    }
    // CONDITION 2: Input cannot have more than 32 distinct arguments
    // STEP: Count the number of arguments passed to the command 
    int argsCount = 0; int stringCounter = 0;
    char *argsCountToken = strtok(inputCommand, " ");
    while( argsCountToken != NULL ) {
      argsCountToken = strtok(NULL, " "); // Grab the next token
      argsCount++;
    }
    // STEP: Now run the condition
    if( argsCount > 32 ) {
      printf("Shell Error: Cannot process command with more than 32 distinct arguments\n");
      continue; // Force another iteration to re-prompt the user
    }
    // Check if the input is any of the special commands
    // STEP: Check for exit
    if( (inputCommand[0] == 'e' && inputCommand[1] == 'x' && inputCommand[2] == 'i' &&
         inputCommand[3] == 't' && ( inputCommand[4] == '\0' || inputCommand[4] == ' ' ) ) ) {

      break; // Exit the loop and thus exit the function

    }
    // STEP: Check for cd
    if( (inputCommand[0] == 'c' && inputCommand[1] == 'd' && ( inputCommand[2] == ' '  ||
                                                               inputCommand[2] == '\0' ) ) ) {
      inputCommand = inputCommand + 3; // Skip the 'cd '
      if(chdir(inputCommand) == (-1)) {
        if( errno == ELOOP )
	  printf("Shell failed to change directory: Too many symbolic links were encountered in resolving path\n");
	else if( errno == ENAMETOOLONG )
	  printf("Shell failed to change directory: path to be resolved is too long\n");
	else if( errno == ENOENT )
	  printf("Shell failed to change directory: The path doesn't exist\n");
	else if( errno == ENOTDIR )
	  printf("Shell failed to change directory: Component of path is not a directory\n");
	else
	  printf("Shell failed to change directory: Unknown error\n");
	continue; // Force another iteration to re-prompt the user
      } else {
        printf("Changed directory to %s\n", inputCommand);
	continue; // Force another iteration to re-prompt the user
      }
    }
    // Create the argp array
    char **argp = (char **)calloc( (argsCount + 2), sizeof(char *) ); 
    argp = mmap(NULL, sizeof(**argp), PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	// We need to share argp across a forked process, so we are doing this
    // Populate argp values now
    loopCounter = 0; // Reset the loop counter
    inputCommand = backupString; // Copy the old string back
    char *stringToken = strtok(inputCommand, " ");
    while( stringToken != NULL ) {
      *(argp + loopCounter) = (char *)malloc(strlen(stringToken) * sizeof(char));
      strcpy(argp[loopCounter++], stringToken);
      stringToken = strtok(NULL, " "); // Grab the next token
    }
    *(argp + loopCounter) = NULL;
    // Bad Command information
    badCommand = mmap(NULL, sizeof(*badCommand), PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	// We need to share badCommand across a forked process, so we are doing this
    *badCommand = 0; // Defaulting to it was NOT a bad command
    // Before executing the child, grab the statistics of the children before something
    // is executed
    struct rusage beforeResourceUsage;
    getrusage(RUSAGE_CHILDREN, &beforeResourceUsage);
    // Try executing the command in the child process now
    int childProcessID = fork();
    if( !childProcessID ) {
      // In child, try executing the child code now
      if( execvp(*argp, argp) < 0 ) {
        printf("Unknown command - %s\n", *argp);
	*badCommand = 1; // It was a bad command and usage stats should NOT be printed
        exit(EXIT_FAILURE); // Exit with failure
      }
    } else {
      // Track the time BEFORE the process started
      struct timeval *beforeTime = (struct timeval*)malloc(sizeof(struct timeval*)); gettimeofday(beforeTime, NULL);
      // Continue waiting for the process now
      int childReturnStatus; // Will hold the return status of the child process
      waitpid(-1, &childReturnStatus, 0); // Wait for the child to end and store the return status
      // Done with the process, so now grab the time
      struct timeval *afterTime = (struct timeval*)malloc(sizeof(struct timeval*)); gettimeofday(afterTime, NULL);
      // Now check if we had a bad command. If we did, don't print stats
      if( *badCommand == 1 )
        continue; // Force another iteration, without printing out stats
      // We didn't have any problems with the program.
      // Continue collect stats
      struct rusage afterResourceUsage;
      getrusage(RUSAGE_CHILDREN, &afterResourceUsage);
      // Now calculate the fresh stats
      long wallClockTime = ( ( ( afterTime->tv_sec -  beforeTime->tv_sec ) * 1000 )
                           + ( (afterTime->tv_usec - beforeTime->tv_usec) / 1000 ) );
      long userTime = ( ( afterResourceUsage.ru_utime.tv_sec * 1000 +
                         afterResourceUsage.ru_utime.tv_usec / 1000 ) -
			 ( beforeResourceUsage.ru_utime.tv_sec * 1000 +
			  beforeResourceUsage.ru_utime.tv_usec / 1000) );
      long systemTime = ( ( afterResourceUsage.ru_stime.tv_sec * 1000 +
                           afterResourceUsage.ru_stime.tv_usec / 1000 ) -
			  ( beforeResourceUsage.ru_stime.tv_sec * 1000 +
			   beforeResourceUsage.ru_stime.tv_usec / 1000 ) );
      long involuntary = afterResourceUsage.ru_nivcsw - beforeResourceUsage.ru_nivcsw;
      long voluntary = afterResourceUsage.ru_nvcsw - beforeResourceUsage.ru_nvcsw;
      long pageFault = afterResourceUsage.ru_majflt - beforeResourceUsage.ru_majflt;
      long unclaimedPages = afterResourceUsage.ru_minflt - beforeResourceUsage.ru_minflt;
      // Print out the stats
      printf("\n===== EXECUTION STATISTICS =====\n\n");
      printf("Elapsed \"wall-clock\" time for the command execution in milliseconds: %ld ms\n", wallClockTime);
      printf("Amount of CPU time used by the user in milliseconds: %ld ms\n", userTime);
      printf("Amount of CPU time used by the system in milliseconds: %ld ms\n", systemTime);
      printf("Total amount of CPU time used by the user and the system combined in milliseconds: %ld ms\n", userTime + systemTime);
      printf("Number of times the process was preeempted involuntarily: %ld\n", involuntary);
      printf("Number of times the process gave up the CPU voluntarily: %ld\n", voluntary);
      printf("Number of page faults: %ld\n", pageFault);
      printf("Number of page faults that could be satisfied by using unreclaimed pages: %ld\n", unclaimedPages);
      // Force another iteration now
      continue; // Will happen even if this line is commented out
    }
  }
}
