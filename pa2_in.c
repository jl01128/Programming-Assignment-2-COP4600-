#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/export.h>

#include "shared_data.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("PA2 input kernel module");
MODULE_VERSION("0.1");

struct shared_data shared_memory;
EXPORT_SYMBOL(shared_memory);

static int pa2_in_major_number;
static struct class *pa2_in_class = NULL;
static struct cdev pa2_in_cdev;

ssize_t pa2_in_write(struct file *file, const char __user *buf, size_t len, loff_t *ppos) {
    ssize_t bytes_written = 0;

    printk(KERN_INFO "pa2_in: Waiting on the lock\n");
    mutex_lock(&shared_memory.buffer_mutex);
    printk(KERN_INFO "pa2_in: Acquiring the lock\n");

    printk(KERN_INFO "pa2_in: In the Critical Section\n");

    // Write data to the shared buffer
    for (bytes_written = 0; bytes_written < len; ++bytes_written) {
        if (shared_memory.write_pos == BUFFER_SIZE) {
            printk(KERN_INFO "pa2_in: Buffer is full\n");
            break;
        }

        if (copy_from_user(&shared_memory.buffer[shared_memory.write_pos], &buf[bytes_written], 1)) {
            printk(KERN_ERR "pa2_in: Failed to write\n");
            break;
        }
        shared_memory.write_pos = (shared_memory.write_pos + 1) % BUFFER_SIZE;
    }

    if (bytes_written < len) {
        printk(KERN_INFO "pa2_in: Not enough space left in the buffer, dropping the rest\n");
    }

    printk(KERN_INFO "pa2_in: Writing %zu bytes\n", bytes_written);

    printk(KERN_INFO "pa2_in: Releasing the lock\n");
    mutex_unlock(&shared_memory.buffer_mutex);
    return bytes_written;
}

static struct file_operations pa2_in_fops = {
    .write = pa2_in_write,
};

static int __init pa2_in_init(void) {
    printk(KERN_INFO "pa2_in: Initializing the pa2_in module\n");

    shared_memory.read_pos = 0;
    shared_memory.write_pos = 0;
    mutex_init(&shared_memory.buffer_mutex);

    pa2_in_major_number = register_chrdev(0, "pa2_in", &pa2_in_fops);
    if (pa2_in_major_number < 0) {
        printk(KERN_ERR "pa2_in: Failed to register a major number\n");
        return pa2_in_major_number;
    }

    pa2_in_class = class_create(THIS_MODULE, "pa2_in_class");
    if (IS_ERR(pa2_in_class)) {
        unregister_chrdev(pa2_in_major_number, "pa2_in");
        printk(KERN_ERR "pa2_in: Failed to register device class\n");
        return PTR_ERR(pa2_in_class);
    }

    device_create(pa2_in_class, NULL, MKDEV(pa2_in_major_number, 0), NULL, "pa2_in");

    cdev_init(&pa2_in_cdev, &pa2_in_fops);
    pa2_in_cdev.owner = THIS_MODULE;

    if (cdev_add(&pa2_in_cdev, MKDEV(pa2_in_major_number, 0), 1) < 0) {
        printk(KERN_ERR "pa2_in: Failed to add character device\n");
        device_destroy(pa2_in_class, MKDEV(pa2_in_major_number, 0));
        class_unregister(pa2_in_class);
        class_destroy(pa2_in_class);
        unregister_chrdev(pa2_in_major_number, "pa2_in");
        return -1;
    }

    return 0;
}

static void __exit pa2_in_exit(void) {
    printk(KERN_INFO "pa2_in: Exiting the pa2_in module\n");

    cdev_del(&pa2_in_cdev);
    device_destroy(pa2_in_class, MKDEV(pa2_in_major_number, 0));
    class_unregister(pa2_in_class);
    class_destroy(pa2_in_class);
    unregister_chrdev(pa2_in_major_number, "pa2_in");
    mutex_destroy(&shared_memory.buffer_mutex);
}

module_init(pa2_in_init);
module_exit(pa2_in_exit);


