/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

/* ---includes for libs and headers--- */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h>      /*file_operations*/
#include "aesdchar.h"
#include <linux/slab.h>    /*kmalloc(), kfree()*/
#include <linux/uaccess.h> /*copy to and from usr space*/
#include <linux/string.h>  /*used for memcpy*/

/*Numerical ID to link files with specific device / driver*/
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Ruben Moreno"); /** Updated with author name **/
MODULE_LICENSE("Dual BSD/GPL"); 

/*global aesd_device*/
struct aesd_dev aesd_device;

/*
 * @brief 1st file operation performed on a dev file. Init in prep for future ops
 *        opens the char dev and inits the file session. 
 *
 * @param inode pointer to a inode struct rep. the device file. Desired info is
 *        contained in the i_cdev field.
 * @param filp pointer to the specific open fd session. Will be used to store the
 *        aesd_dev pointer for easier access in the future *
 * @return 0 on success
 */
int aesd_open(struct inode *inode, struct file *filp)
{
    /*device structure that contains meta data*/
    struct aesd_dev *dev;

    /*debug print, enabled in aesdchar.h*/
    PDEBUG("open");
    
    /*This performs the pointer arithmetic operations to take a pointer struct
     * of type inode and dereference it's cdev structure to arrive at the 
     * start of the aesd struct*/
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);

    /*assign the aesd_dev ptr to the private data member of the filp for easy
     * access*/
    filp->private_data = dev; 

    return 0;
}

/*
 * @brief Performs the reverse of open.
 *
 * @param inode pointer to a inode struct rep.
 * @param filp pointer to the specific open fd session. 
 * @return 0 on success
 */
int aesd_release(struct inode *inode, struct file *filp)
{
    /*print debug for release function*/
    PDEBUG("release");
    /**
     * Nothing to deallocate since aesd does not have hw to shutdown.
     */
    return 0;
}


/*
 * @brief Performs the device driver read operation.
 *
 * @param filp pointer to a file struct.
 * @param char __user *, app allocated buffer
 * @param size_t count, size / num bytes of datat to be transferred
 * @param long offset type pointer for file position that the user is accessing. 
 * @return num of bytes read on success, negative on fail, check errno code
 */
ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    /*declare and init vars to assist with cb read*/
    struct aesd_dev *dev = filp->private_data;
    struct aesd_buffer_entry *entry;           /*current entry*/
    size_t char_offset;                        /*val set when matching char_off is found*/
    ssize_t retval = 0;                        /*return code for functions*/

    /*print debug for num bytes to read with offset*/
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    

    /*Acquire lock, interruptible variation*/
    if(mutex_lock_interruptible(&dev->lock)){
	    return -ERESTARTSYS;  /*return err code on fail*/
    }

    /*try to find the matching char_offset*/
    entry = aesd_circular_buffer_find_entry_offset(&dev->cb, *f_pos,
		    &char_offset);
    
    /*check if the char_offset was found and if corresponding entry was returned*/
    if(entry == NULL){
	retval = 0; /*not found and nothing to return*/
    	goto out;
    }

    /*if offset found, read only up to the end of the entry*/
    if(count > entry->size - char_offset){
	    count = entry->size - char_offset;    
    }

    /*copy data read into userspace buffer*/
    if(copy_to_user(buf, entry->buffptr + char_offset, count)){
	    retval = -EFAULT;
	    goto out;
    }

    /*update fpos*/
    *f_pos += count;
    retval = count;

out:
    mutex_unlock(&dev->lock);
    return retval;
}

