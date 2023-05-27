// The code in this file is based on chardev.c from Recitation #6.

#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
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
// These are treated as "private" variables
static struct Slot *slot_linked_list_head = NULL;
static struct Slot *current_slot = NULL;

// Structs are read & manipulated using getter/setter functions.

//================== DECLARATIONS ===========================

static int append_slot_to_ll(int minor, struct Slot *tail);

static ssize_t read_buffer(char __user *buffer, size_t buffer_length, char *message, int message_length);

static int back_up_user_buffer(char __user *buffer, size_t buffer_length, char *backup_buffer);

static int restore_user_buffer_on_failure(char __user *buffer, size_t buffer_length, char *backup_buffer);

static int set_channel_and_check_read_validity(struct file *file, int buffer_length);

static int is_valid_read_length(int message_length, int buffer_length);

static ssize_t write_buffer(const char __user *buffer, size_t length);

static int back_up_current_message(char *backup);

static void restore_message_on_failure(char *backup, int backup_length);

static int set_channel_and_check_write_validity(struct file *file, size_t length);

static int set_channel_from_file(struct file *file);

static int set_state(int minor, int id);

static int search_and_set_slot_by_minor(int minor);

static int search_and_set_channel_by_id(int id);

static struct Slot *get_current_slot(void);

static void set_current_slot(struct Slot *slot);

static struct Slot *get_slot_ll_head(void);

static void set_slot_ll_head(struct Slot *slot);

static int get_current_slot_minor(void);

static void set_current_slot_minor(int minor);

static struct Slot *get_next_slot(void);

static void set_next_slot(struct Slot *slot);

static int is_valid_write_length(int length);

static int is_valid_ioctl_op(unsigned int ioctl_command_id, unsigned int channel_id);

static int find_or_create_channel(unsigned int channel_id);

static int initialize_slot_channel_ll(unsigned int id);

static int find_or_append_channel_in_existing_ll(unsigned int channel_id);

static int append_channel_to_ll(unsigned int id, struct Channel *tail);

static void initialize_current_channel(unsigned int id);

static void write_channel_to_file(struct file *file, struct Channel *channel);

static int initialize_slot_ll_head(void);

static void clean_up_slots(void);

static void clean_up_channels(void);

static struct Channel *get_current_channel(void);

static void set_current_channel(struct Channel *channel);

static u_int32_t get_channel_count(void);

static void set_channel_count(u_int32_t count);

static struct Channel *get_slot_channel_ll_head(void);

static void set_slot_channel_ll_head(struct Channel *channel);

static char *get_current_message(void);

static void allocate_current_message(size_t size);

static void reset_current_message(void);

static int get_current_message_length(void);

static void set_current_message_length(int length);

static int get_current_channel_id(void);

static int get_channel_id(struct Channel *channel);

static void set_current_channel_id(int id);

static struct Channel *get_next_channel(void);

static void set_next_channel(struct Channel *next_channel);

//================== DEVICE FUNCTIONS ===========================
static int device_open(struct inode *inode,
                       struct file *file)
{

    int minor = iminor(inode);
    struct Slot *slot = get_slot_ll_head();
    while (slot->next_slot != NULL)
    {
        if (slot->minor == minor)
        {
            set_current_slot(slot);
            return SUCCESS;
        }
        slot = slot->next_slot;
    }

    set_current_slot(slot); // last slot in linked list
    if (get_current_slot_minor() == minor)
    {
        return SUCCESS;
    }

    return append_slot_to_ll(minor, get_current_slot());
}

static int append_slot_to_ll(int minor, struct Slot *tail)
{
    void *new_slot_address = (struct Slot *)kmalloc(sizeof(struct Slot), GFP_KERNEL);
    if (!new_slot_address)
    {
        return -ENOMEM;
    }
    set_current_slot(tail); // we should enter this function with tail already current, but just in case
    set_next_slot(new_slot_address);
    set_current_slot(new_slot_address);
    set_current_slot_minor(minor);
    set_channel_count(0);
    set_next_slot(NULL);
    set_slot_channel_ll_head(NULL);
    set_current_channel(NULL);
    return SUCCESS;
}

//---------------------------------------------------------------
static int device_release(struct inode *inode,
                          struct file *file)
{
    return SUCCESS;
}

