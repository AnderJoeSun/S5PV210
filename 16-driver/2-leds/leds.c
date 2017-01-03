#include <linux/crypto.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/bitops.h>
#include <asm/unaligned.h>
#include <asm/io.h>



static struct class	*ledsdrv_class;
static struct device *ledsdrv_class_devs[3];

static volatile unsigned long *gpc0con;
static volatile unsigned long *gpc0dat;

static int leds_drv_open(struct inode *inode, struct file *file)
{
	int minor  = MINOR(inode->i_rdev);

	//printk("open minor :%d\n",minor);
	
	switch(minor) {
		case 0:
				*gpc0con &= ~((0xf << (3 * 4)) | (0xf << (4 * 4)));
				*gpc0con |= ((0x1 << (3 * 4)) | (0x1 << (4 * 4))); 
				break;
		case 1:
				*gpc0con &= ~(0xf << (3 * 4));
				*gpc0con |= (0x1 << (3 * 4));
				break;
		case 2:
				*gpc0con &= ~(0xf << (4 * 4));
				*gpc0con |= (0x1 << (4 * 4));
				break;
		
		default:break;

	}
	
	return 0;
}

static int leds_drv_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{


	int minor = MINOR(file->f_dentry->d_inode->i_rdev);

	char val;
	
	copy_from_user(&val, buf, 1);

	//printk("minor : %d , val : %d\n",minor,val);
	
	switch (minor) {
		case 0:			/* /dev/leds */
			if (val == 1)
				*gpc0dat |= ((1 << 3) | (1 << 4));
			else
				*gpc0dat &= ~((1 << 3) | (1 << 4));
			break;
		case 1:			/* /dev/led1 */
			if (val == 1)
				*gpc0dat |= (1 << 3);    //on 
			else
				*gpc0dat &= ~(1 << 3);   //off
			break;	
		case 2:			/* /dev/led2 */
			if (val == 1)
				*gpc0dat |= (1 << 4);
			else
				*gpc0dat &= ~(1 << 4);
			break;
			
		default:break;
	}	
	
	return 0;
}

struct file_operations leds_drv_fops = {
		.owner = THIS_MODULE,
		.open = leds_drv_open,
		.write = leds_drv_write,
};

int led_major;
static int leds_drv_init(void)
{
	int minor;

	/* ע���ַ��豸
     * ����Ϊ���豸�š��豸���֡�file_operations�ṹ��
     * ���������豸�žͺ;����file_operations�ṹ��ϵ�����ˣ�
     * �������豸ΪLED_MAJOR���豸�ļ�ʱ���ͻ����leds_drv_fops�е���س�Ա����
     * LED_MAJOR������Ϊ0����ʾ���ں��Զ��������豸��
     */
	led_major = register_chrdev(0, "leds_drv", &leds_drv_fops);

	ledsdrv_class = class_create(THIS_MODULE, "leds");

	ledsdrv_class_devs[0] = device_create(ledsdrv_class, NULL, MKDEV(led_major, 0), NULL, "leds");	/* /dev/leds */

	
	for (minor = 1; minor < 3; minor++) {
			ledsdrv_class_devs[minor] = device_create(ledsdrv_class, NULL, MKDEV(led_major,minor), NULL, "led%d", minor);			
		}

	gpc0con = (volatile unsigned long *)ioremap(0xE0200060, 24);
	gpc0dat = gpc0con + 1;

	printk("leds initialized\n");
	
	return 0;
}


static void leds_drv_exit(void)
{

	int minor;
    /* ж���������� */

	unregister_chrdev(led_major, "leds_drv");
	for (minor = 0 ; minor < 3; minor ++) 
			device_unregister(ledsdrv_class_devs[minor]);
	
	class_destroy(ledsdrv_class);
    iounmap(gpc0con);
	
}


module_init(leds_drv_init);
module_exit(leds_drv_exit);


MODULE_AUTHOR("Sourcelink");
MODULE_VERSION("0.1.0");
MODULE_DESCRIPTION("TQ210 LED Driver");
MODULE_LICENSE("GPL");




