#include "message_slot.h"
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

static void check_arguments(int argc);

int main(int argc, char *argv[])
{
    check_arguments(argc);
}

static void check_arguments(int argc)
{
    if (argc != 4)
    {
        perror("Incorrect number of arguments for message_sender.c.\n");
        exit(1);
    }
}