//---------------------------------------------------------------
static ssize_t device_read(struct file *file,
                           char __user *buffer,
                           size_t length,
                           loff_t *offset)
{
    int message_length;
    char *message;
    int validity = set_channel_and_check_read_validity(file, length);
    if (validity != SUCCESS)
    {
        return validity;
    }

    message_length = get_current_message_length();
    message = get_current_message();

    return read_buffer(buffer, length, message, message_length);
}

static ssize_t read_buffer(char __user *buffer, size_t buffer_length, char *message, int message_length)
{
    ssize_t num_bytes_read = 0;
    char *backup_buffer = NULL;
    int put_user_err;
    int backup_err = back_up_user_buffer(buffer, buffer_length, backup_buffer);
    if (backup_err != SUCCESS)
    {
        return backup_err;
    }
    for (; num_bytes_read < message_length; num_bytes_read++)
    {
        put_user_err = put_user(message[num_bytes_read], &buffer[num_bytes_read]);
        if (put_user_err != SUCCESS)
        {
            restore_user_buffer_on_failure(buffer, buffer_length, backup_buffer);
            return put_user_err;
        }
    }
    kfree(backup_buffer);
    return num_bytes_read;
}

// Intended to ensure that read operations are atomic.
static int back_up_user_buffer(char __user *buffer, size_t buffer_length, char *backup_buffer)
{
    backup_buffer = (char *)kmalloc(buffer_length, GFP_KERNEL);
    if (!backup_buffer)
    {
        return -ENOMEM;
    }
    if (!buffer)
    {
        return -EINVAL;
    }

    return -copy_from_user(backup_buffer, buffer, buffer_length); // 0 if successful, negative otherwise
}

static int restore_user_buffer_on_failure(char __user *buffer, size_t buffer_length, char *backup_buffer)
{
    int copy_err = copy_to_user(buffer, backup_buffer, buffer_length);
    kfree(backup_buffer);
    return copy_err;
}

static int set_channel_and_check_read_validity(struct file *file, int buffer_length)
{
    int message_length;
    int valid_length;
    int channel_set = set_channel_from_file(file);
    if (channel_set != SUCCESS)
    {
        return channel_set;
    }

    message_length = get_current_message_length();

    valid_length = is_valid_read_length(message_length, buffer_length);
    if (valid_length != SUCCESS)
    {
        return valid_length;
    }
    return SUCCESS;
}

static int is_valid_read_length(int message_length, int buffer_length)
{
    if (message_length < 1)
    {
        return -EWOULDBLOCK;
    }
    return buffer_length < message_length ? -ENOSPC : SUCCESS;
}

//---------------------------------------------------------------
static ssize_t device_write(struct file *file,
                            const char __user *buffer,
                            size_t length,
                            loff_t *offset)
{

    int validity = set_channel_and_check_write_validity(file, length);
    if (validity != SUCCESS)
    {
        return validity;
    }

    return write_buffer(buffer, length);
}

static ssize_t write_buffer(const char __user *buffer, size_t length)
{
    char *previous_message = NULL;
    int backup_err;
    int backup_length;
    int get_user_err;
    char *message;
    ssize_t num_bytes_written;
    backup_length = get_current_message_length();
    backup_err = back_up_current_message(previous_message);
    if (!buffer)
    {
        return -EINVAL;
    }
    if (backup_err != SUCCESS)
    {
        return backup_err;
    }
    reset_current_message();
    allocate_current_message(length);
    message = get_current_message();
    if (message == NULL) // if kmalloc failed
    {
        return -ENOMEM;
    }
    num_bytes_written = 0;
    for (; num_bytes_written < length; num_bytes_written++)
    {
        get_user_err = get_user(message[num_bytes_written], &buffer[num_bytes_written]);
        if (get_user_err != SUCCESS)
        {
            restore_message_on_failure(previous_message, backup_length);
            return get_user_err;
        }
    }

    set_current_message_length(num_bytes_written);
    kfree(previous_message);
    return num_bytes_written;
}

static int back_up_current_message(char *backup)
{
    int i = 0;
    char *message = get_current_message();
    int message_length = get_current_message_length();
    backup = (char *)kmalloc(message_length, GFP_KERNEL);
    if (!backup)
    {
        return -ENOMEM;
    }
    for (; i < message_length; i++)
    {
        backup[i] = message[i];
    }
    return SUCCESS;
}

// Intended to ensure that write operations are atomic.
static void restore_message_on_failure(char *backup, int backup_length)
{
    int i = 0;
    reset_current_message();
    allocate_current_message(backup_length);
    set_current_message_length(backup_length);
    for (; i < backup_length; i++)
    {
        get_current_message()[i] = backup[i];
    }
    kfree(backup);
}

