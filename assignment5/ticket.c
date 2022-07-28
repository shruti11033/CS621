#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>

//Devices to create
#ifndef TICKET_NDEVICES
#define TICKET_NDEVICES 1
#endif

//Defining a structure for ticket devices
//ticket_mutex - a mutex to protect the fields of this structure;
//cdev - character device structure.
// curr_ticket_num - each time an integer is read from this device it will get the next value in order
struct ticket_dev{
    struct mutex ticket_mutex;
    struct cdev cdev;
    int curr_ticket_num;
};

MODULE_AUTHOR("Eugene A. Shatokhin / Shruti Sharma");
MODULE_LICENSE("GPL");

#define TICKET_DEVICE_NAME "ticket"

//Setting params
static int ticket_ndevices = TICKET_NDEVICES;


static unsigned int ticket_major = 0;
static struct ticket_dev *ticket_devices = NULL;
static struct class *ticket_class = NULL;

//ticket open method
int ticket_open(struct inode *inode, struct file *filp)
{
    unsigned int mj = imajor(inode);
    unsigned int mn = iminor(inode);

    struct ticket_dev *dev = NULL;

    if (mj != ticket_major || mn < 0 || mn >= ticket_ndevices)
    {
        printk(KERN_WARNING "[target] "
                            "No device found with minor=%d and major=%d\n",
               mj, mn);
        return -ENODEV; //Device not found
    }

    //Storing a pointer to struct ticket_dev
    dev = &ticket_devices[mn];
    filp->private_data = dev;

    if (inode->i_cdev != &dev->cdev)
    {
        printk(KERN_WARNING "[target] open: internal error\n");
        return -ENODEV; //Device not found
    }

    return 0;
}

//Method for ticket release
int ticket_release(struct inode *inode, struct file *filp)
{
    return 0;
}

ssize_t
ticket_read(struct file *filp, char __user *buf, size_t count,
            loff_t *f_pos)
{
    struct ticket_dev *dev = (struct ticket_dev *)filp->private_data;
    ssize_t retval = 0;

    if (count != 4) return -EINVAL;
    if (mutex_lock_killable(&dev->ticket_mutex)) return -EINTR;
    
    if (copy_to_user(buf, &(dev->curr_ticket_num), count) != 0) {
        retval = -EINVAL;
    } else {
        retval = 4;
        dev->curr_ticket_num++;
    }

    mutex_unlock(&dev->ticket_mutex);
    return retval;
}

ssize_t
ticket_write(struct file *filp, const char __user *buf, size_t count,
             loff_t *f_pos)
{
    return -EINVAL;
}

loff_t
ticket_llseek(struct file *filp, loff_t off, int whence)
{
    return -EINVAL;
}

struct file_operations ticket_fops = {
    .owner = THIS_MODULE,
    .read = ticket_read,
    .write = ticket_write,
    .open = ticket_open,
    .release = ticket_release,
    .llseek = ticket_llseek,
};


/* Setup and register the device with specific index (the index is also
 * the minor number of the device).
 * Device class should be created beforehand.
 */
static int
ticket_construct_device(struct ticket_dev *dev, int minor,
                        struct class *class)
{
    int err = 0;
    dev_t devno = MKDEV(ticket_major, minor);
    struct device *device = NULL;

    BUG_ON(dev == NULL || class == NULL);

    //Memory is allocated when the device is opened initially
    mutex_init(&dev->ticket_mutex);

    cdev_init(&dev->cdev, &ticket_fops);
    dev->cdev.owner = THIS_MODULE;

    err = cdev_add(&dev->cdev, devno, 1);
    if (err)
    {
        printk(KERN_WARNING "[target] Error %d while trying to add %s%d",
               err, TICKET_DEVICE_NAME, minor);
        return err;
    }

    device = device_create(class, NULL, /* no parent device */
                           devno, NULL, /* no additional data */
                           TICKET_DEVICE_NAME "%d", minor);

    if (IS_ERR(device))
    {
        err = PTR_ERR(device);
        printk(KERN_WARNING "[target] Error %d while trying to create %s%d",
               err, TICKET_DEVICE_NAME, minor);
        cdev_del(&dev->cdev);
        return err;
    }
    return 0;
}

//Destroying device and freeing its buffer
static void
ticket_destroy_device(struct ticket_dev *dev, int minor,
                      struct class *class)
{
    BUG_ON(dev == NULL || class == NULL);
    device_destroy(class, MKDEV(ticket_major, minor));
    cdev_del(&dev->cdev);
    return;
}

static void
ticket_cleanup_module(int devices_to_destroy)
{
    int i;

    //Getting rid of character devices
    if (ticket_devices)
    {
        for (i = 0; i < devices_to_destroy; ++i)
        {
            ticket_destroy_device(&ticket_devices[i], i, ticket_class);
        }
        kfree(ticket_devices);
    }

    if (ticket_class)
        class_destroy(ticket_class);

    /* [NB] ticket_cleanup_module is never called if alloc_chrdev_region()
   * has failed. */
    unregister_chrdev_region(MKDEV(ticket_major, 0), ticket_ndevices);
    return;
}

static int __init
ticket_init_module(void)
{
    int err = 0;
    int i = 0;
    int devices_to_destroy = 0;
    dev_t dev = 0;

    if (ticket_ndevices <= 0)
    {
        printk(KERN_WARNING "[target] Invalid value of ticket_ndevices: %d\n",
               ticket_ndevices);
        err = -EINVAL;
        return err;
    }

    //Range of minor numbers (starting with 0) to work with
    err = alloc_chrdev_region(&dev, 0, ticket_ndevices, TICKET_DEVICE_NAME);
    if (err < 0)
    {
        printk(KERN_WARNING "[target] alloc_chrdev_region() failed\n");
        return err;
    }
    ticket_major = MAJOR(dev);

    //Creating device class
    ticket_class = class_create(THIS_MODULE, TICKET_DEVICE_NAME);
    if (IS_ERR(ticket_class))
    {
        err = PTR_ERR(ticket_class);
        goto fail;
    }

    //Allocating array of devices
    ticket_devices = (struct ticket_dev *)kzalloc(
        ticket_ndevices * sizeof(struct ticket_dev),
        GFP_KERNEL);
    if (ticket_devices == NULL)
    {
        err = -ENOMEM;
        goto fail;
    }

    //Constructing devices
    for (i = 0; i < ticket_ndevices; ++i)
    {
        err = ticket_construct_device(&ticket_devices[i], i, ticket_class);
        if (err)
        {
            devices_to_destroy = i;
            goto fail;
        }
    }
    return 0;

fail:
    ticket_cleanup_module(devices_to_destroy);
    return err;
}

static void __exit
ticket_exit_module(void)
{
    ticket_cleanup_module(ticket_ndevices);
    return;
}

module_init(ticket_init_module);
module_exit(ticket_exit_module);
