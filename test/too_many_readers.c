#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include "ioctl_commands.h"
#include <sys/ioctl.h>

int main(int argc, char const *argv[]) {
    int desired_max_readers = 5; // Set this to the number of readers you want to test with

    // Attempt to open the device file in read-only mode
    int read_pointer = open("/dev/dm510-0", O_RDONLY);
    // Check if the device file opened successfully
    if (read_pointer < 0) {
        perror("Failed to open device");
        // Return EXIT_FAILURE if the file couldn't be opened, indicating unsuccessful execution
        return EXIT_FAILURE;
    }

    // Set maximum number of processes allowed to read from the device with ioctl command
    if(ioctl(read_pointer, SET_MAX_NR_PROCESSES, &desired_max_readers) < 0) {
        perror("Failed to set max number of reader processes");
        close(read_pointer);
        return EXIT_FAILURE;
    }

    // Get the newly set maximum number of processes to validate
    int max_readers = ioctl(read_pointer, GET_MAX_NR_PROCESSES, 0);
    printf("Max readers set to: %d\n", max_readers);

    // Initialize a counter for the loop
    size_t i = 1;

    // While loop to attempt opening additional instances of the device file
    while (i <= (size_t)desired_max_readers) {
        // Open device file in read-only mode
        int result = open("/dev/dm510-0", O_RDONLY);
        // Print our current loop iteration and the file descriptor obtained
        printf("Read pointer %lu : %d ", i, result);
        // Check if opening the device file failed
        if (result < 0) {
            printf("invalid pointer!\n");
            break; // Break the loop if we've hit the maximum number of readers
        }
        // If file descriptor is valid, print a newline
        printf("\n");
        i++;
    }

    // Cleanup: Close all opened file descriptors
    for (size_t j = 1; j < i; j++) {
        close(j);
    }
    close(read_pointer);

    // Return 0, to indicate success
    return 0;
}
