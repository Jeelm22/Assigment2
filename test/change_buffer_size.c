#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include "ioctl_commands.h"
#include <sys/ioctl.h>

//Find the smallest used space in all buffers
int get_minimum_used_space(int fd) {
    int usedSpace;
    // Pass the address of usedSpace to ioctl
    if (ioctl(fd, GET_BUFFER_USED_SPACE, &usedSpace) < 0) {
        perror("Failed to get buffer used space");
        return -1; // Return -1 to indicate error
    }
    return usedSpace;
}


int main(int argc, char const *argv[]) {
    //Ensure there is at least one argument provided
    if (argc <= 1) {
        //Exit if no arguments were provided
        return 0; 
    }

    //Get new size by convert the first argument to an integer
    int newSize = strtol(argv[1], NULL, 10); 
    
    // Open the device file in read-only mode
    int fileDescriptor = open("/dev/dm510-0", O_RDONLY); 
    
    // Attempt to set the buffer size using ioctl command
    int setResult = ioctl(fileDescriptor, SET_BUFFER_SIZE, newSize);
    
    // Check if the buffer size change was successful
	if (setResult < 0) {
    // If it failed, find the minimum used space in the buffers
	    int usedSpace = get_minimum_used_space(fileDescriptor);
	    if (usedSpace < 0) {
	        // Error handling was already done in get_minimum_used_space
	        return 1; // Exit with an error code
	    }
	    printf("Cannot reduce buffer size to %d bytes; minimum used space is %d bytes.\n", newSize, usedSpace);
	} else {
	    printf("Buffer size successfully changed to: %d bytes\n", newSize);
	}
    return 0;
}
