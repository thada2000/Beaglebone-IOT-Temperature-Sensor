//NAME: TANMAYA HADA
//EMAIL: tanmaya2000@hotmail.com
//ID:304925920

#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<errno.h>
#include<time.h>
#include<sys/time.h>
#include<getopt.h>
#include<unistd.h>
#include<string.h>
#include "mraa.h"
#include "mraa/aio.h"
#include<poll.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>

int quit =0;
void intrptFunc(){
  quit=1;
}

int main (int argc, char** argv){
  int stopped = 0;
  char scale = 'F';
  char* logfile =NULL;
  int period = 1;
  FILE* logf;
  opterr=0;
  char* id;
  char* hostname;
  static struct option options[]={
    {"scale", optional_argument,0,1},
    {"period",optional_argument,0,2},
    {"log",required_argument,0,3},
    {"id",required_argument,0,4},
    {"host",required_argument,0,5},
    {0,0,0,0}
  };
  while(1){
    int opt = getopt_long(argc,argv,"",options,NULL);
    if(opt == -1)
      break;
    switch (opt){
    case 1:
      if(optarg != 0){
        if(strlen(optarg)!=1){
	  fprintf(stderr,"Error: Bad argument!\n");
	  exit(1);
	}
	scale=optarg[0];
      }
      break;
    case 2:
      if(optarg != 0){
	period = atoi(optarg);
      }
      break;
    case 3:
      if(optarg != 0){
	logfile = optarg;
      }
      break;
    case 4:
      id = optarg;
	break;
    case 5:
      hostname = optarg;
	break;
    case '?':
      fprintf(stderr,"Error: missing or bad argument!\n");
      exit(1);
      break;
    }
  }
  if(optind == argc){
	fprintf(stderr,"Error: missing port number!\n");
	exit(1);
  }
  int socketD = socket(AF_INET,SOCK_STREAM,0);
  if(socketD < 0){
	fprintf(stderr,"Error: socket opening failed!!\n");
	exit(1);
  }
  struct hostent* serv = gethostbyname(hostname);
  if(serv == NULL){
	fprintf(stderr, "Error: getting host name failed!\n");
	exit(1);
  }
  struct sockaddr_in address;
  memset(&address,0,sizeof(struct sockaddr_in));
  memcpy(&address.sin_addr,serv->h_addr,serv->h_length);
  address.sin_family= AF_INET;
  address.sin_port = htons(atoi(argv[optind]));
  int c = connect(socketD, (struct sockaddr*)&address, sizeof(struct sockaddr_in));
  if(c < 0 ){
	fprintf(stderr, "Error: Failed to establish successful connection!\n");
	exit(1);
  }
  mraa_gpio_context button;
  button = mraa_gpio_init(60);
  if(button == NULL){
    fprintf(stderr,"Error: failed to init button!\n");
    exit(1);
  }
  mraa_gpio_dir(button,MRAA_GPIO_IN);
  mraa_gpio_isr(button,MRAA_GPIO_EDGE_RISING,&intrptFunc,NULL);
  //open logfile to append to
  int fdlog=0;
  if(logfile != NULL){
    logf = fopen(logfile,"w");
    if(logf == NULL){
	fprintf(stderr,"Error:\n");
	exit(1);
    }
  }
  //perform analog read
  mraa_aio_context tempIn;
  tempIn = mraa_aio_init(1);
  if(tempIn == NULL){
    fprintf(stderr,"Error: failed to init aio context!\n");
    mraa_gpio_close(button);
    exit(1);
  }
  
  struct pollfd fds[1];
  fds[0].fd=socketD;
  fds[0].events=POLLIN;
  fds[0].revents=0;
  
  fprintf(logf,"ID=%s\n",id);
  char str[20];
  memset(str,0,20);
  sprintf(str,"ID=%s\n",id);
  if((write(socketD,str,strlen(str)))<0){
	fprintf(stderr,"Error: failed write!\n");
	exit(1);
  }
  time_t oldtime;
  time(&oldtime);
  while(!quit){
    if(!stopped){
	time_t currtime;
	time(&currtime);
      if(currtime-oldtime>=period){
	time(&oldtime);
	int reading;
	reading = mraa_aio_read(tempIn);
	if(reading == -1){
	  fprintf(stderr,"Error: bad reading!\n");
	  mraa_aio_close(tempIn);
	  mraa_gpio_close(button);
	  exit(1);
	}
	//convert reading
	float R= 1023.0/reading-1.0;
	R=100000 * R;
	float temp = 1.0/(log(R/100000)/4275+1/298.15)-273.15;
	float tempf= (temp*9)/5 +32;
	time_t timeValue;
	time(&timeValue);
	struct tm* localT = localtime(&timeValue);
	char* timeStr = malloc(20);
	strftime(timeStr,20,"%H:%M:%S",localT);
	float outtemp;
	if (scale == 'C')
	  outtemp = temp;
	else outtemp = tempf;
	char buf[15];
  	memset(buf,0,15);
	sprintf(buf,"%s %.1f\n",timeStr,outtemp);
	write(socketD,buf,strlen(buf));
	if(logfile != NULL){
	  fprintf(logf,"%s %.1f\n",timeStr,outtemp);
	  fflush(logf);
	}
	free(timeStr);
      }
    }

      int p = poll(fds,1,0);
      if(p<0){
	fprintf(stderr,"Error:poll failed!\n");
	close(fdlog);
	mraa_aio_close(tempIn);
	mraa_gpio_close(button);
	exit(1);
      }
      char stdinBuff[256];
      memset((char*)&stdinBuff,0,256);
      if(p>0){
	int bytesread=read(socketD,&stdinBuff,256);
	char* inputCommand = strtok(stdinBuff,"\n");
	while(inputCommand != NULL && bytesread>0){
	 int val;
	 if((val=strncmp(inputCommand,"SCALE=F",strlen(inputCommand)))==0){
	    scale = 'F';
	 }
	 else if((val=strncmp(inputCommand,"SCALE=C",strlen(inputCommand)))==0){
	   scale = 'C';
	 }
	 else if((val=strncmp(inputCommand,"START",strlen(inputCommand)))==0){
	   stopped=0;
	 }
	 else if((val=strncmp(inputCommand,"STOP",strlen(inputCommand)))==0){
	   stopped=1;
	 }
	 else if((val=strncmp(inputCommand,"OFF",strlen(inputCommand)))==0){
	   quit=1;
	 }
	 else{
	   if(strncmp(inputCommand,"PERIOD=",7)==0){
	     period = atoi(&inputCommand[7]);
	   }
	 }
	 if(logfile != NULL){
	   fprintf(logf,"%s\n",inputCommand);
	 }
	inputCommand = strtok(NULL, "\n");
       }
	}
   }

  time_t timeValue;
  time(&timeValue);
  struct tm* localT = localtime(&timeValue);
  char* timeStr=malloc(50);
  strftime(timeStr,10,"%H:%M:%S",localT);
  char buf1[19];
  memset(buf1,0,19);
  sprintf(buf1,"%s SHUTDOWN\n",timeStr);
  write(socketD,buf1,strlen(buf1));
  if(logfile != NULL){
	fprintf(logf,"%s SHUTDOWN\n",timeStr);
      }
  free(timeStr);
  mraa_gpio_close(button);
  mraa_aio_close(tempIn);
  exit(0);
}
