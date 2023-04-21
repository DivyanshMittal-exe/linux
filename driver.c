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


#define NO_OF_DEVICES 4

#define BUFFERSIZE (1 << 16)

#define FILENAME ""

#define READER 2

#define WRITER 3

struct semaphore lck;            
wait_queue_head_t waiting_readers;


char* temp_buffer;
struct file *main_file = NULL;
loff_t main_file_offset = 0;


struct LIFO_pipe {
        int my_type;
        struct cdev cdev;                  /* Char device structure */
};


dev_t LIFO_char_dev;

static struct LIFO_pipe* LIFO_devices;




static int LIFO_open(struct inode *inode, struct file *filp)
{
	struct LIFO_pipe *lif_dev;

	lif_dev = container_of(inode->i_cdev, struct LIFO_pipe, cdev);
	filp->private_data = lif_dev;

	if (down_interruptible(&lck))
		return -ERESTARTSYS;




	if (filp->f_mode & FMODE_READ){
        my_type = READER;
    }else if(filp->f_mode & FMODE_WRITE){
        my_type = WRITER;
    }


	up(&lck);

	return nonseekable_open(inode, filp);
}








static ssize_t LIFO_reader (struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos) 
{

    int retval;


    char *buffer = kmalloc(count, GFP_KERNEL);
    if (!buffer) {
    
    }

    int buffer_offset = 0;


    size_t count_copy = count;

    while(count){
        if (down_interruptible(&lck))
		    return -ERESTARTSYS;


        while (main_file_offset == 0) { 

            up(&lck); 

        
            if (wait_event_interruptible(waiting_readers, (main_file_offset != 0)))
                return -ERESTARTSYS;
                

            if (down_interruptible(&lck))
                return -ERESTARTSYS;
        }

        int gonna_read =  min(count, main_file_offset);

        retval = vfs_read(main_file, buffer + buffer_offset, gonna_read , &main_file_offset);
        
        count -= gonna_read;
        buffer_offset += gonna_read;


         up(&lck);

    }

    for(int i = 0; i < count_copy/2; i++){
        char temp1 = buffer[i];
        char temp2 = buffer[count_copy - 1 - i];
        buffer[i] = temp2;
        buffer[count_copy - 1 - i] = temp1;
    }



    if (copy_to_user(buf, buffer, retval)) {
        return -EFAULT;
    }

    kfree(buffer);

    return count_copy;

}


ssize_t	LIFO_writer(struct file *file, const char __user *buf, size_t count, loff_t *ppos){

    struct LIFO_pipe *lifo_obj = filp->private_data;

    int offset = 0;

    ssize_t retval = count;

    printk(KERN_INFO "Started Writing \n");

    if (down_interruptible(&lck))
		    return -ERESTARTSYS;

    do{

        
        
        if (copy_from_user(temp_buffer, buf + offset, min(count,BUFFERSIZE )) != 0)
        {
            	up (&lck);
		        return -EFAULT;
        }

        printk(KERN_INFO "%s\n", temp_buffer);
        
        int  retval = vfs_write(main_file, temp_buffer ,  min(count,BUFFERSIZE ), &main_file_offset);


        offset += BUFFERSIZE;
        count -= BUFFERSIZE;


    }while(count > 0);

    up (&lck);

    wake_up_interruptible(&waiting_readers);


    return retval;


}





static const struct proc_ops LIFO_proc_ops = {
    .owner = THIS_MODULE,
    .proc_write = sig_proc_write_handler,
};


static int __init sig_init(void)
{

    if (alloc_chrdev_region(&LIFO_char_dev, 0, NO_OF_DEVICES, "LIFO_CHAR_DEVICE") < 0)
    {
        printk(KERN_ALERT "Unable to allocate major-minor number\n");
        return -1;
    }

    LIFO_devices = kmalloc(NO_OF_DEVICES * sizeof(struct LIFO_pipe), GFP_KERNEL);
    
    if (LIFO_devices == NULL) {
		unregister_chrdev_region(LIFO_char_dev, NO_OF_DEVICES);
		return -1;
	}


    for (size_t i = 0; i < NO_OF_DEVICES; i++)
    {
        cdev_init(LIFO_devices + i, &LIFO_proc_ops);
        (LIFO_devices + i)->cdev.owner = THIS_MODULE;

        if (cdev_add(&((LIFO_devices + i)->cdev), LIFO_char_dev + i, 1) < 0)
        {
            printk(KERN_ALERT "Device not added");
            unregister_chrdev_region(LIFO_char_dev + i, 1);
            return -1;
        }
    }
    



    init_waitqueue_head(&waiting_readers);
	init_MUTEX(&lck);





    // main_file = kmalloc(sizeof(struct file), GFP_KERNEL);
    // if (!main_file) {
    //     printk(KERN_ALERT "Data not allocated for sturct file main_file: %lu \n", sizeof(struct file));
    //     return -ENOMEM;
    // }


    main_file = filp_open(FILENAME, O_CREAT | O_WRONLY | O_TRUNC, 0600);

    temp_buffer = kmalloc(BUFFERSIZE + 1, GFP_KERNEL);
    
    if (!temp_buffer) {
        printk(KERN_ALERT "Data not allocated for temp_buffer: %lu \n", BUFFERSIZE);
        return -ENOMEM;
    }

    // Added so that I can easily printk these chunks;
    temp_buffer[BUFFERSIZE] = '\0';

    printk(KERN_INFO "Lifo device added, major number given is %d\n",MAJOR(LIFO_char_dev));


    return 0;
}

static void __exit sig_exit(void)
{
    cdev_del(&LIFO_char_cdev);
    unregister_chrdev_region(LIFO_char_dev, NO_OF_DEVICES);


    printk(KERN_INFO "Lifo Device says bye world!\n");
}

module_init(sig_init);
module_exit(sig_exit);

MODULE_AUTHOR("Divyansh Mittal");
MODULE_LICENSE("GPL");