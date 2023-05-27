#define MAJOR_NUM 235 // hardcoded as per specs
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)
#define DEVICE_RANGE_NAME "message_slot"
#define BUF_LEN 128
#define DEVICE_FILE_NAME "ms_dev"
#define SUCCESS 0
#define UNDEFINED -1
#define EXIT_FAILURE 1
