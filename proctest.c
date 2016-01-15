/*         Process experiment  - Linux kernel module part         */
/*             meryle swartz - July 2015                          */


////////////////////////////////////////////////////////////////////
/*                  Includes and Definitions                      */
////////////////////////////////////////////////////////////////////

#include <linux/init.h>            
#include <linux/module.h>         
#include <linux/kernel.h>        
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <asm/uaccess.h>	/* for put_user */
 
MODULE_LICENSE("GPL");         
MODULE_AUTHOR("Meryle");      
MODULE_DESCRIPTION("Process Experiment"); 
MODULE_VERSION("0.00001");   
 
#define SUCCESS 0
#define DEVICE_NAME "procdata"	/* Dev name as it appears in /proc/devices   */
#define BUF_LEN 128		/* Max length of the message from the device */

////////////////////////////////////////////////////////////////////
/*                   Function Prototypes                          */ 
////////////////////////////////////////////////////////////////////

int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);


////////////////////////////////////////////////////////////////////
/*                   File scoped globals                          */ 
////////////////////////////////////////////////////////////////////

static int Major;		/* Major number assigned to our device driver */
static int Device_Open = 0;	/* Is device open? */ 
static char msg[BUF_LEN];	/* The msg the device will give when asked */
static char *msg_Ptr;		/* to make things a little simpler */
static struct class *my_dev_class = NULL;
static struct device *device = NULL;

static struct file_operations fops = {
	//the internet says that .owner= THIS_MODULE is the correct modern way to do it
	//but if I only have this and not try_module_get(), it doesn't want to unload, even
	//if i delete the character device file 
	.owner = THIS_MODULE,
	.read = device_read,
	.open = device_open,
	.release = device_release
};
 
////////////////////////////////////////////////////////////////////
/*             Set up and Tear Down Functions                     */
////////////////////////////////////////////////////////////////////
static int __init test_init(void)
{
	int minor = 0;
	int err = 0;
	dev_t devno;

        Major = register_chrdev(0, DEVICE_NAME, &fops);

	if (Major < 0) {
	  printk(KERN_ALERT "Registering char device failed with %d\n", Major);
	  return Major;
	}
	devno = MKDEV(Major, minor);

	my_dev_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(my_dev_class)) {
		err = PTR_ERR(my_dev_class);
		return err;
	}

	device = device_create(my_dev_class, NULL, // no parent device 
		devno, NULL, // no additional data 
		DEVICE_NAME "%d", minor);

	if (IS_ERR(device)) {
		err = PTR_ERR(device);
		printk(KERN_WARNING "[target] Error %d while trying to create %s%d",
			err, DEVICE_NAME, minor);
		//cdev_del(&device->cdev);  //FIXME ? 
		return err;
	}

    	return 0;
}

/* unregister the device */
static void __exit test_exit(void)
{
	device_unregister(device);
	class_destroy(my_dev_class);
	unregister_chrdev(Major, DEVICE_NAME);
}



////////////////////////////////////////////////////////////////////
/*             File Operations Functions                          */ 
////////////////////////////////////////////////////////////////////

/* process data for as long as you care to look at it */
static int device_open(struct inode *inode, struct file *file)
{
	if (Device_Open)
		return -EBUSY;

	Device_Open = 1;

	try_module_get(THIS_MODULE);
	return SUCCESS;
}

// Called when a process closes the device file.
static int device_release(struct inode *inode, struct file *file)
{
	module_put(THIS_MODULE);
	Device_Open = 0;
	return 0;
}

// Called when a process, which already opened the dev file, attempts to read it
static ssize_t device_read(struct file *filp,	/* see include/linux/fs.h   */
			   char *buffer,	/* buffer to fill with data */
			   size_t length,	/* length of the buffer     */
			   loff_t * offset)
{
	// Number of bytes actually written to the buffer 
	int bytes_read = 0;
    
	// Magical process data
	struct task_struct *task;

	// Put the data into the buffer and send to the user
	for_each_process(task) {
	
		//split so we can format how we like in userspace app 
		sprintf(msg, 
			"%d|%s|%d|%.2lu:%.2lu:%.2lu:%.6lu|%p\n",
			task->pid,
			task->comm,
			task->parent->pid,
			(task->start_time.tv_sec / 3600) % (24),
			(task->start_time.tv_sec / 60),
			task->start_time.tv_sec % 60,
			task->start_time.tv_nsec / 1000,
			&task); //the instructions want the swap address but that 
				//doesnt' seem to be a thing anymore 
		length = strlen(msg);
		msg_Ptr = msg;

		while (length && *msg_Ptr) {
			put_user(*(msg_Ptr++), buffer++);
			length--;
			bytes_read++;
		}
	}

#ifdef DEBUG
	printk("leaving device_read() \n");
#endif
	return bytes_read;
}


module_init(test_init);
module_exit(test_exit); 

