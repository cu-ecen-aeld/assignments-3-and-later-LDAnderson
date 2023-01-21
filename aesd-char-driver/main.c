/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/slab.h>		/* kmalloc() */
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("L.D. Anderson");
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    /**
     * TODO: handle open
     */
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    int _strlen = 0;
    int k = 0;
    int i,j;
    char* return_str;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);

    if (mutex_lock_interruptible(&aesd_device.lock))
      return -ERESTARTSYS;
    if(aesd_device.len <= 0) {
      goto done;
    }

      for(i=aesd_device.pos;i<aesd_device.len;i++) {
      _strlen += strlen(aesd_device.list[i]);
    }

    return_str = kmalloc(_strlen, GFP_KERNEL);
    for(i = aesd_device.pos; i < aesd_device.len; i++) {
      for(j=0;j<strlen(aesd_device.list[i]); j++) {
        return_str[k++] = *aesd_device.list[j];
      }
    }

    if (copy_to_user(buf, return_str, count)) {
      retval = -EFAULT;
      goto done;
    }
    retval = _strlen;

    done:
      mutex_unlock(&aesd_device.lock);
      return retval;
    }

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
  int pos=aesd_device.pos;
  int len=aesd_device.len;
    ssize_t retval = -ENOMEM;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);

    if (mutex_lock_interruptible(&aesd_device.lock))
      return -ERESTARTSYS;

    if(aesd_device.len==10) {
      kfree(aesd_device.list[pos]);
      aesd_device.list[pos] = kmalloc(strlen(buf) * sizeof(char), GFP_KERNEL);
      copy_from_user(aesd_device.list[pos], buf, strlen(buf));
      (aesd_device.pos==9) ? aesd_device.pos = 0 : aesd_device.pos++;
    } else {

  aesd_device.list[len] = kmalloc(strlen(buf) * sizeof(char), GFP_KERNEL);
      copy_from_user(aesd_device.list[len], buf, strlen(buf));
      aesd_device.len++;
    }

 done:
    mutex_unlock(&aesd_device.lock);
    return retval;
}
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    // mutex
    mutex_init(&aesd_device.lock);

    // cdev
    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
