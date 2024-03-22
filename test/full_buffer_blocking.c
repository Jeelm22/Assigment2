#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include "ioctl_commands.h"

#define DEVICE_FILE "/dev/dm510-0"

int main() {
    //Open device file with read-write access
    int device_fd = open(DEVICE_FILE, O_RDWR);
    if (device_fd < 0) {
        fprintf(stderr, "Failed to open device: %s\n", strerror(errno));
        //Return 1, if opening fails
        return 1;
    }

    //Get the current buffer size of the device using an ioctl call
    int buffer_size = ioctl(device_fd, GET_BUFFER_SIZE);
    if (buffer_size < 0) {
        fprintf(stderr, "Failed to get buffer size: %s\n", strerror(errno));
        close(device_fd);
        //Return 1, if the ioctl call fails
        return 1;
    }

    //Initialize variable to keep track of the byte value to be written to the device
    char byte_to_write = 0;
    ssize_t bytes_written;
    //Loop through the number of time equel to the buffer size, write a byte each time
    for (int i = 0; i < buffer_size; ++i) {
        //Write one byte to the device
        bytes_written = write(device_fd, &byte_to_write, sizeof(byte_to_write));
        if (bytes_written < 0) {
            fprintf(stderr, "Failed to write to device: %s\n", strerror(errno));
            close(device_fd);
            //Reutrn 1, if writing fails
            return 1;
        }
        //Increment byte value to write for the next iteration
        byte_to_write++;
    }

    //Write one more byte to check if the buffer is full
    bytes_written = write(device_fd, &byte_to_write, sizeof(byte_to_write));
    if (bytes_written < 0) {
        //If it failes, print error message
        printf("Buffer is full.\n");
    } else {
        //If it succeeds, print messeage 
        printf("Unexpectedly able to write beyond reported buffer size.\n");
    }
    //Close device file to release system resources
    close(device_fd);
    return 0;
}
