#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include "dm510_ioctl_commands.h"

int main(int argc, char const *argv[]) {
    size_t i = 0; // Initialize loop counter outside the loop
    while (i < DEVICE_COUNT) {
        int write_pointer = open("/dev/dm510-0", O_RDWR);
        printf("Write pointer %lu : %d ", i, write_pointer);
        if (0 > write_pointer) {
            printf("invalid pointer!\n");
            return 0;
        }
        printf("\n");
        i++; // Increment loop counter
    }
    return 0;
}
