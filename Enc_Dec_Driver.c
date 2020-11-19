#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>	
#include <linux/types.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>

#include <linux/io.h>//iowrite ioread
#include <linux/slab.h>//kmalloc kfree
#include <linux/platform_device.h>//platform driver
#include <linux/of.h>//of match table
#include <linux/ioport.h>//ioremap

MODULE_LICENSE("Dual BSD/GPL");

dev_t my_dev_id;
static struct class *my_class;
static struct device *my_device;
static struct cdev *my_cdev;


struct ED_info {
	unsigned long mem_start;
	unsigned long mem_end;
	void __iomem *base_addr;
};

static struct ED_info *ip = NULL;//Enc_dec
static struct ED_info *bp1 = NULL;//bram0
static struct ED_info *bp2 = NULL;//bram1

#define BUFF_SIZE 30
#define number_of_minors 3
#define DRIVER_NAME "ED_driver"
#define DEVICE_NAME "ED"

int storage[10];
int pos = 0;
int endRead = 0;
int counter = 0;
int k=0;

static int ED_probe(struct platform_device * pdev);
static int ED_remove(struct platform_device *pdev);
int ED_open(struct inode *pinode, struct file *pfile);
int ED_close(struct inode *pinode, struct file *pfile);
ssize_t ED_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset);
ssize_t ED_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset);

static int __init ED_init(void);
static void __exit ED_exit(void);

struct file_operations my_fops =
{
	.owner = THIS_MODULE,
	.open = ED_open,
	.read = ED_read,
	.write = ED_write,
	.release = ED_close,
};

static struct of_device_id ED_of_match[] = {

	{ .compatible = "bram0", },
	{ .compatible = "bram1", },
	{ .compatible = "Enc_dec" },
	{ /* end of list */ }
};

MODULE_DEVICE_TABLE(of, ED_of_match);

static struct platform_driver ED_driver = {

	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = ED_of_match,
		},

	.probe = ED_probe,
	.remove = ED_remove,
};


static int ED_probe (struct platform_device *pdev)
{
	struct resource *r_mem;
	int rc = 0;
	r_mem=platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(!r_mem){
			printk(KERN_ALERT "Failed to get resource\n");
			return -ENODEV;
	}
	
	switch(counter){

	case 0://za bram0
		
		bp1 = (struct ED_info *) kmalloc(sizeof(struct ED_info), GFP_KERNEL);
		if(!bp1){
			printk(KERN_ALERT "Could not allocate memory\n");
			return -ENOMEM;
		}
		
		bp1->mem_start= r_mem->start;
		bp1->mem_end = r_mem->end;
		printk(KERN_INFO "start address: %x \t end address: %x", r_mem->start, r_mem->end);

		if(!request_mem_region(bp1->mem_start, bp1->mem_end - bp1->mem_start+1, DRIVER_NAME)){
			printk(KERN_ALERT "Could not lock memory region at %p\n", (void *) bp1->mem_start);
			rc = -EBUSY;
			goto error3;
		}
		
		bp1->base_addr = ioremap (bp1->mem_start, bp1->mem_end - bp1->mem_start+1);

		if(!bp1->base_addr){
			printk(KERN_ALERT "Could not allocate memory\n");
			rc = -EIO;
			goto error4;
		}

		counter ++;
		printk(KERN_WARNING "bram0 registered\n");
		return 0;//Sve sljaka

		error4:
			release_mem_region(bp1->mem_start, bp1->mem_end - bp1->mem_start+1);
		error3:
			return rc;

	case 1://za bram1

		bp2 = (struct ED_info *) kmalloc(sizeof(struct ED_info), GFP_KERNEL);
		if(!bp2){
			printk(KERN_ALERT "Could not allocate memory\n");
			return -ENOMEM;
		}
		
		bp2->mem_start= r_mem->start;
		bp2->mem_end = r_mem->end;
		printk(KERN_INFO "start address: %x \t end address: %x", r_mem->start, r_mem->end);

		if(!request_mem_region(bp2->mem_start, bp2->mem_end - bp2->mem_start+1, DRIVER_NAME)){
			printk(KERN_ALERT "Could not lock memory region at %p\n", (void *) bp2->mem_start);
			rc = -EBUSY;
			goto error6;
		}
		
		bp2->base_addr = ioremap (bp2->mem_start, bp2->mem_end - bp2->mem_start+1);

		if(!bp2->base_addr){
			printk(KERN_ALERT "Could not allocate memory\n");
			rc = -EIO;
			goto error5;
		}

		counter ++;
		printk(KERN_WARNING "bram1 registered\n");
		return 0;//Sve sljaka

		error5:
			release_mem_region(bp2->mem_start, bp2->mem_end - bp2->mem_start+1);
		error6:
			return rc;

	case 2://sad malo i za ip xD

		ip = (struct ED_info *) kmalloc(sizeof(struct ED_info), GFP_KERNEL);
		if(!ip){
			printk(KERN_ALERT "Could not allocate memory\n");
			return -ENOMEM;
		}
		
		ip->mem_start= r_mem->start;
		ip->mem_end = r_mem->end;
		printk(KERN_INFO "start address: %x \t end address: %x", r_mem->start, r_mem->end);

		if(!request_mem_region(ip->mem_start, ip->mem_end - ip->mem_start+1, DRIVER_NAME)){
			printk(KERN_ALERT "Could not lock memory region at %p\n", (void *) ip->mem_start);
			rc = -EBUSY;
			goto error1;
		}
		
		ip->base_addr = ioremap (ip->mem_start, ip->mem_end - ip->mem_start+1);

		if(!ip->base_addr){
			printk(KERN_ALERT "Could not allocate memory\n");
			rc = -EIO;
			goto error2;
		}

		counter ++;
		printk(KERN_WARNING "IP registered\n");
		return 0;//Sve sljaka

		error2:
			release_mem_region(ip->mem_start, ip->mem_end - ip->mem_start+1);
		error1:
			return rc;
	}
	return 0;
}

