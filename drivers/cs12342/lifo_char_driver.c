#include <linux/module.h>  
#include <linux/kernel.h> 
#include <linux/proc_fs.h> 
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/kstrtox.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/fs.h>

#include <linux/moduleparam.h>

#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/fcntl.h>
#include <linux/poll.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>

#include <asm/segment.h>
#include <linux/buffer_head.h>

#include<linux/mutex.h>


#include <linux/ioctl.h> 

#define LIFO_IOC_MAGIC  'D'

#define LIFO_IOCRESET    _IO(LIFO_IOC_MAGIC, 0)

#define LIFO_DATA_AVBL  _IOR(LIFO_IOC_MAGIC, 1,int)
#define LIFO_REF_COUNT  _IOR(LIFO_IOC_MAGIC, 2,int)


#define NO_OF_DEVICES 4

#define BUFFERSIZE (12)

#define FILENAME "/tmp/driver_rw_storage"

#define INVALID 0
#define READER 2
#define WRITER 3

#define MIN(a, b) ((a) < (b) ? (a) : (b))


// struct semaphore lck;   
static struct mutex lck;

DECLARE_WAIT_QUEUE_HEAD(waiting_readers);



struct file *main_file = NULL;
// loff_t main_file_offset = 0;

// char* temp_buffer;

char* stack_as_buffer;
volatile int stack_size = 1;

static struct class *LIFO_class = NULL;


struct LIFO_pipe {
        int my_type;
        int nr_users;
        struct cdev char_dev;                  /* Char device structure */
};


dev_t LIFO_char_dev;

static struct LIFO_pipe* LIFO_devices;


void init_file(void){
    main_file = filp_open(FILENAME, O_CREAT | O_RDWR | O_APPEND , 0666);

}



static int LIFO_open(struct inode *inode, struct file *filp)
{
	struct LIFO_pipe *lif_dev;

	lif_dev = container_of(inode->i_cdev, struct LIFO_pipe, char_dev);
	filp->private_data = lif_dev;


    mutex_lock(&lck);

    if(lif_dev->my_type == READER){
        if(!(filp->f_mode & FMODE_READ)){
            return -EINVAL;
        }
    }else if( lif_dev->my_type == WRITER ){
        if(!(filp->f_mode & FMODE_WRITE)){
            return -EINVAL;
        }
    }else{
        lif_dev ->nr_users += 1;
        if (filp->f_mode & FMODE_READ){
            lif_dev->my_type = READER;
        }else if(filp->f_mode & FMODE_WRITE){
            lif_dev->my_type = WRITER;
        }
    }

	mutex_unlock(&lck);

	return nonseekable_open(inode, filp);
}








static ssize_t LIFO_reader (struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos) 
{

    int retval;
    int buffer_offset ;
    size_t count_copy ;
    int gonna_read ;
    char eof;

    char *buffer ;
    struct LIFO_pipe *lifo_obj;



    if (!filp || !filp->private_data) {
        printk(KERN_ALERT "LIFO_reader: Invalid file object or private data\n");
        return -EINVAL;
    }

    if (!buf) {
        printk(KERN_ALERT "LIFO_reader: Invalid buffer pointer\n");
        return -EFAULT;
    }

    if (count == 0) {
        printk(KERN_INFO "LIFO_reader: No data to read\n");
        return 0;
    }

    lifo_obj = filp->private_data;


    if(stack_size == 1){
        eof = 5;  // ASCII code for CTRL-D

        printk(KERN_INFO "LIFO_reader: Nothing to read, returning EOF \n");

        if (copy_to_user(buf, &eof, 1)) {
            printk(KERN_ERR "LIFO_reader: Failed to copy data to user space\n");
            return -EFAULT;
        }

        return 1;
    }

    printk(KERN_INFO "LIFO_reader: my type %d\n", lifo_obj->my_type);


    if(lifo_obj->my_type != READER){
        printk(KERN_ERR "LIFO_reader: Invalid file operation\n");
        return -EINVAL;
    }

    buffer = kmalloc(count, GFP_KERNEL);
    if (!buffer) {
        printk(KERN_ALERT "Unable to allocate");
        return -ENOMEM;
    }

    

    buffer_offset = 0;

    retval= count;
    count_copy = count;

    while(count){
        
        printk(KERN_INFO "LIFO_reader: acquiring mutex lock\n");

        mutex_lock(&lck);
        


        while (stack_size <= 1) { 
            printk(KERN_INFO "LIFO_reader: released, nothing to read lock\n");
            mutex_unlock(&lck);

        
            if (wait_event_interruptible(waiting_readers, (stack_size > 1)))
                return -ERESTARTSYS;
                
            
            printk(KERN_INFO "LIFO_reader: Lock acquired after waiting\n");
            mutex_lock(&lck);
        }

        gonna_read =  MIN(count, stack_size - 1);
        printk(KERN_INFO "LIFO_reader: gonna_read = %d\n", gonna_read);

        stack_size -= gonna_read;
        printk(KERN_INFO "LIFO_reader: stack_size updated to %d\n", stack_size);

        // if(!main_file){
        //     printk(KERN_INFO "LIFO: Reinitialised file\n");
        //     init_file();
        // }

        // retval = vfs_read(main_file, buffer + buffer_offset, gonna_read , &main_file_offset);
        memcpy(buffer + buffer_offset, stack_as_buffer + stack_size, gonna_read);

        
        printk(KERN_INFO "LIFO_reader: buffer stats is %s\n", buffer);

        // main_file_offset-= gonna_read;
        printk(KERN_INFO "LIFO_reader: vfs_read returned %d bytes\n", retval);



        count -= gonna_read;
        buffer_offset += gonna_read;

        printk(KERN_INFO "LIFO_reader: releasing mutex lock\n");
        mutex_unlock(&lck);

    }

    printk(KERN_INFO "LIFO_reader: Finally buffer is %s\n", buffer);


    for(int i = 0; i < count_copy/2; i++){
        char temp1 = buffer[i];
        char temp2 = buffer[count_copy - 1 - i];
        buffer[i] = temp2;
        buffer[count_copy - 1 - i] = temp1;
    }

    printk(KERN_INFO "LIFO_reader: Reversed buffer is %s\n", buffer);


    if (copy_to_user(buf, buffer, count_copy)) {
        return -EFAULT;
    }

    kfree(buffer);

    return retval;

}


