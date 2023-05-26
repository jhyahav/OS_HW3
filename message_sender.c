#include "message_slot.h"
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

static void check_arguments(int argc);

int main(int argc, char *argv[])
{
    check_arguments(argc);
    int fd = open_file(argv[1]);
    set_channel_and_write_message(fd, MSG_SLOT_CHANNEL, atoi(argv[2]), argv[3]);
    close(fd);
    exit(SUCCESS);
}

static void check_arguments(int argc)
{
    if (argc != 4)
    {
        perror("Incorrect number of arguments for message_sender.c.\n");
        exit(EXIT_FAILURE);
    }
}

static int open_file(char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        error_and_exit();
    }
    return fd;
}

static void set_channel_and_write_message(int fd, unsigned int command_id, int channel_id, char *message)
{
    set_channel(fd, command_id, channel_id);
}

static void set_channel(int fd, unsigned int command_id, int channel_id)
{
    int ioctl_err = ioctl(fd, command_id, channel_id);
    if (ioctl_err != 0)
    {
        error_and_exit();
    }
}

static void write_message(int fd, char *message)
{
    int write_err = write(fd, message, strlen(message));
    if (write_err != 0)
    {
        error_and_exit();
    }
}

static void error_and_exit(void)
{
    perror(strerror(errno));
    exit(EXIT_FAILURE);
}