/*
 * @brief Performs the device driver write operation.
 *
 * @param filp pointer to a file struct.
 * @param char __user *, app allocated buffer
 * @param size_t count, size / num bytes of datat to be transferred
 * @param long offset type pointer for file position that the user is accessing. 
 * @return num of bytes written on success, negative on fail, check errno code
 */
ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    /*declare and init vars to assist with cb write*/
    struct aesd_dev *dev = filp->private_data;
    struct aesd_buffer_entry entry;  /*current entry*/
    char * buffptr = NULL;           /*per the aesd_cb_add_entry funct we must manage mem allocation*/
    char * temp_ptr = NULL;          /*pointer used to hold string for modification*/
    size_t char_offset;              /*val set when matching char_off is found*/
    size_t size = count;             /*size of data to be written, init to count*/
    bool newline_term = false;       /*flag for if str is newline terminated*/
    ssize_t retval = -ENOMEM;        /*Err code for when kmalloc fails due to low mem*/

    /*debug for write function*/
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    
    /*Acquire lock, interruptible variation*/
    if(mutex_lock_interruptible(&dev->lock)){
	    return -ERESTARTSYS;  /*return err code on fail*/
    }

    /*malloc tmp_ptr*/
    temp_ptr = kmalloc(count, GFP_KERNEL);  /*sizeof(char) not needed, its 1 byte*/
    if(!temp_ptr){
	    goto out;    
    }
    memset(temp_ptr, 0, count);

    /*copy data from  userspace buffer into the temp ptr*/
    if(copy_from_user(temp_ptr, buf, count)){
	    retval = -EFAULT;
	    goto out;
    }

    /*successfully received count bytes from userspace*/
    retval = count;

    /*Writes will only occur on newline termination*/
    if(dev->partial_write == NULL && dev->partial_write_len == 0){
	    /*since no existing line to append to, simply assign tmp_ptr to buffptr*/
	    buffptr = temp_ptr; /*no need to apppend data*/
	    temp_ptr = NULL;    
    }else{
	    /*realloc and zero out new mem*/
	    size = count + dev->partial_write_len; 
	    buffptr = kmalloc(size, GFP_KERNEL);
	    if(!buffptr){
		    goto out;	    
	    }
	    memset(buffptr, 0, size);
	    
	    /*combine the two buffers into one*/
	    memcpy(buffptr, dev->partial_write, dev->partial_write_len);
	    memcpy(buffptr + dev->partial_write_len, temp_ptr, count);

	    /*free the temp_ptr since the data has been copied over*/
	    kfree(temp_ptr);
	    temp_ptr = NULL;
    }

    /*now check for the newline to determine whether to add entry into cb or update
     * the partial write in the aesd char struct*/
    char * result = memchr(buffptr, '\n', size);
    if(result != NULL){
	    newline_term = true;	        
    }

    /*now check whether to update the aesd struct partial write or write an entry to cb*/
    if(newline_term){

	    /*check if cb is full, if so free mem of current write ptr*/
	    if(dev->cb.full){
		    /*free curr write ptr*/
		    kfree(dev->cb.entry[dev->cb.in_offs].buffptr);
	    }
	    
	    /*populate entry and then add to cb*/
	    entry.buffptr = buffptr;
	    entry.size = size;
	    aesd_circular_buffer_add_entry(&dev->cb, &entry);

	    /*reset partial_write and len*/
	    kfree(dev->partial_write);
	    dev->partial_write = NULL;
	    dev->partial_write_len = 0;
    }
    else{
	    /*newline not found*/
	    kfree(dev->partial_write);
	    dev->partial_write = buffptr;
	    dev->partial_write_len = size;
    }

out:
    /* If an error happened before temp_ptr was cleared, this catches it. */
    /* If everything succeeded, temp_ptr is NULL, making this safe. */
    if (temp_ptr) {
        kfree(temp_ptr);
    }
    /*free lock*/
    mutex_unlock(&dev->lock);
    return retval;
}

/*aesd device driver, device methods*/
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

    /**
     * initialize the AESD specific portion of the device
     */
    aesd_circular_buffer_init(&aesd_device.cb);
    mutex_init(&aesd_device.lock);
    aesd_device.partial_write = NULL;
    aesd_device.partial_write_len = 0;

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
