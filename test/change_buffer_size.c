#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include "ioctl_commands.h"
#include <sys/ioctl.h>

// Find the smallest used space in all buffers
int get_minimum_used_space(int fd) {
    int minSpace = ioctl(fd, GET_BUFFER_USED_SPACE, 0);
    for (int bufferIndex = 1; bufferIndex < BUFFER_COUNT; bufferIndex++) {
        int currentSpace = ioctl(fd, GET_BUFFER_USED_SPACE, bufferIndex);
        if (currentSpace < minSpace) {
            minSpace = currentSpace;
        }
    }
    return minSpace;
}

int main(int argc, char const *argv[]) {
    // Ensure there is at least one argument provided
    if (argc <= 1) {
        return 0; // Exit if no arguments
    }
    
    int newSize = strtol(argv[1], NULL, 10); // Convert the first argument to an integer
    int fileDescriptor = open("/dev/dm510-0", O_RDONLY); // Open the device file in read-only mode
    
    // Attempt to set the buffer size using ioctl
    int setResult = ioctl(fileDescriptor, SET_BUFFER_SIZE, newSize);
    
    // Check if the buffer size change was successful
    if (setResult < 0) {
        int usedSpace = get_minimum_used_space(fileDescriptor);
        printf("Cannot reduce buffer size to %d bytes; minimum used space is %d bytes.\n", newSize, usedSpace);
    } else {
        printf("Buffer size successfully changed to: %d bytes\n", newSize);
    }

    return 0;
}
