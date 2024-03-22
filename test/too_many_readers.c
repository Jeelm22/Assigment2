#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include "dm510_ioctl_commands.h"
#include <sys/ioctl.h>

int main(int argc, char const *argv[]) {
    //Attempt to open the device file in read-only mode
    int read_pointer = open("/dev/dm510-0", O_RDONLY);
    //Check if the device file opened succesfully
    if (read_pointer < 0) {
        perror("Failed to open device");
        //Return EXIT_FALIURE if the file couldn't be opened, indicating unsuccesful execution
        return EXIT_FAILURE;
    }
    //Get maximum number of processes allowed to read fro the device, with ioctl commmand
    const int size = ioctl(read_pointer, GET_MAX_NR_PROC, 0);
    //Initialize a counter for the loop
    size_t i = 1;

    //While loop to attept opening additional instance of the device file
    while (i < (size_t)size << 1) {
        //Open device file in read-only mode
        int result = open("/dev/dm510-0", O_RDONLY);
        //Print our current loop iteration and the file descriptor obrained
        printf("Read pointer %lu : %d ", i, result);
        //Check if opening the device file failed
        if (result < 0) {
            printf("invalid pointer!\n");
            //Exit the program, indicating failure
            return EXIT_FAILURE;
        }
        //If file descriptor is valid, print a newline
        printf("\n");
        i++;
    }
    //Return 0, to indicate success 
    return 0;
}
