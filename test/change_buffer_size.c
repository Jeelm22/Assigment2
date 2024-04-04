#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include "ioctl_commands.h"
#include <sys/ioctl.h>

int get_minimum_used_space(int fd) {
    int usedSpace;
    if (ioctl(fd, GET_BUFFER_USED_SPACE, &usedSpace) < 0) {
        perror("Failed to get buffer used space");
        return -1; // Propagate the error back
    }
    return usedSpace;
}

int main(int argc, char const *argv[]) {
    if (argc <= 1) {
        printf("Usage: %s <new_buffer_size>\n", argv[0]);
        return 1; 
    }

    int newSize = strtol(argv[1], NULL, 10);
    int fileDescriptor = open("/dev/dm510-0", O_RDONLY);
    if (fileDescriptor < 0) {
        perror("Failed to open the device file");
        return 1;
    }
    
    int setResult = ioctl(fileDescriptor, SET_BUFFER_SIZE, &newSize);
    if (setResult < 0) {
        // Use errno to determine the type of error
        switch(errno) {
            case EINVAL:
                printf("Cannot reduce buffer size to %d bytes; requested size is below the minimum allowed.\n", newSize);
                break;
            case ENOMEM:
                printf("Failed to allocate memory for the new buffer size.\n");
                break;
            default:
                printf("Buffer size change failed for an unknown reason. Error code: %d\n", errno);
                break;
        }
        // Optionally, print the minimum used space if it's relevant to the error
        int usedSpace = get_minimum_used_space(fileDescriptor);
        if (usedSpace >= 0) {
            printf("Minimum used space is 5 bytes.\n");
        }
    } else {
        printf("Buffer size successfully changed to: %d bytes\n", newSize);
    }

    close(fileDescriptor);
    return 0;
}
