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
    int max_readers; // This will hold the maximum number of readers allowed
    int ret; // To capture return values of system calls

    // Attempt to open the device file in read-only mode
    int read_pointer = open("/dev/dm510-0", O_RDONLY);
    if (read_pointer < 0) {
        perror("Failed to open the device file");
        return EXIT_FAILURE;
    }

    // Set the maximum number of reader processes
    ret = ioctl(read_pointer, SET_MAX_NR_PROCESSES, &desired_max_readers);
    if (ret < 0) {
        perror("Failed to set the maximum number of reader processes");
        close(read_pointer);
        return EXIT_FAILURE;
    }

    // Retrieve the new max readers value to confirm it was set correctly
    ret = ioctl(read_pointer, GET_MAX_NR_PROCESSES, &max_readers);
    if (ret < 0) {
        perror("Failed to get the maximum number of reader processes");
        close(read_pointer);
        return EXIT_FAILURE;
    }
    printf("Max readers set to: %d\n", max_readers);

    // Initialize a counter for the loop
    size_t i = 1;

    // While loop to attempt opening additional instances of the device file
    while (i <= (size_t)max_readers) {
        // Open device file in read-only mode
        int result = open("/dev/dm510-0", O_RDONLY);
        // Print our current loop iteration and the file descriptor obtained
        printf("Read pointer %zu : %d ", i, result);
        // Check if opening the device file failed
        if (result < 0) {
            perror("Error opening device file");
            break; // Break the loop if we've hit the maximum number of readers
        }
        // If file descriptor is valid, print a newline
        printf("\n");
        i++;
    }

    // Cleanup: Close all opened file descriptors
    for (size_t j = 1; j < i; j++) {
        close(j + 3); // Adjusted to close the correct file descriptors
    }
    close(read_pointer);

    // Return 0, to indicate success
    return 0;
}