ssize_t	LIFO_writer(struct file *filp, const char __user *buf, size_t count, loff_t *ppos){

    struct LIFO_pipe *lifo_obj;
    int offset ;
    int signed_count;
    // int retval;
    char *new_stack_as_buffer;

    if (!filp || !filp->private_data) {
        printk(KERN_ALERT "LIFO_reader: Invalid file object or private data\n");
        return -EINVAL;
    }

    if (!buf) {
        printk(KERN_ALERT "LIFO_reader: Invalid buffer pointer\n");
        return -EFAULT;
    }

    if (count == 0) {
        printk(KERN_INFO "LIFO_reader: No data to read\n");
        return 0;
    }

    lifo_obj = filp->private_data;

    printk(KERN_NOTICE "LIFO_writer: my type %d\n", lifo_obj->my_type);


    if(lifo_obj->my_type != WRITER){
        printk(KERN_ERR "LIFO_writer: Invalid file operation\n");
        return -EINVAL;
    }


    printk(KERN_NOTICE "LIFO_writer: I was allowed to write\n");

    if(!stack_as_buffer){
        printk(KERN_DEBUG "LIFO_writer: Stack Buffer Initialised\n");
        stack_as_buffer = kmalloc(1, GFP_KERNEL);
    }

    offset = 0;
    signed_count = (int) count;



    printk(KERN_NOTICE "LIFO_writer: Started Writing Stack-Size:%d Count:%d \n", stack_size,signed_count);


    mutex_lock(&lck);


    new_stack_as_buffer = kmalloc(stack_size + count, GFP_KERNEL);

    if (!new_stack_as_buffer) {
        printk(KERN_ERR "Failed to allocate memory for new buffer\n");
        return -ENOMEM;
    }

    memcpy(new_stack_as_buffer, stack_as_buffer, stack_size);

    kfree(stack_as_buffer);

    stack_as_buffer = new_stack_as_buffer;


    if (copy_from_user(stack_as_buffer + stack_size, buf, count) != 0)
    {
            mutex_unlock(&lck);
            return -EFAULT;
    }


    stack_size += count;

    printk(KERN_NOTICE "LIFO_writer: Stack_Status :%s stack_size:%d \n", stack_as_buffer + 1,stack_size);


    mutex_unlock(&lck);

    wake_up_interruptible(&waiting_readers);

    printk(KERN_NOTICE "Exiting LIFO_writer\n");


    return count;

}


static unsigned int LIFO_poll(struct file *filp, poll_table *wait)
{
	struct LIFO_pipe *lifo_obj;
	unsigned int mask;

    printk(KERN_ALERT "LIFO_poll: Poll operation performed\n");    

    mask = POLLOUT | POLLWRNORM;
    lifo_obj = filp->private_data;

	if (stack_size > 1)
        printk(KERN_ALERT "LIFO_poll: Data available\n");    

		mask |= POLLIN | POLLRDNORM;
	return mask;
}

static int LIFO_release(struct inode *inode, struct file *filp)
{
	struct LIFO_pipe *lifo_obj;


    if (!filp || !filp->private_data) {
        printk(KERN_ALERT "LIFO_release: Invalid file object or private data\n");
        return -EINVAL;
    }
    

    lifo_obj = filp->private_data;


	mutex_lock(&lck);
    lifo_obj->nr_users -= 1;
    printk(KERN_ALERT "LIFO_release: Current users %d\n", lifo_obj->nr_users );

    if(lifo_obj->nr_users == 0){
        lifo_obj->my_type = INVALID;
    }

    mutex_unlock(&lck);
	return 0;
}


