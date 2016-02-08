#include <stdlib.h>
#include <stdio.h>

void main(void) {
  // Open the file virus.txt
  int fd = open("virus.txt", 0);
  // Allocate the read buffer
  char *buffer = (char*)malloc(sizeof(char) * 1024);
  // Read 1024 bytes. Assuming file is small. Can always wrap this in a while loop
  read(fd, buffer, 1024);
  // Close the file
  close(fd);
  // Print the file descriptor so that the user can track the file in the syslog
  printf("File descriptor was: %d\n", fd);
}
