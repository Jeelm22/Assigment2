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
    //Initializes the minimum space to the used space of the first buffer
    int minSpace = ioctl(fd, GET_BUFFER_USED_SPACE, 0);
    //Iterate through all buffers to find the one with the min. used space
    for (int bufferIndex = 1; bufferIndex < BUFFER_COUNT; bufferIndex++) {
        //Get the used space for the current buffer
        int currentSpace = ioctl(fd, GET_BUFFER_USED_SPACE, bufferIndex);
        //Update minSpace if the current buffer has less used space
        if (currentSpace < minSpace) {
            minSpace = currentSpace;
        }
    }
    //Return the minimum used space found
    return minSpace;
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
        //If it failed, find the minum used space in the buffers
        int usedSpace = get_minimum_used_space(fileDescriptor);
        //Inform user if there is an error or a succuses  
        printf("Cannot reduce buffer size to %d bytes; minimum used space is %d bytes.\n", newSize, usedSpace);
    } else {
        printf("Buffer size successfully changed to: %d bytes\n", newSize);
    }

    return 0;
}
