/*
 * aesdchar.h
 *
 *  Created on: Oct 23, 2019
 *      Author: Dan Walkes
 */

#ifndef AESD_CHAR_DRIVER_AESDCHAR_H_
#define AESD_CHAR_DRIVER_AESDCHAR_H_

#define AESD_DEBUG 1  //Remove comment on this line to enable debug

#undef PDEBUG             /* undef it, just in case */
#ifdef AESD_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "aesdchar: " fmt, ## args)
#  else
     /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif

/* ---Includes and defines--- */
#include "aesd-circular-buffer.h"
#include <linux/mutex.h>
#include <linux/errno.h>
struct aesd_dev
{
    /**
     *Added structure(s) and locks needed to complete assignment requirements
     */
    struct aesd_circular_buffer cb; /*circular buff to hold aesd-buffer-entries*/
    struct mutex lock;              /*data locking mechanism*/
    char * partial_write;           /*unterminated message*/
    size_t partial_write_len;       /*len of unterminated message*/
    struct cdev cdev;               /* Char device structure      */
};


#endif /* AESD_CHAR_DRIVER_AESDCHAR_H_ */
