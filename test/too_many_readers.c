#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include "dm510_ioctl_commands.h"
#include <sys/ioctl.h>

int main(int argc, char const *argv[]) {
    
    int read_pointer = open("/dev/dm510-0", O_RDONLY);
    if (read_pointer < 0) {
        perror("Failed to open device");
        return EXIT_FAILURE;
    }

    const int size = ioctl(read_pointer, GET_MAX_NR_PROC, 0);
    size_t i = 1;

    while (i < (size_t)size << 1) {
        int result = open("/dev/dm510-0", O_RDONLY);
        printf("Read pointer %lu : %d ", i, result);
        if (result < 0) {
            printf("invalid pointer!\n");
            return EXIT_FAILURE;
        }
        printf("\n");
        i++;
    }
    return 0;
}
