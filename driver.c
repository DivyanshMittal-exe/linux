#include <linux/module.h>  /* Specifically, a module */
#include <linux/kernel.h>  /* We're doing kernel work */
#include <linux/proc_fs.h> /* Necessary because we use the proc fs */
// #include <linux/string.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/kstrtox.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/fs.h>

#include <linux/module.h>
#include <linux/moduleparam.h>

#include <linux/kernel.h>	/* printk(), MIN() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/proc_fs.h>
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/fcntl.h>
#include <linux/poll.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <asm/uaccess.h>

#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>

#include<linux/mutex.h>

#define NO_OF_DEVICES 4

#define BUFFERSIZE (12)

#define FILENAME "/tmp/driver_rw_storage"

#define READER 2

#define WRITER 3

#define MIN(a, b) ((a) < (b) ? (a) : (b))


// struct semaphore lck;   
static struct mutex lck;

wait_queue_head_t waiting_readers;



struct file *main_file = NULL;
loff_t main_file_offset = 0;

static struct class *LIFO_class = NULL;


struct LIFO_pipe {
        int my_type;
        struct cdev char_dev;                  /* Char device structure */
};


dev_t LIFO_char_dev;

static struct LIFO_pipe* LIFO_devices;




static int LIFO_open(struct inode *inode, struct file *filp)
{
	struct LIFO_pipe *lif_dev;

	lif_dev = container_of(inode->i_cdev, struct LIFO_pipe, char_dev);
	filp->private_data = lif_dev;


    mutex_lock(&lck);



	if (filp->f_mode & FMODE_READ){
        lif_dev->my_type = READER;
    }else if(filp->f_mode & FMODE_WRITE){
        lif_dev->my_type = WRITER;
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

    char *buffer = kmalloc(count, GFP_KERNEL);
    if (!buffer) {
        printk(KERN_ALERT "Unable to allocate");
        return -1;
    }

    buffer_offset = 0;


    count_copy = count;

    while(count){
        
        printk(KERN_INFO "LIFO_reader: acquiring mutex lock\n");

        mutex_lock(&lck);
        


        while (main_file_offset == 0) { 
            printk(KERN_INFO "LIFO_reader: released, nothing to read lock\n");
            mutex_unlock(&lck);

        
            if (wait_event_interruptible(waiting_readers, (main_file_offset != 0)))
                return -ERESTARTSYS;
                
            
            printk(KERN_INFO "LIFO_reader: Lock acquired after waiting\n");
            mutex_lock(&lck);
        }

        gonna_read =  MIN(count, main_file_offset);
        printk(KERN_INFO "LIFO_reader: gonna_read = %d\n", gonna_read);

        main_file_offset-= gonna_read;
        printk(KERN_INFO "LIFO_reader: main_file_offset updated to %lld\n", main_file_offset);

        retval = kernel_read(main_file, buffer + buffer_offset, gonna_read , &main_file_offset);
        
        printk(KERN_INFO "LIFO_reader: main_file_offset after reading to %lld\n", main_file_offset);
        printk(KERN_INFO "LIFO_reader: buffer_offset is %d\n", buffer_offset);
        printk(KERN_INFO "LIFO_reader: buffer stats is %s\n", buffer);

        main_file_offset-= gonna_read;
        printk(KERN_INFO "LIFO_reader: kernel_read returned %d bytes\n", retval);



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

    return count_copy;

}


ssize_t	LIFO_writer(struct file *filp, const char __user *buf, size_t count, loff_t *ppos){

    struct LIFO_pipe *lifo_obj;
    int offset ;
    ssize_t retval;


    lifo_obj = filp->private_data;

    offset = 0;
    retval = count;


    printk(KERN_INFO "LIFO_writer: Started Writing \n");

    mutex_lock(&lck);

    char* temp_buffer;

    temp_buffer = kmalloc(count, GFP_KERNEL);
    
    if (!temp_buffer) {
        printk(KERN_ALERT "Data not allocated for temp_buffer: %d \n", count);
        return -ENOMEM;
    }

    // temp_buffer[BUFFERSIZE] = '\0';

    if (copy_from_user(temp_buffer, buf, count) != 0)
    {
            mutex_unlock(&lck);
            return -EFAULT;
    }

    printk(KERN_INFO "LIFO_writer: Copied from user space: %s\n", temp_buffer);
    printk(KERN_INFO "LIFO_writer: %s\n", temp_buffer);
    
    retval = kernel_write(main_file, temp_buffer ,  count, &main_file_offset);

    printk(KERN_INFO "LIFO_writer: Written to main file: %zd\n", retval);

    printk(KERN_INFO "LIFO_writer: main_file_offset after writing to %lld\n", main_file_offset);

    mutex_unlock(&lck);

    wake_up_interruptible(&waiting_readers);

    printk(KERN_INFO "Exiting LIFO_writer\n");


    return retval;

}





struct file_operations LIFO_proc_ops = {
    .owner = THIS_MODULE,
	.read =		LIFO_reader,
	.write =	LIFO_writer,
	.open =		LIFO_open,
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



    main_file = filp_open(FILENAME, O_CREAT | O_RDWR , 0666);

    if (IS_ERR(main_file)) {
        printk(KERN_ALERT "Failed to open file %s: %ld\n", FILENAME, PTR_ERR(main_file));
        return -1;
    }

    // temp_buffer = kmalloc(BUFFERSIZE + 1, GFP_KERNEL);
    
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

    main_file_offset = 0;

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