#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	printf("File Name,Write Operations,Time taken (microseconds)\n");
	int file_size_in_bytes = 131072;
	char *data_buffer = malloc(file_size_in_bytes);

	int writes_sizes[6] = {2, 16, 128, 1024, 8192, 65536};
	for (int i = 0; i < 6; i++)
	{
		for (int j = 0; j < 10; j++)
		{
			// get file path
			const char *file_path_prefix = "/tmp/write_file_";
			char *num1;
			char *num2;
			char file_path[50];
			asprintf(&num1, "%d", writes_sizes[i]);
			strcat(strcpy(file_path, file_path_prefix), num1);
			strcat(file_path, "bytes_");
			asprintf(&num2, "%d", j+1);
			strcat(file_path, num2);
			free(num1);
			free(num2);

			int fd = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0666);
			if (fd == -1)
			{
				perror("Failed to open file");
				return 1;
			}

			struct timeval start;
			struct timeval stop;
			if (gettimeofday(&start, NULL) == -1)
			{
				perror("Failed to get start time");
				return 1;
			}

			// write loop begins
			int write_ops = 0;
			int bytes_written = 0;
			while (bytes_written < file_size_in_bytes)
			{
				int ret = write(fd, (data_buffer + bytes_written), writes_sizes[i]);
				if (ret == -1)
				{
					perror("Failed to write to /dev/null");
					return 1;
				}
				bytes_written += ret;
				write_ops += 1;
			}
			// wite loop ends

			if (gettimeofday(&stop, NULL) == -1)
			{
				perror("Failed to get stop time");
				return 1;
			}

			printf("%s,%d,%ld\n", file_path, write_ops, 1000000 * stop.tv_sec + stop.tv_usec - 1000000 * start.tv_sec - start.tv_usec);
			//printf("File: %s, WriteOps: %d, Time(us) : %ld\n", file_path, write_ops, 1000000 * stop.tv_sec + stop.tv_usec - 1000000 * start.tv_sec - start.tv_usec);

			if (close(fd) == -1)
			{
				perror("Failed to close file");
				return 1;
			}
		}
	}

	return 0;
}
