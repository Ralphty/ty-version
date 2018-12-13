#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <signal.h>

#define READ_DATA_TIMEOUT 5

int fd;
int flag = 0;

int getnum(char *s1[], char *s2, int argc)
{
	int i;

	for (i = 0; i < argc; i++) {
		/* printf("s1:%s s2:%s\n",s1[i],s2); */
		if(strcmp(s1[i],s2) == 0)
			return i;
	}
	return -1;
}

int tolower(int c)
{
    if (c >= 'A' && c <= 'Z')
    {
        return c + 'a' - 'A';
    }
    else
    {
        return c;
    }
}

int htoi(char s[])
{
    int i;
    int n = 0;
    if (s[0] == '0' && (s[1]=='x' || s[1]=='X'))
    {
        i = 2;
    }
    else
    {
        i = 0;
    }
    for (; (s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'z') || (s[i] >='A' && s[i] <= 'Z');++i)
    {
        if (tolower(s[i]) > '9')
        {
            n = 16 * n + (10 + tolower(s[i]) - 'a');
        }
        else
        {
            n = 16 * n + (tolower(s[i]) - '0');
        }
    }
    return n;
}

/* test example:
*  ./spi_test -l 10 -d FF 5A A5 03 04 05 06 07 08 09  -t 1
*/

static void sig_alarm(int signo)
{
	if(flag)
	{
		printf("*** error:can't read mcu spi data ***\n");
		close(fd);
		exit(-1);
	}
}

void usage(void)
{
	char helpstring[] = {
	"\r\n *******************************************************************************"
	"\r\n spi test help"
	"\r\n -h: list spi test help"
	"\r\n -c: only sent data to mcu"
	"\r\n -p: printf date from mcu sending"
	"\r\n -f x: frame lost test, [x] is the times to test"
	"\r\n -l x: [x] the lenth of total data"
	"\r\n -d x: data to send. if [x] = -0 data will be send 0 to (lenth - 1)"
	"\r\n -r: if [-r] exist,data will be send 255 to (255-lenth)"
	"\r\n -t x: [x] is the times to test"
	"\r\n -e x x x ...:free mode"
	"\r\n *******************************************************************************"
	"\r\n "
	};

	printf("%s",helpstring);
}


int main(int argc, char *argv[])
{
	int i,j,k, t=0,tmp,len=0;
	int oflags;
	int ms=0;
	int num=0;
	unsigned char input[256];
	unsigned char output[256];
	char *Dev="/dev/mcu_spi";
	int cnt=0;
	unsigned char *p = input;
	unsigned char *q = output;
	char ret=0;

	if ((num = getnum(argv,"-h",argc)) != -1) {
		usage();
		return 0;
	}

	fd = open(Dev,O_RDWR);

	if(fd < 0)
	{
		perror("Can't Open SPI Port");
		return -1;
	}

	 if (signal(SIGALRM, sig_alarm) < 0)
		perror("signal");

	if ((num = getnum(argv,"-l",argc)) != -1) {
		len = atoi(argv[num+1]);
		/* printf("len = %d\n",len); */
	}

	if ((num = getnum(argv,"-e",argc)) != -1) {
		for(i=0; i < len; i++)
			input[i] = htoi(argv[num+i+1]);

		ret = write(fd,input,len);
		if(ret < 0) {
			printf("*** error:mcu spi write error***\n");
			goto error;
		}
		close(fd);
		return 0;
	}


	if((num = getnum(argv,"-s",argc)) != -1)
		ms = atoi(argv[num+1]);
	else
		ms = 0;
	/* 04 02 01 02 00, */
	input[0] = 0x04;			/* cmd type */
	input[1] = 0x02;			/* cmd id hi */
	input[2] = 0x01;			/* cmd id low */
	input[3] = (unsigned char)(len) + 1;	/* cmd data size */
	input[4] = 0;				/* cmd flag */

	if ((num = getnum(argv,"-f",argc)) != -1) {
		input[3] = 1;
		tmp = atoi(argv[num+1]);
		/* printf("time= %d\n",tmp); */
		for(i=0; i < tmp; i++) {
			input[5] = (unsigned char)(i);
			ret = write(fd,input,6);
			if(ret < 0) {
				printf("*** error:mcu spi write error***\n");
				goto error;
			}
			usleep(ms*1000);
		}
		close(fd);
		return 0;
	}

	if((num = getnum(argv,"-d",argc)) != -1)
		if(strcmp(argv[num+1],"-0") != 0) {
			for(i=0; i < len; i++) {
				tmp = htoi(argv[i+4]);
				input[5+i] = (unsigned char)(tmp);
			}
		}
		else {
			if ((num = getnum(argv,"-r",argc)) != -1) {
				for(i=0; i < len; i++) {
					input[5+i] = (unsigned char)(255-i);
				}
			}
			else {
				for(i=0; i < len; i++) {
					input[5+i] = (unsigned char)(i);
				}
			}
		}

	if((num = getnum(argv,"-t",argc)) != -1)
		t = atoi(argv[num+1]);
	else
		t = 0;

	/* printf("need to send %d times each %d ms\n",t,ms); */
	if((num = getnum(argv,"-c",argc)) != -1)
	{
		for(i = 0; i < t; i++) {
			ret = write(fd,input,(5+len));
			if(ret < 0) {
				printf("*** error:mcu spi write error***\n");
				goto error;
			}
			usleep(ms*1000);
		}
	}
	else
	{
		for(i = 0; i < t; i++) {
			ret = write(fd,input,(5+len));
			if(ret < 0) {
				printf("*** error:mcu spi write error***\n");
				goto error;
			}

			alarm(READ_DATA_TIMEOUT); /* read data can not exceed 5 seconds */
			memset(output,0,sizeof(output));

			flag = 1;
			cnt = read(fd,output,255);
			flag = 0;

			if((num = getnum(argv,"-p",argc)) != -1) {
				for(j=0; j< cnt-1; j++)
				printf("0x%02X ", output[j]);
				printf("]\n");
			}

			p = input+5; /* ignore command type and command id */
			q = output+5;
			k = len;
			while(k--)
			{
				if(*p++ != *q++)
				{
					printf("*** error: 1read mcu spi data error ***\n");
					goto error;
				}
			}
			usleep(ms*1000);
		}
	}

	close(fd);
	return 0;

error:
	close(fd);
	return -1;
}
