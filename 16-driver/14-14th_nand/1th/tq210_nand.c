#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>

#include <plat/regs-nand.h>


struct tq210_nand_regs {
	unsigned long nfconf;
	unsigned long nfcont;
	unsigned long nfcmd;
	unsigned long nfaddr;
	unsigned long nfdata;
	unsigned long nfmeccd0;
	unsigned long nfmeccd1;
	unsigned long nfseccd;
	unsigned long nfsblk;
	unsigned long nfeblk;
	unsigned long nfstat;
	unsigned long nfeccerr0;
	unsigned long nfeccerr1;
	unsigned long nfmecc0;
	unsigned long nfmecc1;
	unsigned long nfsecc;
	unsigned long nfmlcbitpt;
};



static struct mtd_info *tq210_mtd;
static struct nand_chip *tq210_nand_chip;
static struct tq210_nand_regs *tq210_nand_regs;




/* Default select function for 1 chip devices 
 * chipnr: chipnumber to select, -1 for deselect
 */
static void tq210_select_chip(struct mtd_info *mtd, int chipnr)
{
	if (chipnr == -1) {
		/* ȡ��ѡ�� */
		tq210_nand_regs->nfcont |= (1 << 1);
	} else {
		/* ѡ�� */
		tq210_nand_regs->nfcont &= ~(1 << 1);
	}
}


static void tq210_nand_hwcontrol(struct mtd_info *mtd, int cmd,
				   unsigned int ctrl)
{

	if (ctrl & NAND_CLE) {
		/* д���� */
		tq210_nand_regs->nfcmd = cmd;
	} else {
		/* д��ַ */
		tq210_nand_regs->nfaddr = cmd;
	}
}


static int tq210_nand_dev_ready(struct mtd_info *mtd)
{
	return (tq210_nand_regs->nfstat & (1 << 0));
}



static int tq210_nand_init(void)
{
	struct clk *clk;
	tq210_nand_regs = ioremap(0xB0E00000, sizeof(struct tq210_nand_regs));
	/* 1.����һ��nand_chip�ṹ�� */
	tq210_nand_chip = kzalloc(sizeof(struct nand_chip), GFP_KERNEL);
	
	/* 2.����nand_chip  */

	/* ����nand_chip�Ǹ�nand_scan����ʹ�õ�,�����֪����ô���ã��ȿ�nand_scan��ôʹ��
	 * ��Ӧ���ṩ:ѡ�У����������ַ�������ݣ������ݣ��ж�״̬����
	 */
	tq210_nand_chip->select_chip = tq210_select_chip;
	tq210_nand_chip->cmd_ctrl    = tq210_nand_hwcontrol;
	tq210_nand_chip->IO_ADDR_R   = &tq210_nand_regs->nfdata;
	tq210_nand_chip->IO_ADDR_W   = &tq210_nand_regs->nfdata;
	tq210_nand_chip->dev_ready	 = tq210_nand_dev_ready;
	tq210_nand_chip->ecc.mode	 = NAND_ECC_SOFT;

	/* 3.Ӳ����ص����� */
	clk = clk_get(NULL, "nand");
	clk_enable(clk);

#define TACLS    1
#define TWRPH0   2
#define TWRPH1   1
	tq210_nand_regs->nfconf = (TACLS<<12) | (TWRPH0<<8) | (TWRPH1<<4);

	tq210_nand_regs->nfcont = (1<<1) | (1<<0);
	
	/* 4.ʹ��:nand_scan */
	tq210_mtd = kzalloc(sizeof(struct mtd_info), GFP_KERNEL);

	tq210_mtd->owner = THIS_MODULE;
	tq210_mtd->priv = tq210_nand_chip;
	
	nand_scan(tq210_mtd, 1);		/* ʶ��nand_flash,����mtd_info */

	/* 5.add_mtd_partitions */



	
	return 0;
}



static void tq210_nand_exit(void)
{
	kfree(tq210_mtd);
	kfree(tq210_nand_chip);
	iounmap(tq210_nand_regs);

}



module_init(tq210_nand_init);
module_exit(tq210_nand_exit);
MODULE_AUTHOR("Sourcelink");
MODULE_VERSION("Linux_3.10.8");
MODULE_DESCRIPTION("TQ210 nand_flash Driver");
MODULE_LICENSE("GPL");



