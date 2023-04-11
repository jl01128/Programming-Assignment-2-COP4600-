#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/export.h>

#define BUFFER_SIZE 1024

static dev_t my_dev;
static struct cdev my_cdev;
char *buffer;
int *current_size;
struct mutex *my_mutex;

static ssize_t pa2_in_write(struct file *file, const char __user *user_buffer, size_t count, loff_t *ppos) {
    int bytes_to_write;
    int bytes_written;

    if (mutex_lock_interruptible(my_mutex)) {
        return -ERESTARTSYS;
    }

    bytes_to_write = min(count, (size_t)(BUFFER_SIZE - *current_size));

    if (copy_from_user(&buffer[*current_size], user_buffer, bytes_to_write)) {
        mutex_unlock(my_mutex);
        return -EFAULT;
    }

    bytes_written = bytes_to_write;
    *current_size += bytes_written;

    printk(KERN_INFO "pa2_in: Wrote %d bytes to the buffer\n", bytes_written);

    mutex_unlock(my_mutex);
    return bytes_written;
}

static struct file_operations pa2_in_fops = {
    .owner = THIS_MODULE,
    .write = pa2_in_write,
};

static int __init pa2_in_init(void) {
    int err;

    // Allocate major number dynamically
    err = alloc_chrdev_region(&my_dev, 0, 1, "pa2_in");
    if (err) {
        printk(KERN_ALERT "pa2_in: Failed to allocate major number\n");
        return err;
    }

    // Initialize the buffer, current_size, and my_mutex pointers
    buffer = kmalloc(BUFFER_SIZE * sizeof(char), GFP_KERNEL);
    if (!buffer) {
        printk(KERN_ALERT "pa2_in: Failed to allocate buffer\n");
        unregister_chrdev_region(my_dev, 1);
        return -ENOMEM;
    }

    current_size = kmalloc(sizeof(int), GFP_KERNEL);
    if (!current_size) {
        printk(KERN_ALERT "pa2_in: Failed to allocate current_size\n");
        kfree(buffer);
        unregister_chrdev_region(my_dev, 1);
        return -ENOMEM;
    }
    *current_size = 0;

    my_mutex = kmalloc(sizeof(struct mutex), GFP_KERNEL);
    if (!my_mutex) {
        printk(KERN_ALERT "pa2_in: Failed to allocate my_mutex\n");
        kfree(buffer);
        kfree(current_size);
        unregister_chrdev_region(my_dev, 1);
        return -ENOMEM;
    }
    mutex_init(my_mutex);

    // Initialize the cdev structure
    cdev_init(&my_cdev, &pa2_in_fops);
    my_cdev.owner = THIS_MODULE;

    // Add the cdev to the kernel
    err = cdev_add(&my_cdev, my_dev, 1);
    if (err) {
        printk(KERN_ALERT "pa2_in: Failed to add cdev\n");
        kfree(buffer);
        kfree(current_size);
        kfree(my_mutex);
        unregister_chrdev_region(my_dev, 1);
        return err;
    }

    printk(KERN_INFO "pa2_in: Module initialized successfully %d\n", MAJOR(my_dev));
    return 0;
}

static void __exit pa2_in_exit(void) {
    cdev_del(&my_cdev);
    kfree(buffer);
    kfree(current_size);
    kfree(my_mutex);
    unregister_chrdev_region(my_dev, 1);
    printk(KERN_INFO "pa2_in: Module unloaded successfully\n");
}

module_init(pa2_in_init);
module_exit(pa2_in_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("PA2 Input Module");
MODULE_VERSION("1.0");


