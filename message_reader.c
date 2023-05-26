#include "message_slot.h"
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

static void check_arguments(int argc);

int main(int argc, char *argv)
{
    check_arguments(argc);
}

static void check_arguments(int argc)
{
    if (argc != 3)
    {
        perror("Incorrect number of arguments for message_reader.c.\n");
        exit(1);
    }
}