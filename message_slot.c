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

typedef struct
{
    int minor;
    struct Slot *next_slot;
    uint32_t channel_count;
    struct Channel *channel_linked_list_head;
    struct Channel *current_channel;
} Slot;

typedef struct
{
    int channel_id;
    char message[BUF_LEN];
    int message_length;
    struct Channel *next;
} Channel;

static int dev_open_flag = 0;

static Slot device_info;

//================== DEVICE FUNCTIONS ===========================
static int device_open(struct inode *inode,
                       struct file *file)
{
    if (dev_open_flag == 1)
    {
        return -EBUSY;
    }

    ++dev_open_flag;
    return SUCCESS;
}

//---------------------------------------------------------------
static int device_release(struct inode *inode,
                          struct file *file)
{
    // TODO: implement
    unsigned long flags;
    --dev_open_flag;
    return SUCCESS;
}

//---------------------------------------------------------------
static ssize_t device_read(struct file *file,
                           char __user *buffer,
                           size_t length,
                           loff_t *offset)
{
    // TODO: implement
    return -EINVAL;
}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
static ssize_t device_write(struct file *file,
                            const char __user *buffer,
                            size_t length,
                            loff_t *offset)
{
    // TODO: implement
    ssize_t i;
    for (i = 0; i < length && i < BUF_LEN; ++i)
    {
        // get_user(the_message[i], &buffer[i]);
        // if (1 == encryption_flag)
        //     the_message[i] += 1;
    }

    // return the number of input characters used
    return i;
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