static int set_channel_and_check_write_validity(struct file *file, size_t length)
{
    int channel_set;
    int valid_length;
    channel_set = set_channel_from_file(file);
    if (channel_set != SUCCESS)
    {
        return channel_set;
    }

    valid_length = is_valid_write_length(length);
    if (valid_length != SUCCESS)
    {
        return valid_length;
    }
    return SUCCESS;
}

static int is_valid_write_length(int length)
{
    return length > BUF_LEN || length < 1 ? -EMSGSIZE : SUCCESS;
}

//----------------------------------------------------------------
static long device_ioctl(struct file *file,
                         unsigned int ioctl_command_id,
                         unsigned long ioctl_param)
{
    int validity;
    int channel_err;
    validity = is_valid_ioctl_op(ioctl_command_id, ioctl_param);
    if (validity != SUCCESS)
    {
        return validity;
    }
    channel_err = find_or_create_channel(ioctl_param);
    if (channel_err != SUCCESS)
    {
        return channel_err;
    }

    // After the above, the current channel should be the one we want
    write_channel_to_file(file, get_current_channel());
    return SUCCESS;
}

static int is_valid_ioctl_op(unsigned int ioctl_command_id, unsigned int channel_id)
{
    return ioctl_command_id == MSG_SLOT_CHANNEL && channel_id != 0 ? SUCCESS : -EINVAL;
}

static int find_or_create_channel(unsigned int channel_id)
{
    if (get_channel_count() == 0)
    {
        return initialize_slot_channel_ll(channel_id);
    }
    return find_or_append_channel_in_existing_ll(channel_id);
}

static int initialize_slot_channel_ll(unsigned int id)
{
    void *new_channel_address = (struct Channel *)kmalloc(sizeof(struct Channel), GFP_KERNEL);
    if (!new_channel_address)
    {
        return -ENOMEM;
    }
    set_slot_channel_ll_head(new_channel_address);
    set_current_channel(get_slot_channel_ll_head());
    initialize_current_channel(id);
    return SUCCESS;
}

static int find_or_append_channel_in_existing_ll(unsigned int channel_id)
{
    struct Channel *channel = get_slot_channel_ll_head();
    while (channel->next != NULL)
    {
        if (channel->channel_id == channel_id)
        {
            set_current_channel(channel);
            return SUCCESS;
        }
        channel = channel->next;
    }

    set_current_channel(channel); // last channel in linked list
    if (get_current_channel_id() == channel_id)
    {
        return SUCCESS;
    }

    return append_channel_to_ll(channel_id, get_current_channel());
}

static int append_channel_to_ll(unsigned int id, struct Channel *tail)
{
    void *new_channel_address = (struct Channel *)kmalloc(sizeof(struct Channel), GFP_KERNEL);
    if (!new_channel_address)
    {
        return -ENOMEM;
    }
    set_current_channel(tail);
    set_next_channel(new_channel_address);
    set_current_channel(new_channel_address);
    set_next_channel(NULL);
    initialize_current_channel(id);
    return SUCCESS;
}

static void initialize_current_channel(unsigned int id)
{
    set_channel_count(get_channel_count() + 1);
    set_current_channel_id(id);
    set_current_message_length(0);
    set_next_channel(NULL);
}

static void write_channel_to_file(struct file *file, struct Channel *channel)
{
    file->private_data = (void *)channel;
}

//==================== DEVICE SETUP =============================
struct file_operations Fops = {
    .owner = THIS_MODULE,
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .unlocked_ioctl = device_ioctl,
    .release = device_release,
};

//---------------------------------------------------------------
static int __init device_init(void)
{
    int slot_err;
    int rc = -1;
    rc = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops);

    if (rc < 0)
    {
        printk(KERN_ERR "%s registration failed for  %d\n",
               DEVICE_FILE_NAME, MAJOR_NUM);
        return rc;
    }

    slot_err = initialize_slot_ll_head();
    if (slot_err != SUCCESS)
    {
        return slot_err;
    }

    printk("message_slot initialization was successful.");

    return SUCCESS;
}

