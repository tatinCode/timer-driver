/**************************************************************
* File:: TimerDriver.c
*
* Description::
*    This is a linux device driver that implements a simple timer
*    logic. the user calls ioctl with a duration, and a timer is
*    then scheduled in kernel space. The read() call blocks until
*    the timer expires, then it returns a custom message. The user
*    can set a custom message via write() or just use the default one.
*    The message would also be reset to default each time the device
*    is opened again, to make sure it functions as intended.
*
**************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/vmalloc.h>  

#include <linux/uaccess.h>
#include <linux/wait.h>         //wait queues
#include <linux/sched.h>     
#include <linux/timer.h>        //for timer related functions
#include <linux/jiffies.h>      //for jiffies to ms conversion
#include <linux/workqueue.h>    //for workqueues
#include <linux/sched.h>

#define MY_MAJOR 415
#define MY_MINOR 0
#define DEVICE_NAME "TimerDriver"

#define MAX_MSG_LEN 256     //max length for user message
#define CMD_SET_TIMER 1     //ioctl command to set the timer

int major, minor;
char *kernel_buffer;

struct cdev my_cdev;
wait_queue_head_t workQ;        //supposed to be waitQ but driver breaks if I replace it to waitQ
int timer_expired =0;           //flag for singaling the timer's completion
struct delayed_work timerWork;  //the timer work structure

//the message to be sent to the user at the end
static char user_message[MAX_MSG_LEN] = "Timer expired!\n"; //default message

MODULE_AUTHOR("Justine Tenorio");
MODULE_DESCRIPTION("A simple timer driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");


struct timerData{
    int duration_ms;
};

struct timerData tData;

/**
 * Timer Callback. Runs in kernel thread.
 * called when the timer expires
 */
static void timer_callback(struct work_struct *timerWork){
    timer_expired = 1;
    wake_up_interruptible(&workQ);

    printk(KERN_INFO "[TIMER DRIVER] Timer expired. Waking up read().\n");
}

/**
 * This will block everything until the timer is expired, then returns a message
 * to the user.
 */
static ssize_t myRead(struct file *fs, char __user *buf, size_t count, loff_t *offset){
    // char *msg = "Timer Expired!\n";
    size_t len = strlen(user_message);

    printk(KERN_INFO "[TIMER DRIVER] Read called. Waiting for timer...\n");
    wait_event_interruptible(workQ, timer_expired);

    if(copy_to_user(buf, user_message, len)){
        return -1;
    }

    timer_expired = 0;  //resets it for next use, oops T_T
    return len;
}

/**
 * writes and sets a custom message to return after timer expires
 */
static ssize_t myWrite(struct file *fs, const char __user *buf, size_t count, loff_t *offset){
    if(count >= MAX_MSG_LEN){
        count = MAX_MSG_LEN - 1;    //truncates the message if too long
    }

    if(copy_from_user(user_message, buf, count)){
        return -1;  //failed to copy from user
    }

    user_message[count] = '\0';     //null-terminates the string
    printk(KERN_INFO "[TIMER DRIVER] Custom message set: %s\n", user_message);
    
    return count;
}


/**
 * Opening and closing logic
 */
static int myOpen(struct inode * inode, struct file *fs){
    printk(KERN_INFO "[TIMER DRIVER] Device opened.\n");

    //resets the message to the default one everytime the program is opened
    strncpy(user_message, "Timer Expired!\n", MAX_MSG_LEN);
    user_message[MAX_MSG_LEN - 1] = '\0';

    return 0;
}

static int myClose(struct inode * inode, struct file *fs){
    printk(KERN_INFO "[TIMER DRIVER] Device closed.\n");
    return 0;
}


/**
 * sets the timer using ioctl with CMD_SET_TIMER
 */
static long myIoCtl (struct file *fs, unsigned int command, unsigned long data){

    //early return if cmd set timer wasnt turned on
    if(command != CMD_SET_TIMER){
        printk(KERN_ERR);
        return -1;
    }

    int ms = (int)data;
    unsigned long delay = msecs_to_jiffies(ms);
    printk(KERN_INFO "[TIMER DRIVER] Setting timer for %d ms (%lu jiffies)\n", ms, delay);

    //schedules the timer
    schedule_delayed_work(&timerWork, delay);   //starts the timer

    return 0;
}

/**
 * another data structure
 */
struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = myOpen,
    .release = myClose,
    .read = myRead,
    .write = myWrite,
    .unlocked_ioctl = myIoCtl,
};

int module_init(void){
    int result;
    dev_t devno;

    init_waitqueue_head(&workQ);                        //set up the timer callbac
    INIT_DELAYED_WORK(&timerWork, timer_callback);      //initialize wait queue

    devno = MKDEV(MY_MAJOR, MY_MINOR);

    result = register_chrdev_region(devno, 1, DEVICE_NAME);
    if(result < 0){
        printk(KERN_ERR "[TIMER DRIVER] register_chrdev_region failed\n");
        return result;
    }

    cdev_init(&my_cdev, &fops);
    result = cdev_add(&my_cdev, devno, 1);
    my_cdev.owner = THIS_MODULE;

    if(result < 0){
        unregister_chrdev_region(devno, 1);
        printk(KERN_ERR "[TIMER DRIVER] cdev_add failed\n");
        return result;
    }

    printk(KERN_INFO "[TIMER DRIVER] Module loaded: %d\n", result);
    printk(KERN_INFO "Welcome - Timer Driver is loaded\n");
    // if(result < 0){
    //     printk(KERN_ERR "Register chardev failed : %d\n", result);
    // }

    return 0;
}

/**
 * unregistering and removing device from kernel
 */
void cleanup_module(void){
    dev_t devno;
    devno = MKDEV(MY_MAJOR, MY_MINOR);

    cancel_delayed_work_sync(&timerWork);       //cancel any pending timers
    cdev_del(&my_cdev);                         //remove character device
    unregister_chrdev_region(devno, 1);         //free device number

    printk(KERN_INFO "[TIMER DRIVER] Module unloaded.\n");
}