static int ED_remove (struct platform_device *pdev)
{
		
	switch(counter){

	case 0://za bram0
		printk(KERN_WARNING "bram0_remove: platform driver removing\n");
		iowrite32(0,bp1->base_addr);
		iounmap(bp1->base_addr);
		release_mem_region(bp1->mem_start, bp1->mem_end - bp1->mem_start+1);
		kfree(bp1);
		printk(KERN_INFO"bram0_remove: bram0 removed\n");

		break;

	case 1://za bram1
		
		printk(KERN_WARNING "bram0_remove: platform driver removing\n");
		iowrite32(0,bp1->base_addr);
		iounmap(bp1->base_addr);
		release_mem_region(bp1->mem_start, bp1->mem_end - bp1->mem_start+1);
		kfree(bp1);
		printk(KERN_INFO"bram0_remove: bram0 removed\n");
		counter--;

	break;

	case 2://za ip
		
		printk(KERN_WARNING "bram0_remove: platform driver removing\n");
		iowrite32(0,bp1->base_addr);
		iounmap(bp1->base_addr);
		release_mem_region(bp1->mem_start, bp1->mem_end - bp1->mem_start+1);
		kfree(bp1);
		printk(KERN_INFO"bram0_remove: bram0 removed\n");
		counter--;

	break;

	}
	return 0;
}

int ED_open(struct inode *pinode, struct file *pfile) 
{
		printk(KERN_INFO "Succesfully opened file\n");
		return 0;
}

int ED_close(struct inode *pinode, struct file *pfile) 
{
		printk(KERN_INFO "Succesfully closed file\n");
		return 0;
}

ssize_t ED_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset) 
{
		int ret;
		char buff[BUFF_SIZE];
		long int len;
		if (endRead){
			endRead = 0;
			pos = 0;
			printk(KERN_INFO "Succesfully read from file\n");
			return 0;
		}
		
		len = scnprintf(buff, BUFF_SIZE, "%d", storage[pos]);
		ret = copy_to_user(buffer, buff, len);
		if (ret)
			return -EFAULT;
		pos++;
		if (pos == 10) {
			endRead = 1;
		}
		return len;
	}


ssize_t ED_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset)
{
		char buff[BUFF_SIZE];
		int position, value;
		int ret;

		ret = copy_from_user(buff, buffer, length);
		if(ret)
			return -EFAULT;
		buff[length-1] = '\0';

		ret = sscanf(buff, "%d,%d",&value,&position);

		if(ret==2)
		{
			if(position >=0 && position <=9)
			{
				storage[position] = value;
				printk(KERN_INFO "Succesfully wrote value %d in position %d \n", value, position);
			}
			else
			{
				printk(KERN_WARNING "Position should be between 0 and 9\n");
			}
		}
		else
		{
			printk(KERN_WARNING "Wrong command format\nExpected: n, m\ntn-position\n\tm-value\n");
		}

		return length;
}  


static int __init ED_init(void)
{
   int ret = 0;
   int i = 0;
   for (i=0; i<10; i++)
	storage[i];


   ret = alloc_chrdev_region(&my_dev_id, 0, number_of_minors, "storage");
   if (ret){
      printk(KERN_ERR "failed to register char device\n");
      return ret;
   }
   printk(KERN_INFO "char device region allocated\n");

   my_class = class_create(THIS_MODULE, "ED_class1");
   if (my_class == NULL){
      printk(KERN_ERR "failed to create class\n");
      goto fail_0;
   }
   printk(KERN_INFO "class created\n");
   
   my_device = device_create(my_class, NULL,MKDEV(MAJOR(my_dev_id),0), NULL, "bram0");
   if (my_device == NULL){
      printk(KERN_ERR "failed to create device\n");
      goto fail_1;
   }
   printk(KERN_INFO "device created\n");

   my_device = device_create(my_class, NULL, MKDEV(MAJOR(my_dev_id),1), NULL,"bram1");
   if (my_device == NULL){
       printk(KERN_ERR "failed to create device\n");
       goto fail_1;
   }
       printk(KERN_INFO "device created\n");

   my_device = device_create(my_class, NULL, MKDEV(MAJOR(my_dev_id),2), NULL, "Enc_dec");
   if (my_device == NULL){
	printk(KERN_ERR "failed to create device\n");
	goto fail_1;
	}


	printk(KERN_INFO "device created\n");
 
	my_cdev = cdev_alloc();	
	my_cdev->ops = &my_fops;
	my_cdev->owner = THIS_MODULE;
	ret = cdev_add(my_cdev, my_dev_id, number_of_minors);
	if (ret)
	{
      printk(KERN_ERR "failed to add cdev\n");
		goto fail_2;
	}
   printk(KERN_INFO "cdev added\n");
   printk(KERN_INFO "Hello world\n");

   return 0;

   return platform_driver_register(&ED_driver);

   fail_2:
	device_destroy(my_class, my_dev_id);
   fail_1:
      class_destroy(my_class);
   fail_0:
      unregister_chrdev_region(my_dev_id, 1);
   return -1;

}
static void __exit ED_exit(void)
{
   int i = 0;
   platform_driver_unregister(&ED_driver);
   cdev_del(my_cdev);
   for (i=0; i < number_of_minors; i++)
	device_destroy(my_class, MKDEV(MAJOR(my_dev_id), i));
   class_destroy(my_class);
   unregister_chrdev_region(my_dev_id,4);
   printk(KERN_INFO "Goodbye, cruel world\n");
}


module_init(ED_init);
module_exit(ED_exit);
