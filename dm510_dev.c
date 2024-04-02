#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/semaphore.h>

#define DEVICE_NAME "dm510_dev"
#define BUFFER_SIZE 1024
#define MINOR_START 0
#define DEVICE_COUNT 2

static int dm510_major = 252;
module_param(dm510_major, int, S_IRUGO);

typedef struct {
    char data[BUFFER_SIZE];
    int head;
    int tail;
    size_t size;
    wait_queue_head_t read_queue;
    wait_queue_head_t write_queue;
    struct semaphore sem;
} circular_buffer;

static circular_buffer buffers[DEVICE_COUNT];
static struct buffer buffers[BUFFER_COUNT];
static size_T max_processes = 10;
static struct cdev dm510_cdev;

static int buffer_init(circular_buffer *buffer) {
    memset(buffer->data, 0, BUFFER_SIZE);
    buffer->head = 0;
    buffer->tail = 0;
    buffer->size = 0;
    init_waitqueue_head(&buffer->read_queue);
    init_waitqueue_head(&buffer->write_queue);
    sema_init(&buffer->sem, 1);
    return 0;
}

static int dm510_open(struct inode *inode, struct file *filp) {
    int minor = iminor(inode);
    if (minor < MINOR_START || minor >= MINOR_START + DEVICE_COUNT) {
        printk(KERN_ALERT "DM510: No device for minor %d\n", minor);
        return -ENODEV;
    }
    filp->private_data = &buffers[minor - MINOR_START];
    return 0;
}

static int dm510_release(struct inode *inode, struct file *filp) {
    return 0;
}

static ssize_t dm510_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
    circular_buffer *buffer = (circular_buffer *)filp->private_data;
    size_t to_copy, not_copied;

    if (down_interruptible(&buffer->sem))
        return -ERESTARTSYS;

    while (buffer->size == 0) {
        up(&buffer->sem);
        if (wait_event_interruptible(buffer->read_queue, buffer->size > 0))
            return -ERESTARTSYS;
        if (down_interruptible(&buffer->sem))
            return -ERESTARTSYS;
    }

    to_copy = min(count, buffer->size);
    not_copied = copy_to_user(buf, &buffer->data[buffer->tail], to_copy);

    buffer->tail = (buffer->tail + to_copy - not_copied) % BUFFER_SIZE;
    buffer->size -= to_copy - not_copied;

    up(&buffer->sem);
    wake_up_interruptible(&buffer->write_queue);

    return to_copy - not_copied;
}

static ssize_t dm510_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
    circular_buffer *buffer = (circular_buffer *)filp->private_data;
    size_t to_copy, not_copied;

    if (down_interruptible(&buffer->sem))
        return -ERESTARTSYS;

    while (BUFFER_SIZE - buffer->size == 0) {
        up(&buffer->sem);
        if (wait_event_interruptible(buffer->write_queue, BUFFER_SIZE - buffer->size > 0))
            return -ERESTARTSYS;
        if (down_interruptible(&buffer->sem))
            return -ERESTARTSYS;
    }

    to_copy = min(count, BUFFER_SIZE - buffer->size);
    not_copied = copy_from_user(&buffer->data[buffer->head], buf, to_copy);

    buffer->head = (buffer->head + to_copy - not_copied) % BUFFER_SIZE;
    buffer->size += to_copy - not_copied;

    up(&buffer->sem);
    wake_up_interruptible(&buffer->read_queue);

    return to_copy - not_copied;
}

static struct file_operations dm510_fops = {
    .owner = THIS_MODULE,
    .open = dm510_open,
    .release = dm510_release,
    .read = dm510_read,
    .write = dm510_write,
};

static int __init dm510_init_module(void) {
    int result;
    dev_t dev = 0;
    
    if (dm510_major) {
    dev_t dev = MKDEV(dm510_major, MINOR_START);
    result = register_chrdev_region(dev, DEVICE_COUNT, DEVICE_NAME);
} else {
    result = alloc_chrdev_region(&dev, MINOR_START, DEVICE_COUNT, DEVICE_NAME);
    dm510_major = MAJOR(dev);
}


    if (result < 0) {
        printk(KERN_WARNING "DM510: can't get major %d\n", dm510_major);
        return result;
    }

    // Initialize each buffer
    for (int i = 0; i < DEVICE_COUNT; ++i) {
        buffer_init(&buffers[i]);
    }

    // Initialize the cdev structure and add it to the kernel
    cdev_init(&dm510_cdev, &dm510_fops);
    dm510_cdev.owner = THIS_MODULE;
    result = cdev_add(&dm510_cdev, dev, DEVICE_COUNT);
    if (result) {
        printk(KERN_NOTICE "Error %d adding DM510 device", result);
        unregister_chrdev_region(dev, DEVICE_COUNT);
        return result;
    }

    printk(KERN_INFO "DM510: registered with major number %d\n", dm510_major);
    return 0;
}

static void __exit dm510_cleanup_module(void) {
    // Remove the character device
    cdev_del(&dm510_cdev);

    // Free the device numbers
    unregister_chrdev_region(MKDEV(dm510_major, MINOR_START), DEVICE_COUNT);

    printk(KERN_INFO "DM510: unregistered the device\n");
}

#define GET_BUFFER_SIZE 0 
#define SET_BUFFER_SIZE 1
#define GET_MAX_NR_PROCESSES 2
#define SET_MAX_NR_PROCESSES 3

long dm510_ioctl( 
    struct file *filp, 
    unsigned int cmd,   /* command passed from the user */
    unsigned long arg ) /* argument of the command */
{
	/* ioctl code belongs here */
    switch(cmd){
        case GET_BUFFER_SIZE:
        return buffer -> size;

        case SET_BUFFER_SIZE:{
            int i;
            for(i = 0; i < BUFFER_COUNT: i++){
                int used_space = buffers[i].size - free_space(buffers + i);
                if(used_space > arg){
                    return rerror(-EINVAL, "The buffer(%d) has %lu amount of used space, it cannot be reduced to size %lu", i, used_space, arg);
                }
                for(i = 0; i < BUFFER_COUNT ; i++){
                    resize(buffers + i, arg);
                }
            }
            break;

            case GET_MAX_NR_PROCESSES:
            return max_processes;

            case SET_MAX_NR_PROCESSES:
            max_processes = arg;
            break;

            case GET_Free_spACE:
            return free_space(buffers + arg);

            case GET_BUFFER_USED_SPACE:
            return buffers[arg].size - free_space(buffers + arg);
        }
    }

	//printk(KERN_INFO "DM510: ioctl called.\n");

	return 0; //has to be changed
}


module_init(dm510_init_module);
module_exit(dm510_cleanup_module);

MODULE_AUTHOR("Your Name Here");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("DM510 Assignment Device Driver");

