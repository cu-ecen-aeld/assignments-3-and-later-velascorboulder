#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/slab.h> // For kmalloc, kfree
#include <linux/mutex.h> // For mutex
#include "aesdchar.h"
#include "aesd-circular-buffer.h"

#define AESD_MAJOR 0
#define AESD_MINOR 0

MODULE_AUTHOR("Richard Velasco"); // TODO: Fill in your name
MODULE_LICENSE("Dual BSD/GPL");



static int aesd_major = AESD_MAJOR;
static int aesd_minor = AESD_MINOR;
static struct aesd_dev aesd_device;

static int aesd_open(struct inode *inode, struct file *filp) {
    struct aesd_dev *dev;

    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev; // Set private data

    return 0;
}

static int aesd_release(struct inode *inode, struct file *filp) {
    return 0;
}

static ssize_t aesd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
    struct aesd_dev *dev = filp->private_data;
    struct aesd_buffer_entry *entry;
    size_t entry_offset;
    ssize_t retval = 0;

    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;

    entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->circular_buffer, *f_pos, &entry_offset);
    if (entry) {
        size_t to_copy = min(count, entry->size - entry_offset);
        if (copy_to_user(buf, entry->buffptr + entry_offset, to_copy)) {
            retval = -EFAULT;
            goto out;
        }
        retval = to_copy;
        *f_pos += to_copy;
    }

out:
    mutex_unlock(&dev->lock);
    return retval;
}

static ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
    struct aesd_dev *dev = filp->private_data;
    ssize_t retval = 0;
    const char *newline_ptr;
    struct aesd_buffer_entry new_entry;
    char *new_write;
    size_t to_copy;

    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;

    if (dev->write_entry.size == 0) {
        dev->write_entry.buffptr = kmalloc(count, GFP_KERNEL);
        if (dev->write_entry.buffptr == NULL) {
            retval = -ENOMEM;
            goto out;
        }
        dev->write_entry.size = 0;
    } else {
        char *temp = krealloc(dev->write_entry.buffptr, dev->write_entry.size + count, GFP_KERNEL);
        if (temp == NULL) {
            retval = -ENOMEM;
            goto out;
        }
        dev->write_entry.buffptr = temp;
    }

    if (copy_from_user((void *)(dev->write_entry.buffptr + dev->write_entry.size), buf, count)) {
        retval = -EFAULT;
        goto out;
    }

    dev->write_entry.size += count;

    newline_ptr = memchr(dev->write_entry.buffptr, '\n', dev->write_entry.size);
    if (newline_ptr != NULL) {
        size_t new_entry_size = newline_ptr - dev->write_entry.buffptr + 1;
        new_write = kmalloc(new_entry_size, GFP_KERNEL);
        if (new_write == NULL) {
            retval = -ENOMEM;
            goto out;
        }

        memcpy(new_write, dev->write_entry.buffptr, new_entry_size);

        new_entry.buffptr = new_write;
        new_entry.size = new_entry_size;

        if (dev->circular_buffer.full) {
            kfree(dev->circular_buffer.entry[dev->circular_buffer.out_offs].buffptr);
        }
        aesd_circular_buffer_add_entry(&dev->circular_buffer, &new_entry);

        if (new_entry_size < dev->write_entry.size) {
            memmove(dev->write_entry.buffptr, dev->write_entry.buffptr + new_entry_size, dev->write_entry.size - new_entry_size);
            dev->write_entry.size -= new_entry_size;
        } else {
            kfree(dev->write_entry.buffptr);
            dev->write_entry.buffptr = NULL;
            dev->write_entry.size = 0;
        }
    }

    retval = count;

out:
    mutex_unlock(&dev->lock);
    return retval;
}

static loff_t aesd_llseek(struct file *filep, loff_t off, int whence) {
    struct aesd_dev *dev = filep->private_data;
    loff_t new_pos = 0;
    loff_t buffer_size;
    size_t entry_offset;
    
    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;

    // Calculate the total size of the circular buffer data
    buffer_size = 0;
    for (size_t i = 0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++) {
        buffer_size += dev->circular_buffer.entry[i].size;
    }

    switch (whence) {
    case SEEK_SET:
        new_pos = off;
        break;
    case SEEK_CUR:
        new_pos = filep->f_pos + off;
        break;
    case SEEK_END:
        new_pos = buffer_size + off;
        break;
    default:
        mutex_unlock(&dev->lock);
        return -EINVAL;
    }

    // Ensure the new position is within bounds
    if (new_pos < 0 || new_pos > buffer_size) {
        mutex_unlock(&dev->lock);
        return -EINVAL;
    }

    filep->f_pos = new_pos;
    mutex_unlock(&dev->lock);
    return new_pos;
}

long aesd_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct aesd_dev *dev = filp->private_data;
    struct aesd_seekto seekto;
    size_t entry_offset;
    loff_t new_pos = 0;
    loff_t buffer_pos = 0;
    int ret = 0;

    if (_IOC_TYPE(cmd) != AESD_IOC_MAGIC) return -ENOTTY;
    if (_IOC_NR(cmd) > AESDCHAR_IOC_MAXNR) return -ENOTTY;

    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;

    switch (cmd) {
    case AESDCHAR_IOCSEEKTO:
        if (copy_from_user(&seekto, (const void __user *)arg, sizeof(seekto))) {
            ret = -EFAULT;
            goto out;
        }

        // Validate command index
        if (seekto.write_cmd >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED ||
            dev->circular_buffer.entry[seekto.write_cmd].size == 0) {
            ret = -EINVAL;
            goto out;
        }

        // Validate offset
        if (seekto.write_cmd_offset >= dev->circular_buffer.entry[seekto.write_cmd].size) {
            ret = -EINVAL;
            goto out;
        }

        // Calculate the new position based on the command index and offset
        for (size_t i = 0; i < seekto.write_cmd; i++) {
            buffer_pos += dev->circular_buffer.entry[i].size;
        }
        new_pos = buffer_pos + seekto.write_cmd_offset;

        // Update the file position
        filp->f_pos = new_pos;
        ret = new_pos;
        break;

    default:
        ret = -ENOTTY;
        break;
    }

out:
    mutex_unlock(&dev->lock);
    return ret;
}

static struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .llseek =   aesd_llseek,
    .unlocked_ioctl = aesd_unlocked_ioctl,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev) {
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add(&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}

static int __init aesd_init_module(void) {
    dev_t dev = 0;
    int result;

    result = alloc_chrdev_region(&dev, aesd_minor, 1, "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }

    memset(&aesd_device, 0, sizeof(struct aesd_dev));
    aesd_circular_buffer_init(&aesd_device.circular_buffer);
    mutex_init(&aesd_device.lock);

    result = aesd_setup_cdev(&aesd_device);
    if (result) {
        unregister_chrdev_region(dev, 1);
    }

    return result;
}

static void __exit aesd_cleanup_module(void) {
    dev_t devno = MKDEV(aesd_major, aesd_minor);
    
    uint8_t index;
    struct aesd_buffer_entry *entry;

    cdev_del(&aesd_device.cdev);
    AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.circular_buffer, index) {
        kfree(entry->buffptr);
    }
    kfree(aesd_device.write_entry.buffptr);
    unregister_chrdev_region(devno, 1);
    mutex_destroy(&aesd_device.lock);
}

module_init(aesd_init_module);
module_exit(aesd_cleanup_module);

