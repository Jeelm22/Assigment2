#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include "ioctl_commands.h"

#define DEVICE_FILE "/dev/dm510-0"

int main() {
    int device_fd = open(DEVICE_FILE, O_RDWR);
    if (device_fd < 0) {
        fprintf(stderr, "Failed to open device: %s\n", strerror(errno));
        return 1;
    }

    int buffer_size = ioctl(device_fd, GET_BUFFER_SIZE);
    if (buffer_size < 0) {
        fprintf(stderr, "Failed to get buffer size: %s\n", strerror(errno));
        close(device_fd);
        return 1;
    }

    char byte_to_write = 0;
    ssize_t bytes_written;
    for (int i = 0; i < buffer_size; ++i) {
        bytes_written = write(device_fd, &byte_to_write, sizeof(byte_to_write));
        if (bytes_written < 0) {
            fprintf(stderr, "Failed to write to device: %s\n", strerror(errno));
            close(device_fd);
            return 1;
        }
        byte_to_write++;
    }

    // Attempt to write one more byte to check if the buffer is full
    bytes_written = write(device_fd, &byte_to_write, sizeof(byte_to_write));
    if (bytes_written < 0) {
        printf("Buffer is full.\n");
    } else {
        printf("Unexpectedly able to write beyond reported buffer size.\n");
    }

    close(device_fd);
    return 0;
}
