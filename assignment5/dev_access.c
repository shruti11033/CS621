#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

int read_mouse_input();
long read_write_random();
int print_tickets();
int parse_int(char *input, int *output);

int main(int argc, char **argv) {
    // Error checking for command line arguments
    // If the number of arguments is not exactly one
    if (argc < 2) {
        printf("Invalid program argument. Please enter one of 0, 1 or 2\n");
        return 1;
    }

    int return_code = 0;
    int argument = atoi(argv[1]);
    switch (argument)
    {
        case 0:
            return_code = read_mouse_input();
            break;
        case 1:
            return_code = read_write_random();
            break;
        case 2:
            return_code = print_tickets();
            break;
        default:
            printf("Invalid program argument. Please enter one of 0, 1 or 2\n");
            return 1;
    }

    return return_code;
}


int read_mouse_input() {
    char single_byte[1];
    int fd = open("/dev/input/mouse0", O_RDONLY);
    if (fd == -1) {
        perror("Failed to open /dev/input/mouse0");
        return 1;
    }

    while (true) {
        int ret = read(fd, single_byte, 1);
        if (ret == -1) {
            perror("Failed to read from /dev/input/mouse0");
            return 1;
        }
        if (ret == 0) continue;
        printf("%d\n", single_byte[0]);
    }

    if (close(fd) == -1) {
        perror("Failed to close /dev/input/mouse0");
        return 1;
    }
    
    return 0;
}


int parse_int(char *input, int *output) {
    char *scanitr = input;
    if (*scanitr == '-')
        scanitr++;
    if (*scanitr > '1' && *scanitr < '9') {
        scanitr++;
        while (*scanitr != '\0') {
            if (*scanitr < '0' || *scanitr > '9') return 1;
            scanitr++;
        }
    } else if (*scanitr != '0' || *(scanitr + 1) != '\0') return 1;

    sscanf(input, "%d", output);

    return 0;
}

//method to randomize
long read_write_random() {
    
    int random_fd = open("/dev/urandom", O_RDONLY);
    if (random_fd == -1) {
        perror("Failed to open urandom");
        return 1;
    }

    int null_fd = open("/dev/null", O_WRONLY);
    if (null_fd == -1) {
        perror("Failed to open dev/null");
        return 1;
    }

    int ret = 0;
    struct timeval start;
    struct timeval stop;
    ret = gettimeofday(&start, NULL);
    if (ret == -1) {
        perror("Failed to get start time");
        return 1;
    }

    char *data_buffer = malloc(1048576);
    int total_bytes = 1048576;
    int bytes_read = 0;
    int bytes_written = 0;
    while (bytes_read < total_bytes || bytes_written < total_bytes) {
        if (bytes_read < total_bytes) {
            ret = read(random_fd, (data_buffer + bytes_read), total_bytes - bytes_read);
            if (ret == -1) {
                perror("Failed to read /dev/urandom");
                return 1;
            }
            bytes_read += ret;
        }
            
        if (bytes_written < total_bytes && bytes_read > bytes_written) {
            ret = write(null_fd, (data_buffer + bytes_written), bytes_read - bytes_written);
            if (ret == -1) {
                perror("Failed to write to /dev/null");
                return 1;
            }
            bytes_written += ret;
        }
    }

    ret = gettimeofday(&stop, NULL);
    if (ret == -1) {
        perror("Failed to get end time");
        return 1;
    }

    printf("Time taken to read and write 10Mb is : %ld microsecnds\n",  1000000*stop.tv_sec + stop.tv_usec - 1000000*start.tv_sec - start.tv_usec);

    int return_code = 0;
    ret = close(random_fd);
    if (ret == -1) {
        perror("Failed to close /dev/urandom");
        return_code = 1;
    }

    ret = close(null_fd);
    if (ret == -1) {
        perror("Failed to close /dev/null");
        return_code = 1;
    }

    return return_code;
}


int print_tickets() {
    int fd = open("/dev/ticket0", O_RDONLY);
    if (fd == -1) {
        perror("Failed to open /dev/ticket0");
        return 1;
    }

    for (int i = 0; i < 10; i++) {
        int ticket;
        int ret = read(fd, &ticket, 4);
        if (ret != 4)
            printf("Failed to read 4 bytes on attempt : %d\n", i+1);
        else
            printf("Got ticket: %d\n", ticket);
        sleep(1);
    }

    if (close(fd) < 0) {
        perror("Failed to close /dev/ticket0");
        return 1;
    }

    return 0;
}

