#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define DEVICE_FILE "/dev/dm510-0"
#define READ_SIZE 42

int main() {
    int fd = open(DEVICE_FILE, O_RDWR);
    char buffer[READ_SIZE + 1]; // +1 for null termination

    if (fd < 0) {
        fprintf(stderr, "Failed to open device: %s\n", strerror(errno));
        return 1;
    }

    ssize_t bytes_read = read(fd, buffer, READ_SIZE);
    if (bytes_read < 0) {
        fprintf(stderr, "Read error: %s\n", strerror(errno));
    } else {
        buffer[bytes_read] = '\0'; // Ensure the string is null-terminated
        printf("Received: '%s'\n", buffer);
    }

    close(fd);
    return 0;
}