static int initialize_slot_ll_head(void)
{
    void *new_head_slot_address = (struct Slot *)kmalloc(sizeof(struct Slot), GFP_KERNEL);
    if (!new_head_slot_address)
    {
        return -ENOMEM;
    }
    set_slot_ll_head(new_head_slot_address);
    set_current_slot(new_head_slot_address);
    set_next_slot(NULL);
    set_slot_channel_ll_head(NULL);
    set_current_channel(NULL);
    set_channel_count(0);
    set_current_slot_minor(UNDEFINED);
    return SUCCESS;
}

//---------------------------------------------------------------
static void __exit device_cleanup(void)
{
    clean_up_slots();
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

static void clean_up_slots(void)
{
    struct Slot *next_slot;
    set_current_slot(get_slot_ll_head());
    while (get_current_slot() != NULL)
    {
        next_slot = get_next_slot();
        clean_up_channels();
        kfree(get_current_slot());
        set_current_slot(next_slot);
    }
}

static void clean_up_channels(void)
{
    struct Channel *next_channel;
    set_current_channel(get_slot_channel_ll_head());
    while (get_current_channel() != NULL)
    {
        next_channel = get_next_channel();
        reset_current_message();
        kfree(get_current_channel());
        set_current_channel(next_channel);
    }
}

//---------------------------------------------------------------

module_init(device_init);
module_exit(device_cleanup);

//================== FUNCTIONS FOR STRUCTS ===========================

// Returns SUCCESS if set successfully or -EINVAL if invalid
static int set_channel_from_file(struct file *file)
{
    int minor = iminor(file_inode(file));
    int id;
    struct Channel *channel;
    if (file->private_data == NULL)
    { // if read/write attempted before ioctl invoked
        return -EINVAL;
    }
    channel = (struct Channel *)file->private_data;
    id = get_channel_id(channel);
    return set_state(minor, id);
}

static int set_state(int minor, int id)
{
    int slot_err = search_and_set_slot_by_minor(minor);
    int channel_err = search_and_set_channel_by_id(id);

    return slot_err == SUCCESS && channel_err == SUCCESS ? SUCCESS : -EINVAL;
}

static int search_and_set_slot_by_minor(int minor)
{
    struct Slot *slot = get_slot_ll_head();
    // Standard linked list traversal search
    while (slot != NULL)
    {
        if (slot->minor == minor)
        {
            set_current_slot(slot);
            return SUCCESS;
        }
        slot = slot->next_slot;
    }
    return -1;
}

static int search_and_set_channel_by_id(int id)
{
    struct Channel *channel = get_slot_channel_ll_head();
    while (channel != NULL)
    {
        if (channel->channel_id == id)
        {
            set_current_channel(channel);
            return SUCCESS;
        }
        channel = channel->next;
    }
    return -1;
}

//================== GETTERS & SETTERS ===========================

static struct Slot *get_current_slot(void)
{
    return current_slot;
}

static void set_current_slot(struct Slot *slot)
{
    current_slot = slot;
}

static struct Slot *get_slot_ll_head(void)
{
    return slot_linked_list_head;
}

static void set_slot_ll_head(struct Slot *slot)
{
    slot_linked_list_head = slot;
}

static int get_current_slot_minor(void)
{
    return get_current_slot()->minor;
}

static void set_current_slot_minor(int minor)
{
    get_current_slot()->minor = minor;
}

static struct Slot *get_next_slot(void)
{
    return get_current_slot()->next_slot;
}

static void set_next_slot(struct Slot *slot)
{
    get_current_slot()->next_slot = slot;
}

static struct Channel *get_current_channel(void)
{
    return get_current_slot()->current_channel;
}

static void set_current_channel(struct Channel *channel)
{
    get_current_slot()->current_channel = channel;
}

static u_int32_t get_channel_count(void)
{
    return get_current_slot()->channel_count;
}

static void set_channel_count(u_int32_t count)
{
    get_current_slot()->channel_count = count;
}

static struct Channel *get_slot_channel_ll_head(void)
{
    return get_current_slot()->channel_linked_list_head;
}

static void set_slot_channel_ll_head(struct Channel *channel)
{
    get_current_slot()->channel_linked_list_head = channel;
}

static char *get_current_message(void)
{
    return get_current_channel()->message;
}

static void allocate_current_message(size_t size)
{
    struct Channel *current_channel = get_current_channel();
    current_channel->message = (char *)kmalloc(size, GFP_KERNEL);
}

static void reset_current_message(void)
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
    get_current_channel()->message_length = length;
}

static int get_current_channel_id(void)
{
    return get_current_channel()->channel_id;
}

static int get_channel_id(struct Channel *channel)
{
    return channel->channel_id;
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