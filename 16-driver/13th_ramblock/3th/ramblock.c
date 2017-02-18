#include <linux/major.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/bitops.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/hdreg.h>

#include <asm/setup.h>
#include <asm/pgtable.h>



#define RAMBLOCK_SIZE	(1024 * 1024)

static struct gendisk *ramblock_gendisk;
static struct request_queue *ramblock_queue;
static unsigned char *ramblock_buf;
static int major;

static DEFINE_SPINLOCK(ramblock_lock);


static void do_ramblock_request(struct request_queue *q)
{
//	static int cnt = 0;
	struct request *req;

	//printk("do_ramblock_request cnt = %d\n",++cnt);

	req = blk_fetch_request(q);
	while (req) {
	/* ������Ҫ��:Դ��Ŀ�ġ����� */
		unsigned long offset = blk_rq_pos(req) << 9;
		unsigned long len  = blk_rq_cur_bytes(req);

		if (rq_data_dir(req) == READ)
				memcpy(req->buffer, ramblock_buf + offset, len);
		else
				memcpy(ramblock_buf + offset, req->buffer, len);

		/* wrap up, 0 = success, -errno = fail */
		if (!__blk_end_request_cur(req, 0))
			req = blk_fetch_request(q);
	}
}

static int ramblock_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	/* ��-->��-->���� */
	geo->heads = 2;	
	geo->cylinders = 32;
	geo->sectors = RAMBLOCK_SIZE / 2 / 32 / 512;
	
	return 0;
}



static const struct block_device_operations ramblock_fops =
{
	.owner		= THIS_MODULE,
	.getgeo		= ramblock_getgeo,
};

static int ramblock_init(void)
{
	/* ע�����豸�� */
	major = register_blkdev(0 , "ramblock");

	/* ����һ��genddisk�ṹ�� */
	ramblock_gendisk = alloc_disk(3);

	/* ��ʼ������ */
	ramblock_queue = blk_init_queue(do_ramblock_request, &ramblock_lock);
	
	ramblock_gendisk->major = major;
    ramblock_gendisk->first_minor = 0;
    ramblock_gendisk->fops = &ramblock_fops;
    sprintf(ramblock_gendisk->disk_name, "ramblock");
	set_capacity(ramblock_gendisk, RAMBLOCK_SIZE / 512); 					//p->heads * p->cylinders * p->sectors

    ramblock_gendisk->queue = ramblock_queue;

	/* Ӳ������ */
	ramblock_buf = kzalloc(RAMBLOCK_SIZE, GFP_KERNEL);
	
	/* ע��disk */
    add_disk(ramblock_gendisk);
	
	return 0;
}

static void ramblock_exit(void)
{
	unregister_blkdev(major, "ramblock");
	del_gendisk(ramblock_gendisk);
	put_disk(ramblock_gendisk);
	blk_cleanup_queue(ramblock_queue);
	
	kfree(ramblock_buf);
}


module_init(ramblock_init);
module_exit(ramblock_exit);

MODULE_AUTHOR("Sourcelink");
MODULE_VERSION("Linux_3.10.8");
MODULE_DESCRIPTION("TQ210 ramblock Driver");
MODULE_LICENSE("GPL");



