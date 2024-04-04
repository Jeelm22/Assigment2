#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/module.h>
#include "ioctl_commands.h"

#define DEVICE_NAME "dm510_dev"
#define MINOR_START 0
#define DEVICE_COUNT 2
#define DM510_IOC_MAGIC 'k'
#define MIN_BUFFER_SIZE 512
#define MAX_BUFFER_SIZE (1024*1024) //1 MB

static int dm510_major;

struct dm510_device {
    struct cdev cdev;
    char *data; // Changed to a pointer to support dynamic resizing
    int buffer_size; // New field to keep track of the buffer size
    int head, tail;
    struct semaphore sem;
    wait_queue_head_t read_queue, write_queue;
    int nreaders, nwriters;
    int max_processes; // New field to limit the number of processes
};

static struct dm510_device device[DEVICE_COUNT];

static int dm510_open(struct inode *inode, struct file *filp) {
    struct dm510_device *dev;
    dev = container_of(inode->i_cdev, struct dm510_device, cdev);
    filp->private_data = dev;

    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;
    switch (filp->f_flags & O_ACCMODE) {
	    // Will be denind writing acces, becues device is busy
        case O_WRONLY:
            if (dev->nwriters) {
                up(&dev->sem);
                return -EBUSY;
            }
            dev->nwriters++;
            break;
        case O_RDONLY:
		// Will be dening access, becues there are to many readers
            if (dev->nreaders >= dev->max_processes) {
                up(&dev->sem);
                return -EMFILE;
            }
            dev->nreaders++;
            break;
        case O_RDWR:
		// Will be denine read/write access becuse device is busy
            if (dev->nwriters) {
                up(&dev->sem);
                return -EBUSY;
            }
            // This needs to check max_processes for readers as well
	    // Will be denine access becues there are too many readers
            if (dev->nreaders >= dev->max_processes) {
                up(&dev->sem);
                return -EMFILE;
            }
            dev->nwriters++;
            dev->nreaders++;
            break;
    }

    up(&dev->sem);
    return 0;
}


static int dm510_release(struct inode *inode, struct file *filp) {
    struct dm510_device *dev = filp->private_data;

    down(&dev->sem);
    // Handle the release of writers properly
    if ((filp->f_flags & O_ACCMODE) == O_WRONLY || (filp->f_flags & O_ACCMODE) == O_RDWR)
        dev->nwriters--;
    // Handle the release of readers properly
    if ((filp->f_flags & O_ACCMODE) == O_RDONLY || (filp->f_flags & O_ACCMODE) == O_RDWR)
        dev->nreaders--;
    
    up(&dev->sem);
    return 0;
}


static ssize_t dm510_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
    struct dm510_device *dev = filp->private_data;
    ssize_t result = 0;

    if (mutex_lock_interruptible(&dev->mutex))
        return -ERESTARTSYS;

    // Wait for data to be available
    while (dev->used == 0) {
        mutex_unlock(&dev->mutex); // Release lock while waiting
        if (filp->f_flags & O_NONBLOCK) {
            return -EAGAIN; // Non-blocking read
        }
        if (wait_event_interruptible(dev->read_queue, dev->used > 0)) {
            return -ERESTARTSYS; // Interrupted while waiting
        }
        if (mutex_lock_interruptible(&dev->mutex)) {
            return -ERESTARTSYS;
        }
    }

    // Calculate actual amount of data to read
    count = min(count, (size_t)dev->used);

    // Handle reading data with potential wrap-around in the circular buffer
    while (count > 0) {
        size_t chunk = min(count, (size_t)(dev->buffer_size - dev->head)); // Until end of buffer
        chunk = min(chunk, (size_t)dev->used); // Do not read more than available

        // Copy data to user space
        if (copy_to_user(buf, dev->data + dev->head, chunk)) {
            result = -EFAULT; // Failed to copy data to user space
            goto out;
        }

        dev->head = (dev->head + chunk) % dev->buffer_size;
        dev->used -= chunk;
        buf += chunk;
        count -= chunk;
        result += chunk;
    }

