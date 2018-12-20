#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <signal.h>

//#define READ_DATA_TIMEOUT 5

/* test example:
*  ./spi_test -l 10 -d FF 5A A5 03 04 05 06 07 08 09  -t 1
*/

int g_fd = -1;

static void writeMsg(unsigned char *pMsg, int msgLen)
{
	if (-1 != g_fd) {
		int size = write(g_fd, pMsg, msgLen);
		if (msgLen != size)
			printf("ERROR: Send message failed! write_size: %d != msgLen: %d !\n", size, msgLen);
		else
			printf("INFO: Send message [%02x %02x %02x %02x %02x] successful\n",
				pMsg[0], pMsg[1], pMsg[2], pMsg[3], pMsg[4]);
	}
}

int main(int argc, char *argv[])
{
	int j=0;
	int ms=0;
	int num=0;
	unsigned char output[256];
	char *Dev="/dev/mcu_spi";
	int cnt=0;
	unsigned char fc[5] = {0x04, 0x7d, 0x01, 0x01, 0x00};

	g_fd = open(Dev,O_RDWR);

	if(g_fd < 0)
	{
		perror("Can't Open SPI Port");
		return -1;
	}
	 sleep(1);
       writeMsg(fc, 4);
//	 alarm(READ_DATA_TIMEOUT); /* read data can not exceed 5 seconds */
		cnt = read(g_fd,output,255);
		printf("mcu version number:");
		for(j=2; j< cnt; j++)
		{
		printf("%c", output[j]);
		}
		printf("\n");
	 return 0;
}


