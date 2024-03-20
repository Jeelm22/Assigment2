#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include "dm510_ioctl_commands.h"

int main(int argc, char const *argv[]) {
    if(argc <= 1) {
        fprintf(stderr, "Usage: %s <message>\n", argv[0]);
        return 1;
    }

    int dev1 = open("/dev/dm510-0", O_RDWR);
    if(dev1 < 0) {
        perror("Error opening /dev/dm510-0");
        return 1;
    }

    int dev2 = open("/dev/dm510-1", O_RDWR);
    if(dev2 < 0) {
        perror("Error opening /dev/dm510-1");
        close(dev1); // Close the first device if the second fails to open
        return 1;
    }

    const char *msg = argv[1];
    size_t size = strlen(msg);
    printf("Writing: %s\n", msg);

    if(write(dev1, msg, size) < 0) {
        int bufferSize = ioctl(dev1, GET_BUFFER_SIZE);
        fprintf(stderr, "Failed to write. Message size %zu is larger than buffer size %d\n", size, bufferSize);
        close(dev1);
        close(dev2);
        return 1;
    }

    char *buf = malloc(size + 1); // Allocate buffer for the read message plus null terminator
    if(buf == NULL) {
        perror("Failed to allocate buffer");
        close(dev1);
        close(dev2);
        return 1;
    }

    ssize_t bytesRead = read(dev2, buf, size);
    if(bytesRead < 0) {
        perror("Error reading from device");
        free(buf);
        close(dev1);
        close(dev2);
        return 1;
    }

    buf[bytesRead] = '\0'; // Ensure the buffer is null-terminated
    printf("Read: %s\n", buf);

    free(buf);
    close(dev1);
    close(dev2);

    return 0;
}