out:
    mutex_unlock(&dev->mutex);
    wake_up_interruptible(&dev->write_queue); // Wake up any waiting writers

    return result;
}

static ssize_t dm510_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
    struct dm510_device *dev = filp->private_data;
    ssize_t result = 0;

    if (mutex_lock_interruptible(&dev->mutex))
        return -ERESTARTSYS;

    // Wait for space to be available if the buffer is full
    while (dev->used == dev->buffer_size) {
        mutex_unlock(&dev->mutex); // Release lock while waiting
        if (filp->f_flags & O_NONBLOCK) {
            return -EAGAIN; // Non-blocking write
        }
        if (wait_event_interruptible(dev->write_queue, dev->used < dev->buffer_size)) {
            return -ERESTARTSYS; // Interrupted while waiting
        }
        if (mutex_lock_interruptible(&dev->mutex)) {
            return -ERESTARTSYS;
        }
    }

    // Calculate actual amount of data to write
    count = min(count, (size_t)(dev->buffer_size - dev->used));

    // Handle writing data with potential wrap-around in the circular buffer
    while (count > 0) {
        size_t chunk = min(count, (size_t)(dev->buffer_size - dev->tail)); // Until end of buffer
        chunk = min(chunk, (size_t)(dev->buffer_size - dev->used)); // Do not write more than available space

        // Copy data from user space
        if (copy_from_user(dev->data + dev->tail, buf, chunk)) {
            if (result == 0) {
                result = -EFAULT; // Only return an error if no data has been written yet
            }
            goto out;
        }

        dev->tail = (dev->tail + chunk) % dev->buffer_size;
        dev->used += chunk;
        buf += chunk;
        count -= chunk;
        result += chunk;
    }

out:
    mutex_unlock(&dev->mutex);
    wake_up_interruptible(&dev->read_queue); // Wake up any waiting readers

    return result;
}


static long dm510_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
    struct dm510_device *dev = filp->private_data;
    long retval = 0;
    int new_size;
    switch (cmd) {
        case GET_BUFFER_SIZE:
            if (copy_to_user((int __user *)arg, &dev->buffer_size, sizeof(dev->buffer_size)))
                retval = -EFAULT;
            break;
	    
	 case SET_BUFFER_SIZE:
            if (copy_from_user(&new_size, (int __user *)arg, sizeof(new_size)))
                return -EFAULT;

            if (new_size < MIN_BUFFER_SIZE || new_size > MAX_BUFFER_SIZE)
                return -EINVAL;  // Check if new_size is within the allowed range

            if (mutex_lock_interruptible(&dev->mutex))
                return -ERESTARTSYS;

            // Prevent buffer resizing if there's ongoing read/write operations
            if (dev->used > 0) {
                mutex_unlock(&dev->mutex);
                return -EBUSY;  // Cannot resize if buffer contains unread data
            }

            // Attempt to allocate a new buffer of the requested size
            char *new_buffer = kzalloc(new_size, GFP_KERNEL);
            if (!new_buffer) {
                mutex_unlock(&dev->mutex);
                return -ENOMEM;  // Memory allocation failed
            }

            // Free the old buffer and update device structure with new buffer
            kfree(dev->data);
            dev->data = new_buffer;
            dev->buffer_size = new_size;
            dev->head = 0;
            dev->tail = 0;
            dev->used = 0;  // Reset buffer usage since we're starting fresh

            mutex_unlock(&dev->mutex);
            break;

       case GET_MAX_NR_PROCESSES:
           if (copy_to_user((int __user *)arg, &dev->max_processes, sizeof(dev->max_processes))) {
               retval = -EFAULT;
	  } 
          break;

        case SET_MAX_NR_PROCESSES:
            // Updating max_processes from the value provided by user space
            if (copy_from_user(&dev->max_processes, (int __user *)arg, sizeof(dev->max_processes))) {
                retval = -EFAULT;
            }
            break;

        case GET_BUFFER_FREE_SPACE: { 
            int free_space;
            down(&dev->sem); // Ensure exclusive access
            if (dev->tail >= dev->head) {
                free_space = dev->buffer_size - (dev->tail - dev->head) - 1;
            } else {
                free_space = (dev->head - dev->tail) - 1;
            }
            up(&dev->sem);
            if (copy_to_user((int __user *)arg, &free_space, sizeof(free_space))) {
                retval = -EFAULT;
            }
            break;
	}

        case GET_BUFFER_USED_SPACE: {
            int used_space;
            down(&dev->sem); // Ensure exclusive access
            if (dev->tail >= dev->head) {
                used_space = dev->tail - dev->head;
            } else {
                used_space = dev->buffer_size - (dev->head - dev->tail);
            }
            up(&dev->sem);
            if (copy_to_user((int __user *)arg, &used_space, sizeof(used_space))) {
                retval = -EFAULT;
            }
	    break;
	}
                default:
                    retval = -ENOTTY;
	   }
    	   return retval;
}

