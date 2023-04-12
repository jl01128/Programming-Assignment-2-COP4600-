#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include "shared_data.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("PA2 output kernel module");
MODULE_VERSION("0.1");

static int pa2_out_major_number;
static struct class *pa2_out_class = NULL;
static struct cdev pa2_out_cdev;

ssize_t pa2_out_read(struct file *file, char __user *buf, size_t len, loff_t *ppos) {
    ssize_t bytes_read = 0;

    printk(KERN_INFO "pa2_out: Waiting on the lock\n");
    mutex_lock(&shared_memory.buffer_mutex);
    printk(KERN_INFO "pa2_out: Acquiring the lock\n");

    printk(KERN_INFO "pa2_out: In the Critical Section\n");

    // Read data from the shared buffer
    for (bytes_read = 0; bytes_read < len; ++bytes_read) {
        if (shared_memory.read_pos == shared_memory.write_pos) {
            printk(KERN_INFO "pa2_out: Buffer is empty\n");
            break;
        }

        if (copy_to_user(&buf[bytes_read], &shared_memory.buffer[shared_memory.read_pos], 1)) {
            printk(KERN_ERR "pa2_out: Failed to read\n");
            break;
        }
        shared_memory.read_pos = (shared_memory.read_pos + 1) % BUFFER_SIZE;
    }

    printk(KERN_INFO "pa2_out: Reading %zu bytes\n", bytes_read);

    printk(KERN_INFO "pa2_out: Releasing the lock\n");
    mutex_unlock(&shared_memory.buffer_mutex);
    return bytes_read;
}

static struct file_operations pa2_out_fops = {
    .read = pa2_out_read,
};

static int __init pa2_out_init(void) {
    printk(KERN_INFO "pa2_out: Initializing the pa2_out module\n");

    pa2_out_major_number = register_chrdev(0, "pa2_out", &pa2_out_fops);
    if (pa2_out_major_number < 0) {
        printk(KERN_ERR "pa2_out: Failed to register a major number\n");
        return pa2_out_major_number;
    }

    pa2_out_class = class_create(THIS_MODULE, "pa2_out_class");
    if (IS_ERR(pa2_out_class)) {
        unregister_chrdev(pa2_out_major_number, "pa2_out");
        printk(KERN_ERR "pa2_out: Failed to register device class\n");
        return PTR_ERR(pa2_out_class);
    }

    device_create(pa2_out_class, NULL, MKDEV(pa2_out_major_number, 0), NULL, "pa2_out");

    cdev_init(&pa2_out_cdev, &pa2_out_fops);
    pa2_out_cdev.owner = THIS_MODULE;

    if (cdev_add(&pa2_out_cdev, MKDEV(pa2_out_major_number, 0), 1) < 0) {
        printk(KERN_ERR "pa2_out: Failed to add character device\n");
        device_destroy(pa2_out_class, MKDEV(pa2_out_major_number, 0));
        class_unregister(pa2_out_class);
        class_destroy(pa2_out_class);
        unregister_chrdev(pa2_out_major_number, "pa2_out");
        return -1;
    }

    return 0;
}

static void __exit pa2_out_exit(void) {
    printk(KERN_INFO "pa2_out: Exiting the pa2_out module\n");

    cdev_del(&pa2_out_cdev);
    device_destroy(pa2_out_class, MKDEV(pa2_out_major_number, 0));
    class_unregister(pa2_out_class);
    class_destroy(pa2_out_class);
    unregister_chrdev(pa2_out_major_number, "pa2_out");
}

module_init(pa2_out_init);
module_exit(pa2_out_exit);


