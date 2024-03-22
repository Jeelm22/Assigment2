#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "dm510_ioctl_commands.h"

int main(int argc, char const *argv[]) {
    //Open the device file for read-write access, in non-blocking mode
    int write_pointer = open("/dev/dm510-0", O_RDWR | O_NONBLOCK);
    //Check if the device file failed to open
    if (write_pointer < 0) {
        perror("Failed to open device");
        //Retrun 1 to indicate a failure
        return 1;
    }
    //Retrieve the current size of the device buffer with ioctl command
    const int size = ioctl(write_pointer, GET_BUFFER_SIZE);
    char n = 0;    //Initialize character to write to the device
    size_t i = 0;    //Counter for the loop

    //Loop to fill the device buffer
    while (i < size) {
        //Write character to device
        write(write_pointer, &n, sizeof(n));
        n++;    //Increment the character 
        i++;    //Increment the loop counter
    }
    //Check if the buffer is full, by writing once more
    if (write(write_pointer, &n, sizeof(n)) < 0) {
        //Print error messaeges if it failes
        printf("Buffer is full\n");
    }
    // Close the device file descriptor
    close(write_pointer); 
    return 0;
}
