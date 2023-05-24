#include <linux/ioctl.h>
#include <linux/fs.h>

#define MAJOR_NUM 235 // hardcoded as per specs
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)
#define DEVICE_RANGE_NAME "message_slot"
#define BUF_LEN 128
#define DEVICE_FILE_NAME "ms_dev" // FIXME: update this guy if needed
#define SUCCESS 0

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

ssize_t read_buffer(char __user *buffer, char *message, int message_length);

ssize_t write_buffer(const char __user *buffer, size_t length);

int set_channel_and_check_write_validity(struct file *file, size_t length);

int set_channel_from_file(struct file *file);

int is_valid_write_length(int length);

static void clean_up_memory(void);

static struct Channel *get_current_channel(void);

static void set_current_channel(struct Channel *channel);

static u_int32_t get_channel_count(void);

static void set_channel_count(u_int32_t count);

static char *get_current_message(void);

static void allocate_current_message(size_t size);

static void *reset_current_message(void);

static int get_current_message_length(void);

static void set_current_message_length(int length);

static int get_current_channel_id(void);

static void set_current_channel_id(int id);

static struct Channel *get_next_channel(void);

static void set_next_channel(struct Channel *next_channel);
