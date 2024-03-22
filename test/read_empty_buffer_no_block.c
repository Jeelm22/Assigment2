#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define DEVICE_FILE "/dev/dm510-0"
//Defined number of bytes to read
#define READ_SIZE 42

int main() {
    //Open the device with read-write acces, in non-blocking mode
    int fd = open(DEVICE_FILE, O_RDWR | O_NONBLOCK);
    //Allocate buffer for the data read from device, +1 for null termination
    char buffer[READ_SIZE + 1]; 

    //Check if the device file was successfully opened
    if (fd < 0) {
        fprintf(stderr, "Failed to open device: %s\n", strerror(errno));
        //Returnn 1 to indicate an error condition
        return 1;
    }
    //Read from the device file
    ssize_t bytes_read = read(fd, buffer, READ_SIZE);
    //Check if the read operation was successful
    if (bytes_read < 0) {
        //First cheeck if it failed due to the operation being non-blocking
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            printf("Non-blocking read returned immediately with no data.\n");     
        } else {
            //If it's for another reason print error message
            fprintf(stderr, "Read error: %s\n", strerror(errno));
        }
    } else {
        //If the read was a succes, null-terminate the string and print it
        buffer[bytes_read] = '\0'; // Ensure the string is null-terminated
        printf("Received: '%s'\n", buffer);
    }

    close(fd);
    return 0;
}