long LIFO_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{


	int retval;
    struct LIFO_pipe *lifo_obj;

    printk(KERN_ALERT "LIFO_ioctl: IOCTL performed" );


    if (!filp || !filp->private_data) {
        printk(KERN_ALERT "LIFO_reader: Invalid file object or private data\n");
        return -EINVAL;
    }
    

    lifo_obj = filp->private_data;
    retval = 0;
    
	if (_IOC_TYPE(cmd) != LIFO_IOC_MAGIC){
        printk(KERN_INFO "LIFO_IOC_MAGIC error \n");
        return -ENOTTY;
    }

    switch(cmd) {
       case LIFO_IOCRESET:
            printk(KERN_ALERT "LIFO_ioctl: Stack Reset" );
           stack_size = 1;
           break;

        case LIFO_DATA_AVBL:
            printk(KERN_ALERT "LIFO_ioctl: Data queried" );
            retval = __put_user(stack_size - 1, (int __user *)arg);

            break;
        case  LIFO_REF_COUNT:
            printk(KERN_ALERT "LIFO_ioctl: Users queried" );
            retval = __put_user(lifo_obj->nr_users, (int __user *)arg);
            break;


	  default: 
		return -ENOTTY;
    }

    return retval;

}


struct file_operations LIFO_proc_ops = {
    .owner = THIS_MODULE,
	.read =		LIFO_reader,
	.write =	LIFO_writer,
	.open =		LIFO_open,
    .llseek =	no_llseek,
	.poll =		LIFO_poll,
	.unlocked_ioctl = LIFO_ioctl,
	.release =	LIFO_release,
};


static int __init LIFO_init(void)
{

    if (alloc_chrdev_region(&LIFO_char_dev, 0, NO_OF_DEVICES, "LIFO_CHAR_DEVICE") < 0)
    {
        printk(KERN_ALERT "Unable to allocate major-MINor number\n");
        return -1;
    }

    LIFO_devices = kmalloc(NO_OF_DEVICES * sizeof(struct LIFO_pipe), GFP_KERNEL);
    
    if (LIFO_devices == NULL) {
		unregister_chrdev_region(LIFO_char_dev, NO_OF_DEVICES);
		return -1;
	}

    LIFO_class = class_create(THIS_MODULE, "LIFO_CHAR_DEVICE");


    for (size_t i = 0; i < NO_OF_DEVICES; i++)
    {
        cdev_init(&(LIFO_devices + i)->char_dev, &LIFO_proc_ops);
        (LIFO_devices + i)->char_dev.owner = THIS_MODULE;

        (LIFO_devices + i)->my_type = INVALID;
        (LIFO_devices + i)->nr_users = 0;

        if (cdev_add(&((LIFO_devices + i)->char_dev), LIFO_char_dev + i, 1) < 0)
        {
            printk(KERN_ALERT "Device not added");
            unregister_chrdev_region(LIFO_char_dev + i, 1);
            return -1;
        }

        device_create(LIFO_class, NULL, MKDEV(MAJOR(LIFO_char_dev), i), NULL, "LIFO_CHAR_DEVICE%ld", i);

    }


    



    init_waitqueue_head(&waiting_readers);
	mutex_init(&lck);


    // init_file();


    // if (IS_ERR(main_file)) {
    //     printk(KERN_ALERT "Failed to open file %s: %ld\n", FILENAME, PTR_ERR(main_file));
    //     return -1;
    // }

    stack_as_buffer = kmalloc(1, GFP_KERNEL);
    
    if (!stack_as_buffer) {
        printk(KERN_ALERT "Data not allocated for stack_as_buffer: %d \n", BUFFERSIZE);
        return -ENOMEM;
    }
    stack_as_buffer[0] = (char)4;

    stack_size = 1;

    
    // if (!temp_buffer) {
    //     printk(KERN_ALERT "Data not allocated for temp_buffer: %d \n", BUFFERSIZE);
    //     return -ENOMEM;
    // }

    // // Added so that I can easily printk these chunks;
    // temp_buffer[BUFFERSIZE] = '\0';

    printk(KERN_INFO "Lifo device added, major number given is %d\n",MAJOR(LIFO_char_dev));


    return 0;
}

static void __exit LIFO_exit(void)
{

    stack_size = 1;

    for (size_t i = 0; i < NO_OF_DEVICES; i++)
    {
        cdev_del(&(LIFO_devices + i)->char_dev);
        device_destroy(LIFO_class, MKDEV(MAJOR(LIFO_char_dev), i));

    }
    unregister_chrdev_region(LIFO_char_dev, NO_OF_DEVICES);


    // class_unregister(LIFO_class);
    class_destroy(LIFO_class);



    printk(KERN_INFO "Lifo Device says bye world!\n");
}

module_init(LIFO_init);
module_exit(LIFO_exit);

MODULE_AUTHOR("Divyansh Mittal");
MODULE_LICENSE("GPL");