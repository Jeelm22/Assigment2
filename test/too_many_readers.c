#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include "ioctl_commands.h"
#include <sys/ioctl.h>

int main(int argc, char const *argv[]) {
    int desired_max_readers = 9; // Change this to the expected max readers
    printf("Setting max amount of readers to: %d\n", desired_max_readers);
    int max_readers; // This will hold the maximum number of readers allowed
    int ret; // To capture return values of system calls
    int *fds; // Array to keep track of file descriptors

    // Open the device file in read-only mode
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

    // Get the new max readers value to confirm it was set correctly
    ret = ioctl(read_pointer, GET_MAX_NR_PROCESSES, &max_readers);
    if (ret < 0) {
        perror("Failed to get the maximum number of reader processes");
        close(read_pointer);
        return EXIT_FAILURE;
    }

    // Allocate memory to keep track of file descriptors
    fds = malloc(sizeof(int) * (max_readers + 1));
    if (!fds) {
        perror("Failed to allocate memory for file descriptors");
        close(read_pointer);
        return EXIT_FAILURE;
    }

    // Initialize file descriptor list
    for (int i = 0; i <= max_readers; i++) {
        fds[i] = -1; // Initialize all elements to -1
    }

    // Open device files up to the maximum number of readers plus one to test the limit
    for (int i = 0; i < max_readers + 1; i++) {
    	int fd = open("/dev/dm510-0", O_RDONLY);
    	if (fd < 0) {
            perror("Error opening device file");
            break; // Stop trying once you hit the error
    	}
    
    // Only assign and print if successful and within the limit
    	if (i < max_readers) {
            fds[i] = fd;
            printf("Read pointer %d : %d\n", i + 1, fds[i]);
    	} else {
            // Here, you've reached your test case beyond the limit, so close the fd immediately
            close(fd);
    	}
}

    // Cleanup: Close all opened file descriptors
    for (int j = 1; j <= max_readers + 1; j++) {
        if (fds[j] != -1) {
            close(fds[j]);
        }
    }
    close(read_pointer);
    free(fds); // Free the memory allocated for file descriptors

    return 0;
}
