#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <signal.h>

#define MAX_BUFF 32
#define LINE_BUFF 16
#define FPGA_TEXT_LCD_DEVICE "/dev/fpga_text_lcd"
#define FPGA_PUSH_SWITCH_DEVICE "/dev/fpga_push_switch"
#define MAX_BUTTON 9
#define MOTOR_ATTRIBUTE_ERROR_RANGE 4
#define FPGA_STEP_MOTOR_DEVICE "/dev/fpga_step_motor"

int main(void)
{
	double d_cur;
	int dev_text;
	int str_size;

	int dev_switch;
	int set=18,cur=0,on=0,off=0,wait=0,count=0;
	int buff_size;

	int dev_motor;
	int motor_action=0;
	int motor_direction=0;
	int motor_speed=0;

	unsigned char motor_state[3];
	unsigned char string[32];
	unsigned char date[16];
	unsigned char* temper;	
	unsigned char push_sw_buff[MAX_BUTTON];

	struct tm *tm;
	time_t t;

	FILE *wfp,*rfp;
	char buf[BUFSIZ];
	
	dev_text = open(FPGA_TEXT_LCD_DEVICE, O_WRONLY);			
	if (dev_text<0) {
		printf("Device open error : %s\n",FPGA_TEXT_LCD_DEVICE);
		return -1;
	}

	dev_switch = open(FPGA_PUSH_SWITCH_DEVICE, O_RDWR);
	if (dev_switch<0) {
		printf("Device open error : %s\n",FPGA_PUSH_SWITCH_DEVICE);
		return -1;
	}

	memset(motor_state,0,sizeof(motor_state));
	motor_state[1]=(unsigned char)motor_direction;
	dev_motor = open(FPGA_STEP_MOTOR_DEVICE, O_WRONLY);
	if (dev_motor<0) {
		printf("Device open error : %s\n",FPGA_STEP_MOTOR_DEVICE);
		return -1;
	}
	buff_size=sizeof(push_sw_buff);
	printf("System start\n");

	while(1) {
		memset(string,0,sizeof(string));
		time(&t);
		tm=localtime(&t);
		strftime(date, sizeof(date),"%r",tm);

		if((wfp=fopen("setting.txt","w"))==NULL) {
			perror("fopen : setting.txt");
			exit(1);
		}
		
		fprintf(wfp,"set=%d / cur=%d", set, cur);
		fclose(wfp);

		if((rfp=fopen("setting.txt","r"))==NULL) {
			perror("fopen : setting.txt");
			exit(1);
		}
		fgets(buf,BUFSIZ,rfp);
		temper=buf;
		fclose(rfp);

		if(strlen(date)>LINE_BUFF||strlen(temper)>LINE_BUFF)
		{
			printf("Input over 16 alphanumeric characters on a line!\n");
			return -1;
		}
		
		str_size=strlen(date);
		if(str_size>0) {
			strncat(string,date,str_size);
			memset(string+str_size,' ',LINE_BUFF-str_size);
		}

		str_size=strlen(temper);
		if(str_size>0) {
			strncat(string,temper,str_size);
			memset(string+LINE_BUFF+str_size,' ',LINE_BUFF-str_size);
		}

		write(dev_text,string,MAX_BUFF);	
	
		if(set<=cur) {
			wait=1;
			count=10;
		}
	
		if(on>0&&set>cur&&count==0) {
			motor_speed=(250-(set-cur)*10);
			if(motor_speed<0) {
				motor_speed=0;
			}
			motor_state[0]=1;
			motor_state[2]=motor_speed;
			d_cur+=(250-motor_speed)*0.002;
			cur=(int)d_cur;
			wait=0;
			usleep(100000);
		}

		if(wait) {
			motor_state[0]=0;
			d_cur-=0.5;
			cur=(int)d_cur;
			count--;
			usleep(100000);
		}
		write(dev_motor,motor_state,3);
		
		usleep(100000);
		read(dev_switch, &push_sw_buff, buff_size);
		if(push_sw_buff[0]==1) {
			on++;
			off=0;
			motor_state[0]=1;
			if(on==1) {
				printf("Turn on\n");
			}
			else {
				printf("Now working\n");
			}
		}
		if(push_sw_buff[1]==1) {
			if(set>=30) {
				printf("Reach max temperature(30) setting\n");
				continue;
			}
			set++;
		}
		if(push_sw_buff[2]==1) {
			if(set<=18) {
				printf("Reach min temperature(18) setting\n");
				continue;
			}
			set--;
		}
		if(push_sw_buff[3]==1) {
			off++;
			on=0;
			if(off==1) {
				printf("Turn off\n");
			}
			else {
				printf("Already stopped\n");
			}
			motor_state[0]=0;
		}
		if(push_sw_buff[4]==1) {
			printf("System end\n");
			motor_state[0]=0;
			break;
		}
	}
	close(dev_text);
	close(dev_switch);
	close(dev_motor);

	return(0);
}
