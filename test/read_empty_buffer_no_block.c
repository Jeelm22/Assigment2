#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

//Defined number of bytes to read
#define READ_SIZE 42

int main() {
    //Open the device with read-write acces, in non-blocking mode
    int read_pointer = open("/dev/dm510-0", O_RDWR | O_NONBLOCK);
    //Allocate buffer for the data read from device, +1 for null termination
    char buffer[READ_SIZE + 1]; 

    //Check if the device file was successfully opened
    if (read_pointer < 0) {
        fprintf(stderr, "Failed to open device: %s\n", strerror(errno));
        //Returnn 1 to indicate an error condition
        return 1;
    }
    //Read from the device file
    ssize_t bytes_read = read(read_pointer, buffer, READ_SIZE);
    //Check if the read operation was successful
    if (bytes_read < 0) {
        //First cheeck if it failed due to the operation being non-blocking
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            printf("Buffer is empty, no data to read\n");     
        } else {
            //If it's for another reason print error message
            fprintf(stderr, "Read error: %s\n", strerror(errno));
            close(read_pointer);
            return 2; //return 2 to indicate a different error
        }
    } else {
        //If the read was a succes(which should not happen), null-terminate the string
        buffer[bytes_read] = '\0'; // Ensure the string is null-terminated
        close(read_pointer);
        return 3; // Return 3 to indicate unexpected success
    }

    close(read_pointer);
    return 0;
}
