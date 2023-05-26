#define MAJOR_NUM 235 // hardcoded as per specs
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)
#define DEVICE_RANGE_NAME "message_slot"
#define BUF_LEN 128
#define DEVICE_FILE_NAME "ms_dev" // FIXME: update this guy if needed
#define SUCCESS 0
#define UNDEFINED -1

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

static int append_slot_to_ll(int minor, struct Slot *tail);

ssize_t read_buffer(char __user *buffer, char *message, int message_length);

int set_channel_and_check_read_validity(struct file *file, int buffer_length);

int is_valid_read_length(int message_length, int buffer_length);

ssize_t write_buffer(const char __user *buffer, size_t length);

int set_channel_and_check_write_validity(struct file *file, size_t length);

int set_channel_from_file(struct file *file);

static int set_state(int minor, int id);

static int search_and_set_slot_by_minor(int minor);

static int search_and_set_channel_by_id(int id);

static struct Slot *get_slot_ll_head(void);

static void set_slot_ll_head(struct Slot *slot);

static int get_current_slot_minor(void);

static void set_current_slot_minor(int minor);

static struct Slot *get_next_slot(void);

static void *set_next_slot(struct Slot *slot);

int is_valid_write_length(int length);

static int is_valid_ioctl_op(unsigned int ioctl_command_id, unsigned int channel_id);

static int find_or_create_channel(unsigned int channel_id);

static int initialize_slot_channel_ll(unsigned int id);

static int find_or_append_channel_in_existing_ll(unsigned int channel_id);

static int append_channel_to_ll(unsigned int id, struct Channel *tail);

static void initialize_current_channel(unsigned int id);

static void write_channel_to_file(struct file *file, struct Channel *channel);

static int initialize_slot_ll_head(void);

static void clean_up_slots(void);

static void clean_up_channels();

static struct Channel *get_current_channel(void);

static void set_current_channel(struct Channel *channel);

static u_int32_t get_channel_count(void);

static void set_channel_count(u_int32_t count);

static struct Channel *get_slot_channel_ll_head();

static void set_slot_channel_ll_head(struct Channel *channel);

static char *get_current_message(void);

static void allocate_current_message(size_t size);

static void *reset_current_message(void);

static int get_current_message_length(void);

static void set_current_message_length(int length);

static int get_current_channel_id(void);

static int get_channel_id(struct Channel *channel);

static void set_current_channel_id(int id);

static struct Channel *get_next_channel(void);

static void set_next_channel(struct Channel *next_channel);
