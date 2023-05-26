#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "message_slot.h"

static void check_arguments(int argc);
static int open_file_for_read(char *path);
static int set_channel_and_read_message(int fd, unsigned int command_id, int channel_id, char *buffer);
static void set_channel(int fd, unsigned int command_id, int channel_id);
static int read_message(int fd, char *buffer);
static void print_message(char *buffer, int length);
static void error_and_exit(void);

int main(int argc, char *argv[])
{
    check_arguments(argc);
    int fd = open_file_for_read(argv[1]);
    char buffer[BUF_LEN];
    int bytes_read = set_channel_and_read_message(fd, MSG_SLOT_CHANNEL, atoi(argv[2]), buffer);
    print_message(buffer, bytes_read);
    return 0;
}

static void check_arguments(int argc)
{
    if (argc != 3)
    {
        perror("Incorrect number of arguments for message_reader.c.\n");
        exit(EXIT_FAILURE);
    }
}

static int open_file_for_read(char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        error_and_exit();
    }
    return fd;
}

static int set_channel_and_read_message(int fd, unsigned int command_id, int channel_id, char *buffer)
{
    set_channel(fd, command_id, channel_id);
    return read_message(fd, buffer);
}

static void set_channel(int fd, unsigned int command_id, int channel_id)
{
    int ioctl_err = ioctl(fd, command_id, channel_id);
    if (ioctl_err != 0)
    {
        error_and_exit();
    }
}

static int read_message(int fd, char *buffer)
{
    int bytes_read = read(fd, buffer, BUF_LEN);
    if (bytes_read < 0)
    {
        error_and_exit();
    }
    return bytes_read;
}

static void print_message(char *buffer, int length)
{
    int bytes_printed = write(STDOUT_FILENO, buffer, length);
    if (bytes_printed != length)
    {
        error_and_exit();
    }
}

static void error_and_exit(void)
{
    perror(strerror(errno));
    exit(EXIT_FAILURE);
}