#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

char *buffer;
int *current_size;
struct mutex *my_mutex;

static dev_t my_dev;
static struct cdev my_cdev;

static ssize_t pa2_out_read(struct file *file, char __user *user_buffer, size_t count, loff_t *ppos) {
    int bytes_to_read;
    int bytes_read;

    if (mutex_lock_interruptible(my_mutex)) {
        return -ERESTARTSYS;
    }

    bytes_to_read = min(count, (size_t)*current_size);

    if (copy_to_user(user_buffer, buffer, bytes_to_read)) {
        mutex_unlock(my_mutex);
        return -EFAULT;
    }

    bytes_read = bytes_to_read;
    memmove(buffer, &buffer[bytes_read], *current_size - bytes_read);
    *current_size -= bytes_read;

    printk(KERN_INFO "pa2_out: Read %d bytes from the buffer\n", bytes_read);

    mutex_unlock(my_mutex);
    return bytes_read;
}

static struct file_operations pa2_out_fops = {
    .owner = THIS_MODULE,
    .read = pa2_out_read,
};

static int __init pa2_out_init(void) {
    int err;

    // Allocate major number dynamically
    err = alloc_chrdev_region(&my_dev, 0, 1, "pa2_out");
    if (err) {
        printk(KERN_ALERT "pa2_out: Failed to allocate major number\n");
        return err;
    }

    // Initialize the cdev structure
    cdev_init(&my_cdev, &pa2_out_fops);
    my_cdev.owner = THIS_MODULE;

    // Add the cdev to the kernel
    err = cdev_add(&my_cdev, my_dev, 1);
    if (err) {
        printk(KERN_ALERT "pa2_out: Failed to add cdev\n");
        unregister_chrdev_region(my_dev, 1);
        return err;
    }

    printk(KERN_INFO "pa2_out: Module initialized successfully %d\n", MAJOR(my_dev));
    return 0;
}

static void __exit pa2_out_exit(void) {
    cdev_del(&my_cdev);
    unregister_chrdev_region(my_dev, 1);
    printk(KERN_INFO "pa2_out: Module unloaded successfully\n");
}

module_init(pa2_out_init);
module_exit(pa2_out_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("PA2 Output Module");
MODULE_VERSION("1.0");