static struct file_operations dm510_fops = {
    .owner = THIS_MODULE,
    .open = dm510_open,
    .release = dm510_release,
    .read = dm510_read,
    .write = dm510_write,
    .unlocked_ioctl = dm510_ioctl,
};

static void dm510_setup_cdev(struct dm510_device *dev, int index) {
    int err, devno = MKDEV(dm510_major, MINOR_START + index);
    cdev_init(&dev->cdev, &dm510_fops);
    dev->cdev.owner = THIS_MODULE;
    err = cdev_add(&dev->cdev, devno, 1);
    if (err)
        printk(KERN_NOTICE "Error %d adding DM510 device", err);
}

static void buffer_init(struct dm510_device *dev) {
    dev->data = kzalloc(BUFFER_SIZE * sizeof(char), GFP_KERNEL);
    dev->buffer_size = BUFFER_SIZE;
    memset(dev->data, 0, BUFFER_SIZE);
    dev->head = 0;
    dev->tail = 0;
    sema_init(&dev->sem, 1);
    init_waitqueue_head(&dev->read_queue);
    init_waitqueue_head(&dev->write_queue);
    dev->nreaders = 0;
    dev->nwriters = 0;
    dev->max_processes = 1; 
}

static int __init dm510_init(void) {
    int result, i;
    dev_t dev = 0;

    if (dm510_major) {
        dev = MKDEV(dm510_major, MINOR_START);
        result = register_chrdev_region(dev, DEVICE_COUNT, DEVICE_NAME);
    } else {
        result = alloc_chrdev_region(&dev, MINOR_START, DEVICE_COUNT, DEVICE_NAME);
        dm510_major = MAJOR(dev);
    }
    if (result < 0) {
        printk(KERN_WARNING "DM510: can't get major %d\n", dm510_major);
        return result;
    }

    for (i = 0; i < DEVICE_COUNT; ++i) {
        device[i].data = kzalloc(MAX_BUFFER_SIZE, GFP_KERNEL);
        if (!device[i].data) {
            unregister_chrdev_region(MKDEV(dm510_major, MINOR_START), DEVICE_COUNT);
            return -ENOMEM;
        }
        device[i].buffer_size = MAX_BUFFER_SIZE;
        mutex_init(&device[i].mutex);
        init_waitqueue_head(&device[i].read_queue);
        init_waitqueue_head(&device[i].write_queue);
        dm510_setup_cdev(&device[i], i);
    }
    printk(KERN_INFO "DM510: Registered with major number %d\n", dm510_major);
    return 0;
}

static void __exit dm510_cleanup(void) {
    int i;
    for (i = 0; i < DEVICE_COUNT; ++i) {
        if (device[i].data) kfree(device[i].data);
        cdev_del(&device[i].cdev);
    }
    unregister_chrdev_region(MKDEV(dm510_major, MINOR_START), DEVICE_COUNT);
    printk(KERN_INFO "DM510: Module unloaded\n");
}

module_init(dm510_init);
module_exit(dm510_cleanup);

MODULE_AUTHOR("Your Name");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("DM510 Assignment Device Driver");
