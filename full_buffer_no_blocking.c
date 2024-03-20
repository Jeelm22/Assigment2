#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "dm510_ioctl_commands.h"

int main(int argc, char const *argv[]) {
    int write_pointer = open("/dev/dm510-0", O_RDWR | O_NONBLOCK);
    if (write_pointer < 0) {
        perror("Failed to open device");
        return 1;
    }

    const int size = ioctl(write_pointer, GET_BUFFER_SIZE);
    char n = 0;
    size_t i = 0;

    while (i < size) {
        write(write_pointer, &n, sizeof(n));
        n++;
        i++;
    }

    if (write(write_pointer, &n, sizeof(n)) < 0) {
        printf("Buffer is full\n");
    }

    close(write_pointer); // Close the device file descriptor
    return 0;
}
