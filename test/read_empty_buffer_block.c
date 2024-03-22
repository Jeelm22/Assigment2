#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define DEVICE_FILE "/dev/dm510-0"
//Define number of bytes to read
#define READ_SIZE 42

int main() {
    //Open the device file for both reading and writing
    int fd = open(DEVICE_FILE, O_RDWR);
    //Allocate buffer to store the data read, +1 byte for null termination
    char buffer[READ_SIZE + 1]; // +1 for null termination

    //Check if opening the device file failed
    if (fd < 0) {
        fprintf(stderr, "Failed to open device: %s\n", strerror(errno));
        //Return 1 for failure
        return 1;
    }
    //Read up to READ_SIZE bytes from the device file into the buffer
    ssize_t bytes_read = read(fd, buffer, READ_SIZE);
    //Check if read operation failed
    if (bytes_read < 0) {
        //If it failed, print error message
        fprintf(stderr, "Read error: %s\n", strerror(errno));
    } else {
        //If successful, null-terminate the string read into the buffer
        buffer[bytes_read] = '\0'; // Ensure the string is null-terminated
        //Print data that was read from the device
        printf("Received: '%s'\n", buffer);
    }

    close(fd);
    return 0;
}

