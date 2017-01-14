#include <linux/crypto.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/bitops.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/gpio.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <asm/unaligned.h>
#include <asm/io.h>
#include <mach/gpio.h>
#include <mach/regs-gpio.h>

struct pin_desc {
		unsigned int pin;
		unsigned char key_val;
};


static struct class *sixthdrv_class;
static struct device *sixthdrv_class_devs;

static DECLARE_WAIT_QUEUE_HEAD(button_waitq);

static volatile int ev_press = 0;

struct fasync_struct *button_fasync;				/* ����źźͽ���id��Ϣ  */

//static atomic_t canopen = ATOMIC_INIT(1);    		/* ����ԭ�ӱ�������ʼ��Ϊ1 */
static DEFINE_MUTEX(button_lock);     		/* ���廥���� */
//static DEFINE_SEMAPHORE(button_lock);				/* �����ź��� */

static struct timer_list buttons_timer;

static struct pin_desc *irq_pd;


/*
	��ֵ
*/
static unsigned char key_val;

struct pin_desc pins_desc[4] = {
		{EINT_GPIO_0(0), 0x01},
		{EINT_GPIO_0(1), 0x02},
		{EINT_GPIO_0(2), 0x04},
		{EINT_GPIO_0(3), 0x08},
};

/*
 * ȷ�����µļ�ֵ	
 */
static irqreturn_t buttons_interrupt (int irq, void *dev_instance)
{	
	irq_pd = (struct pin_desc *)dev_instance;

	/* HZ = 100��10ms�ж�һ�Σ���ʱ���ж�1�ξ��� Ҳ����10ms��һ�� */
	mod_timer(&buttons_timer, jiffies + HZ / 100);

//	printk("HZ = %d, jiffies = %d \n", HZ, jiffies);
	return IRQ_RETVAL(IRQ_HANDLED); 
}




static int sixth_drv_open (struct inode * inode, struct file * file)
{
//ԭ�Ӳ���
#if 0
	if (!atomic_dec_and_test(&canopen)) {		/* ��1�Ƿ����0 */
		atomic_inc(&canopen);
		return -EBUSY;
	}
#endif

//�ź���
#if 0 
	mutex_lock(&button_lock);
#endif 

//����
#if 1
	if (file->f_flags & O_NONBLOCK) {
		if(!mutex_trylock(&button_lock))	/* ����]�гɹ��@����̖���t���� */
			return -EBUSY;
	} else { 
		mutex_lock(&button_lock);
	}
#endif	

	/* ���� CPH0_0~CPH0_3 Ϊ�������� */	
	request_irq(IRQ_EINT(0), buttons_interrupt, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, "KEY1", &pins_desc[0]);
	request_irq(IRQ_EINT(1), buttons_interrupt, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, "KEY2", &pins_desc[1]);
	request_irq(IRQ_EINT(2), buttons_interrupt, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, "KEY3", &pins_desc[2]);
	request_irq(IRQ_EINT(3), buttons_interrupt, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, "KEY4", &pins_desc[3]);
	
	return 0;
}


static ssize_t sixth_drv_read (struct file *file, char __user *userbuf, size_t bytes, loff_t *off)
{

	if (bytes != 1)
		return -EINVAL;

	if (file->f_flags & O_NONBLOCK) {
		if (!ev_press) 
			return -EAGAIN;			
	} else {
		/* ���û�а��������������� */
		wait_event_interruptible(button_waitq, ev_press);     //ev_press = 0 ˯�� ���Ȱѽ��̼����Լ������button_waitq�ȴ�������
	}

	
	copy_to_user(userbuf, &key_val, 1);
	ev_press = 0;
	
	
	//printk("readkey_val = 0x%x\n",key_val);

	
		
	return 0;
}



int sixth_drv_relaease (struct inode *inode, struct file *file)
{
	free_irq(IRQ_EINT(0), &pins_desc[0]);
	free_irq(IRQ_EINT(1), &pins_desc[1]);
	free_irq(IRQ_EINT(2), &pins_desc[2]);
	free_irq(IRQ_EINT(3), &pins_desc[3]);

//	atomic_inc(&canopen);
	if(!mutex_trylock(&button_lock))	/* ����]�гɹ��@����̖���t���� */
		mutex_unlock(&button_lock);	

	return 0;
}

static unsigned int sixth_drv_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;

	poll_wait(file, &button_waitq, wait);			/* �ж��¼��������ѽ��̼���button_waitq �ȴ������� */


	if (ev_press)									/* �ж��л��ѽ��̣�*/
		mask |= POLLIN |POLLRDNORM;

	return mask;
}

static int sixth_drv_fasync(int fd, struct file *filp, int on)
{
	printk("driver: sixth_drv_fasync\n");

	return fasync_helper(fd, filp, on, &button_fasync);
}


struct file_operations sixth_drv_fops = {
		.owner   = THIS_MODULE,
		.open    = sixth_drv_open,
		.read    = sixth_drv_read,
		.release = sixth_drv_relaease,
		.poll 	 = sixth_drv_poll,
		.fasync  = sixth_drv_fasync,
};


static void buttons_timer_fun(unsigned long data)
{
	struct pin_desc *pin_desc = irq_pd;

	if (!pin_desc)
	return;

	if(!gpio_get_value(pin_desc->pin))
			key_val = pin_desc->key_val;			//����
	else
			key_val = pin_desc->key_val | 0x80;		//�ɿ�

	//printk("irqkey_val = 0x%x\n",key_val);
	
	ev_press = 1;
	wake_up_interruptible(&button_waitq);		/* �������ߵĽ��� */

	//kill_fasync(&button_fasync, SIGIO, POLL_IN);	/* ����io�źŸ�Ӧ�ò� */

}

int major;
static int sixth_drv_init(void)
{

	init_timer(&buttons_timer);
	buttons_timer.function = buttons_timer_fun;
	//buttons_timer.expires	= 0;
	add_timer(&buttons_timer);
	
	major = register_chrdev(0, "sixth_drv", &sixth_drv_fops);

	sixthdrv_class = class_create(THIS_MODULE, "sixth_drv");

	sixthdrv_class_devs = device_create(sixthdrv_class, NULL, MKDEV(major,0), NULL, "buttons");

	return 0;
}

static void sixth_drv_exit(void)
{
	unregister_chrdev(major, "sixth_drv");

	device_unregister(sixthdrv_class_devs);
	
	class_destroy(sixthdrv_class);

}

module_init(sixth_drv_init);
module_exit(sixth_drv_exit);

MODULE_AUTHOR("Sourcelink");
MODULE_VERSION("0.1.0");
MODULE_DESCRIPTION("TQ210 sixth Driver");
MODULE_LICENSE("GPL");




