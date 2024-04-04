#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

//Define number of bytes to read
#define READ_SIZE 42
#define TIMEOUT 5

int read_pointer; // Global variable for the file descriptor

// Signal handler for SIGALRM
void alarm_timer(int sig) {
    printf("Read operation timed out for 5 seconds.\n");
    if (read_pointer >= 0) {
        close(read_pointer); // Close the file descriptor if it's open
    }
    _exit(1); // Exit immediately with status 1
}

int main() {
    //Open the device file for both reading and writing
    read_pointer = open("/dev/dm510-0", O_RDWR);
    //Check if opening the device file failed
    if (read_pointer < 0) {
        fprintf(stderr, "Failed to open device: %s\n", strerror(errno));
        //Return 1 for failure
        return 1;
    }

    // Set up signal handler for SIGALRM
    signal(SIGALRM, alarm_timer);
    // Start the alarm to timeout the read operation
    alarm(TIMEOUT);

    char buffer[READ_SIZE + 1]; // Buffer for data read from the device +1 byte for null termination
    //Read up to READ_SIZE bytes from the device file into the buffer
    ssize_t bytes_read = read(read_pointer, buffer, READ_SIZE);
    //Check if read operation failed
    // Disable the alram if read complets
    alarm(0);
    if (bytes_read < 0) {
        //If it failed, print error message
        fprintf(stderr, "Read error: %s\n", strerror(errno));
    } else {
        //If unexpcted successful, null-terminate the string read into the buffer
        buffer[bytes_read] = '\0'; // Ensure the string is null-terminated
        //Print data that was read from the device
        printf("Received: '%s'\n", buffer);
    }

    close(read_pointer);
    return 0;
}

