#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

int fd;

void my_signal_fun(int signum)
{
	unsigned char key_val;

	read(fd , &key_val, 1);
	printf("key_val = 0x%x\n",key_val);
}


int main(int argc, char **argv)
{

	int  Oflags;
	unsigned int  ret = 0 ;
	unsigned char key_val;
	
//	signal(SIGIO, my_signal_fun);	/* �յ�io�ź���ִ�лص����� */
	
	fd = open("/dev/buttons", O_RDWR | O_NONBLOCK);

	if (fd < 0) {
		printf("can't open!\n");
		return -1;
	} else {
		printf("open successful\n");
	}
	
#if 0	
	fcntl(fd, F_SETOWN, getpid());			/* ���ý���ָ��Ϊ�豸�ļ��������ߣ�Ŀ��:���ں�֪���źŵ���ʱ��֪ͨ�ĸ��ź�  */
	Oflags = fcntl(fd, F_SETFL);			/* ��ȡ��ǰ�ļ���־ */
	fcntl(fd, F_SETFL, Oflags | FASYNC);	/* �����豸�ļ�Ϊ�첽֪ͨ��������fasync���������� */
#endif

	/*
	 * �����ݵ���ʱ�����е�ע���첽֪ͨ�Ľ��̶����յ�һ��SIGIO�ź�
	 */

	while (1) {
			ret = read(fd , &key_val, 1);
			printf("key_val = 0x%x, ret = %d\n",key_val,ret);
			sleep(5);
	}
			

	return 0;
}


