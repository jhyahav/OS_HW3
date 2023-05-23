// The code in this file is based on chardev.c from Recitation #6.

#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
// #include <linux/string.h> TODO: check if needed
#include <linux/slab.h>
#include "message_slot.h"

MODULE_LICENSE("GPL");

//================== DATA STRUCTURES ===========================
/*
    Each Slot comprises Channels which are linked to each other.
    The Slots make up a linked list of the minor nodes we need.
    Each Channel has an ID and a message.
*/

struct Slot
{
    int minor;
    struct Slot *next_slot;
    uint32_t channel_count;
    struct Channel *channel_linked_list_head;
    struct Channel *current_channel;
};

struct Channel
{
    int channel_id;
    char *message;
    ssize_t message_length;
    struct Channel *next;
};

static struct Slot *slot_linked_list_head;
static struct Slot *current_slot;

// Structs are read & manipulated using getter/setter functions. See end of file.

//================== DEVICE FUNCTIONS ===========================
static int device_open(struct inode *inode,
                       struct file *file)
{

    unsigned int minor = iminor(inode);

    return SUCCESS;
}

//---------------------------------------------------------------
static int device_release(struct inode *inode,
                          struct file *file)
{
    // TODO: implement
    return SUCCESS;
}

//---------------------------------------------------------------
static ssize_t device_read(struct file *file,
                           char __user *buffer,
                           size_t length,
                           loff_t *offset)
{
    // TODO: implement
    int message_length = get_current_message_length();
    char *message = get_current_message();

    return read_from_buffer(buffer, message, message_length);
    // return -EINVAL;
}

int check_read_validity()
{
    // TODO: implement
}

ssize_t read_from_buffer(char __user *buffer, char *message, int message_length)
{
    int put_user_err;
    ssize_t num_bytes_read = 0;
    for (; num_bytes_read < message_length; num_bytes_read++)
    {
        put_user_err = put_user(message[num_bytes_read], &buffer[num_bytes_read]);
        if (put_user_err != SUCCESS)
        {
            return put_user_err;
        }
    }
}

//---------------------------------------------------------------
static ssize_t device_write(struct file *file,
                            const char __user *buffer,
                            size_t length,
                            loff_t *offset)
{

    int validity = set_channel_and_check_validity(file, length);
    if (validity != SUCCESS)
    {
        return validity;
    }

    return write_from_buffer(buffer, length);
}

ssize_t write_from_buffer(const char __user *buffer, size_t length)
{
    int get_user_err;
    reset_current_message();
    allocate_current_message(length);
    char *message = get_current_message();
    if (message == NULL) // if kmalloc failed
    {
        return -1;
    }
    ssize_t num_bytes_written = 0;
    for (; num_bytes_written < length; num_bytes_written++)
    {
        get_user_err = get_user(message[num_bytes_written], &buffer[num_bytes_written]);
        if (get_user_err != SUCCESS)
        {
            return get_user_err;
        }
    }

    set_current_message_length(num_bytes_written);

    return num_bytes_written;
}

int set_channel_and_check_validity(struct file *file, size_t length)
{
    int channel_set = set_channel_from_file(file);
    if (channel_set != SUCCESS)
    {
        return channel_set;
    }

    int valid_length = is_valid_length(length);
    if (valid_length != SUCCESS)
    {
        return valid_length;
    }
    return SUCCESS;
}

// Returns SUCCESS if set successfully or -EINVAL if invalid
int set_channel_from_file(struct file *file)
{
    // TODO: implement
    return -EINVAL;
}

int is_valid_length(int length)
{
    return length > BUF_LEN || length < 1 ? -EMSGSIZE : SUCCESS;
}

//----------------------------------------------------------------
static long device_ioctl(struct file *file,
                         unsigned int ioctl_command_id,
                         unsigned long ioctl_param)
{
    // TODO: implement init for slot
    if (MSG_SLOT_CHANNEL == ioctl_command_id)
    {
        // TODO: implement
    }

    return SUCCESS;
}

//==================== DEVICE SETUP =============================
struct file_operations Fops = {
    .owner = THIS_MODULE,
    .read = device_read,
    .write = device_write, // FIXME: Fops is good to go
    .open = device_open,
    .unlocked_ioctl = device_ioctl,
    .release = device_release,
};

//---------------------------------------------------------------
static int __init device_init(void) // FIXME: good to go
{
    int rc = -1;
    rc = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops);

    if (rc < 0)
    {
        printk(KERN_ERR "%s registration failed for  %d\n",
               DEVICE_FILE_NAME, MAJOR_NUM);
        return rc;
    }

    printk("message_slot initialization was successful.");

    return SUCCESS;
}

//---------------------------------------------------------------
static void __exit device_cleanup(void)
{
    clean_up_memory();
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------

static void clean_up_memory(void)
{
    // TODO: implement
}

module_init(device_init);
module_exit(device_cleanup);

//================== GETTERS & SETTERS ===========================

static struct Channel *get_current_channel(void)
{
    return current_slot->current_channel;
}

static void set_current_channel(struct Channel *channel)
{
    current_slot->current_channel = channel;
}

static u_int32_t get_channel_count(void)
{
    return current_slot->channel_count;
}

static void set_channel_count(u_int32_t count)
{
    current_slot->channel_count = count;
}

static char *get_current_message(void)
{
    return get_current_channel()->message;
}

static void allocate_current_message(size_t size)
{
    struct Channel *current_channel = get_current_channel();
    current_channel->message = (char *)kmalloc(size, GFP_KERNEL); // TODO: consider changing size to BUF_LEN
}

static void *reset_current_message(void)
{
    char *current_message = get_current_message();
    if (current_message != NULL)
    {
        kfree(current_message);
    }
}

static int get_current_message_length(void)
{
    return get_current_channel()->message_length;
}

static void set_current_message_length(int length)
{
    get_current_channel()->message_length = length; // TODO: make sure I work!
}

static int get_current_channel_id(void)
{
    return get_current_channel()->channel_id;
}

static void set_current_channel_id(int id)
{
    get_current_channel()->channel_id = id;
}

static struct Channel *get_next_channel(void)
{
    return get_current_channel()->next;
}

static void set_next_channel(struct Channel *next_channel)
{
    get_current_channel()->next = next_channel;
}