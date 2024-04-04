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
#define BUFFER_SIZE 1024
#define MINOR_START 0
#define DEVICE_COUNT 2
#define DM510_IOC_MAGIC 'k'
#define DM510_IOCRESET _IO(DM510_IOC_MAGIC, 0)
#define DM510_IOCSQUANTUM _IOW(DM510_IOC_MAGIC, 1, int)

static int dm510_major = 0;
module_param(dm510_major, int, S_IRUGO);

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

    printk(KERN_INFO "DM510: Attempting to open device\n");


    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;
    switch (filp->f_flags & O_ACCMODE) {
	    // Will be denind writing acces, becues device is busy
        case O_WRONLY:
            if (dev->nwriters) {
                printk(KERN_INFO "DM510: Device is busy, write access denied\n");
                up(&dev->sem);
                return -EBUSY;
            }
            dev->nwriters++;
            break;
        case O_RDONLY:
		// Will be dening access, becues there are to many readers
            if (dev->nreaders >= dev->max_processes) {
                printk(KERN_INFO "DM510: Too many readers, access denied\n");
                up(&dev->sem);
                return -EMFILE;
            }
            dev->nreaders++;
            break;
        case O_RDWR:
		// Will be denine read/write access becuse device is busy
            if (dev->nwriters) {
                printk(KERN_INFO "DM510: Device is busy, read/write access denied\n");
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
    printk(KERN_INFO "DM510: Device opened successfully\n");

    up(&dev->sem);
    return 0;
}


static int dm510_release(struct inode *inode, struct file *filp) {
    struct dm510_device *dev = filp->private_data;

    printk(KERN_INFO "DM510: Releasing device\n");

    down(&dev->sem);
    // Handle the release of writers properly
    if ((filp->f_flags & O_ACCMODE) == O_WRONLY || (filp->f_flags & O_ACCMODE) == O_RDWR)
        dev->nwriters--;
    // Handle the release of readers properly
    if ((filp->f_flags & O_ACCMODE) == O_RDONLY || (filp->f_flags & O_ACCMODE) == O_RDWR)
        dev->nreaders--;
    printk(KERN_INFO "DM510: Device released, readers: %d, writers: %d\n", dev->nreaders, dev->nwriters);    
    up(&dev->sem);
    return 0;
}


ssize_t dm510_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
    struct dm510_device *dev = filp->private_data;
    ssize_t result = 0;
    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;
    while (dev->head == dev->tail) { 
        up(&dev->sem);
        if (filp->f_flags & O_NONBLOCK){
	    printk(KERN_WARNING "dm510_read: Operation would block, returning -EAGAIN\n");
	    return -EAGAIN;
	}
        if (wait_event_interruptible(dev->read_queue, dev->head != dev->tail))
            return -ERESTARTSYS; 
        if (down_interruptible(&dev->sem))
            return -ERESTARTSYS;
    }

    if (dev->head < dev->tail)
        count = min(count, (size_t)(dev->tail - dev->head));
    else 
        count = min(count, (size_t)(BUFFER_SIZE - dev->head));
    if (copy_to_user(buf, dev->data + dev->head, count)) {
	result = -EFAULT; // Error accessing user space buffer
        goto out;
    }
    dev->head = (dev->head + count) % BUFFER_SIZE; // Update head pointer
    wake_up_interruptible(&dev->write_queue); // Wake up any waiting writers

    result = count; // Return number of bytes read
    out:	
    up(&dev->sem);
    return result;
}

ssize_t dm510_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
    struct dm510_device *dev = filp->private_data;
    ssize_t result = 0;
    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;
    while ((dev->tail + 1) % BUFFER_SIZE == dev->head) { 
        up(&dev->sem); 
        if (filp->f_flags & O_NONBLOCK) {
	    printk(KERN_WARNING "dm510_read: Operation would block, returning -EAGAIN\n");
	    return -EAGAIN;
	}
        if (wait_event_interruptible(dev->write_queue, (dev->tail + 1) % BUFFER_SIZE != dev->head))
            return -ERESTARTSYS; 
        if (down_interruptible(&dev->sem))
            return -ERESTARTSYS;
    }
    count = min(count, (size_t)(BUFFER_SIZE - dev->tail));
    if (copy_from_user(dev->data + dev->tail, buf, count)) {
        result = -EFAULT;
        goto out;
    }
    dev->tail = (dev->tail + count) % BUFFER_SIZE;
    printk(KERN_INFO "dm510_write: %zu bytes written to device. New tail position: %d\n", count, dev->tail);
    wake_up_interruptible(&dev->read_queue);
    result = count;
out:
    up(&dev->sem);
    return result;
}

long dm510_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
    struct dm510_device *dev = filp->private_data;
    int new_size, retval = 0;
    switch (cmd) {
        case GET_BUFFER_SIZE:
            if (copy_to_user((int __user *)arg, &dev->buffer_size, sizeof(dev->buffer_size)))
                retval = -EFAULT;
            break;
	case SET_BUFFER_SIZE:
	    if (copy_from_user(&new_size, (int __user *)arg, sizeof(new_size))) {
        	printk(KERN_WARNING "DM510: Error copying buffer size from user.\n");
        	retval = -EFAULT;
	    } else if (new_size < 5) { // Ensure minimum buffer size of 5 bytes
        	retval = -EINVAL; // Invalid buffer size
	    } else {
        	char *new_buffer = kzalloc(new_size * sizeof(char), GFP_KERNEL);
        	if (!new_buffer) {
	            printk(KERN_WARNING "DM510: Memory allocation for new buffer failed.\n");
	            retval = -ENOMEM; // Out of memory
	        } else {
	            down(&dev->sem); // Ensure exclusive access to the buffer
	            //  printk(KERN_INFO "DM510: Current buffer size is %d bytes, new size is %d bytes.\n", dev->buffer_size, new_size);
            	    kfree(dev->data); // Free old buffer
            	    dev->data = new_buffer; // Assign new buffer
            	    dev->buffer_size = new_size; // Update buffer size
            	    dev->head = 0; // Reset pointers
            	    dev->tail = 0;
            	    up(&dev->sem); // Release the semaphore
            	    retval = 0; // Indicate success
        	}
    	    }
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
        buffer_init(&device[i]);
        dm510_setup_cdev(&device[i], i);
    }
    return 0;
}

static void __exit dm510_cleanup(void) {
    int i;
    for (i = 0; i < DEVICE_COUNT; ++i) {
        kfree(device[i].data);
        cdev_del(&device[i].cdev);
    }
    unregister_chrdev_region(MKDEV(dm510_major, MINOR_START), DEVICE_COUNT);
    printk(KERN_INFO "DM510: unregistered the devices\n");
}

module_init(dm510_init);
module_exit(dm510_cleanup);

MODULE_AUTHOR("Your Name");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("DM510 Assignment Device Driver");
