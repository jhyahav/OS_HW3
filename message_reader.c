#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include "message_slot.h"

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
        exit(EXIT_FAILURE);
    }
}
