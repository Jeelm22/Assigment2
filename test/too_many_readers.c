#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include "ioctl_commands.h"
#include <sys/ioctl.h>

int main(int argc, char const *argv[]) {
printf("1:Size of int in user space: %zu bytes\n", sizeof(int));
    int desired_max_readers = 9; // Change this to the expected max readers
    int max_readers; // This will hold the maximum number of readers allowed
    int ret; // To capture return values of system calls
    int *fds; // Array to keep track of file descriptors
printf("2:Size of int in user space: %zu bytes\n", sizeof(int));

    // Open the device file in read-only mode
    int read_pointer = open("/dev/dm510-0", O_RDONLY);
    if (read_pointer < 0) {
        perror("Failed to open the device file");
        return EXIT_FAILURE;
    }

    // Set the maximum number of reader processes
    printf("Setting max amount of readers to: %d\n", desired_max_readers);
    ret = ioctl(read_pointer, SET_MAX_NR_PROCESSES, &desired_max_readers);
    printf("Setting max amount of readers to: %d\n", desired_max_readers);
    printf("ioctl SET_MAX_NR_PROCESSES return value: %d\n", ret);
    if (ret < 0) {
        perror("Failed to set the maximum number of reader processes");
        close(read_pointer);
        return EXIT_FAILURE;
    }
printf("Setting max amount of readers to: %d\n", desired_max_readers);

    // Get the new max readers value to confirm it was set correctly
    ret = ioctl(read_pointer, GET_MAX_NR_PROCESSES, &max_readers);
    printf("ioctl GET_MAX_NR_PROCESSES return value: %d, max readers: %d\n", ret, max_readers);
    if (ret < 0) {
        perror("Failed to get the maximum number of reader processes");
        close(read_pointer);
        return EXIT_FAILURE;
    }
    printf("Max amount of readers set to: %d\n", max_readers);

    // Allocate memory to keep track of file descriptors
    fds = malloc(sizeof(int) * (max_readers + 1));
    printf("Setting max amount of readers to: %d\n", max_readers);
    if (!fds) {
        perror("Failed to allocate memory for file descriptors");
        close(read_pointer);
        return EXIT_FAILURE;
    }

    // Initialize file descriptor list
    for (int i = 0; i <= max_readers; i++) {
        fds[i] = -1; // Initialize all elements to -1
    }

    // Open device files up to the maximum number of readers
    for (int i = 1; i <= max_readers + 1; i++) {
        printf("Setting max amount of readers to: %d\n", max_readers);
        fds[i] = open("/dev/dm510-0", O_RDONLY);
        printf("Read pointer %d : %d ", i, fds[i]);
        if (fds[i] < 0) {
            perror("Error opening device file");
            break;
        }
        printf("\n");
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
