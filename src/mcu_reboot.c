#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>

/************************************************************
 * Call radio board Reboot
 ***********************************************************/
#define REMOTE_MESSAGE_BUFFER_SIZE	256
int g_fd = -1;

static void remote_message_handler(int signum)
{
	printf("INFO: Handle remote message %d- Begine\n", signum);
	if (-1 != g_fd) {
		unsigned char data[REMOTE_MESSAGE_BUFFER_SIZE] = {0};
		int len = read(g_fd, data, REMOTE_MESSAGE_BUFFER_SIZE);

		if (len > 0) {
			char msg_str[REMOTE_MESSAGE_BUFFER_SIZE] ="Read: ";
			int str_idx = 0;
			int str_len_max = REMOTE_MESSAGE_BUFFER_SIZE -12;

			int i = 0;
			for (; i<len; i++) {
				if (str_idx < str_len_max-4) {
					sprintf( msg_str+12 + str_idx, "%02x ", data[i]);
					str_idx += 3;
				}
			}

			if (str_idx < str_len_max)
				msg_str[str_idx+12] = 0;	/* EOS */
			else
				msg_str[str_len_max - 1] = 0;	/* EOS */
			printf("%s\n", msg_str);
		}
	}
	printf("INFO: Handle remote message %d- End\n", signum);
}

static int remote_init()
{
	if (-1 == g_fd) {
		g_fd = open("/dev/mcu_spi",O_RDWR);
		if (-1 == g_fd) {
			printf("ERROR: Open device \"/dev/mcu_spi\" failed!\n");
			return -1;
		} else {
			/* Asynchronous notification initialization */
			if (-1 == fcntl(g_fd, F_SETOWN, getpid())) {
				printf("ERROR: F_SETOWN failed!\n");
				return -1;
			}

			int oflags = fcntl(g_fd, F_GETFL);

			if (-1 == fcntl(g_fd, F_SETFL, oflags | FASYNC)) {
				printf("ERROR: F_SETFL: FASYNC failed!\n");
				return -1;
			}
			signal(SIGIO, remote_message_handler);
		}
	}
	return 0;
}


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

static int system_reboot(void)
{
	if (0 == remote_init()) {
		sleep(1);
		unsigned char fc[5] = {0x04, 0x55, 0x02, 0x01, 0x00};
		writeMsg(fc, 4);
	}
	return 0;
}

int main(int argc,char *argv[])
{
	/* Call 32bit-MCU reboot */
	system_reboot();

	return 0;
}
