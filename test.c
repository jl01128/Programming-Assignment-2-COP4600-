#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define IN_DEVICE "/dev/pa2_in"
#define OUT_DEVICE "/dev/pa2_out"

int main() {
    int fd_in, fd_out;
    char write_buf[] = "Hello, this is a test message!";
    char read_buf[1024];
    ssize_t bytes_written, bytes_read;

    // Open the pa2_in device for writing
    fd_in = open(IN_DEVICE, O_WRONLY);
    if (fd_in < 0) {
        perror("Failed to open pa2_in");
        return 1;
    }

    // Open the pa2_out device for reading
    fd_out = open(OUT_DEVICE, O_RDONLY);
    if (fd_out < 0) {
        perror("Failed to open pa2_out");
        close(fd_in);
        return 1;
    }

    // Write to the pa2_in device
    bytes_written = write(fd_in, write_buf, strlen(write_buf));
    if (bytes_written < 0) {
        perror("Failed to write to pa2_in");
    } else {
        printf("Wrote %zd bytes to pa2_in\n", bytes_written);
    }

    // Read from the pa2_out device
    bytes_read = read(fd_out, read_buf, sizeof(read_buf) - 1);
    if (bytes_read < 0) {
        perror("Failed to read from pa2_out");
    } else {
        read_buf[bytes_read] = '\0';
        printf("Read %zd bytes from pa2_out: %s\n", bytes_read, read_buf);
    }

    // Close the devices
    close(fd_in);
    close(fd_out);

    return 0;
}

