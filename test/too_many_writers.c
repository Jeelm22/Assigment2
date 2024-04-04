#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include "ioctl_commands.h"

int main(int argc, char const *argv[]) {
    //Initialize loop counter, for looping through devices
    size_t i = 0; 
    //While loop to attempt opening DEVICE_COUNT number of device files
    while (i < DEVICE_COUNT) {
        //Try to open with read and write premission
        int write_pointer = open("/dev/dm510-0", O_RDWR);
        printf("Write pointer %lu : %d ", i, write_pointer);
        //Check if the file descriptor is invalid
        if (0 > write_pointer) {
            printf("invalid pointer!\n");
            //Exit the program with an error state if falied
            return 0;
        }
        //Print newline if the file descriptor is valid
        printf("\n");
        i++; // Increment loop counter
    }
    //Exit the program succesfully, if there loop completes without errors
    return 0;
}
