#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define F 0x7E
#define A 0x03
#define C_SET 0x07
#define BCC (A^C_SET)
#define C_UA 0x03
#define C_DISC 0x0B

struct Info {
  int fd;
  struct termios oldtio;
  struct termios newtio;
  char endPorta[20];

}info;


int llopen(int porta, int flag);
int llclose(int fd);
void state_machine(int state, char signal, char type);
int trasmitirSET(int flag, char * type);
int receberSET(int flag, char * type);


volatile int STOP=FALSE;

unsigned char SET[5];
unsigned char SET2[5];

int c, res;
//STATES
enum state {START, FLAG, A_STATE, C, UA, BCC_STATE, STOP2};
int estado = START;

int main(int argc, char** argv){
  if (strcmp("0", argv[1])==0)
    llopen(atoi(argv[2]), 0);
  else if (strcmp("1", argv[1])==0)
    llopen(atoi(argv[2]), 1);
}

int llopen(int porta, int flag){

  sprintf(info.endPorta, "/dev/ttyS%d", porta);
  info->fd = open(info->endPorta, O_RDWR | O_NOCTTY);
  if (info->fd < 0) {perror(endPorta); exit(-1);}

  if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
    perror("tcgetattr");
    return -1;
  }

  bzero(&newtio, sizeof(newtio));

  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = OPOST;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
  newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */

  tcflush(fd, TCIFLUSH);

  if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
    perror("tcsetattr");
    return -1;
  }

  int tentativas = 3;

  if (flag == 0){
    if(receberSET(flag, "set")==1)
      transmitirSET(flag, "ua");
    else
      return -1;
  }
  else{
    while(tentativas > 0){
      transmitirSET(flag, "set");
      alarm(3);
      if (receberSET(flag, "ua") != 1)
        tentativas--;
      else{
        alarm(0);
        break;
      }
    }
  }


  return fd;
}

int llclose_transmitter(int fd){

    transmitirSET(1, "disc");
    if (receberSET(1, "disc") == 1)
      transmitirSET(1, "ua");
    else
      return -1;

    sleep(5);

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      return -1;
    }
    close(fd);
    return 1;
}

int llclose_receiver(int fd){

    if (receberSET(1, "disc") == 1){
      transmitirSET(1, "disc");
      if (receberSET(1, "ua") == 1){
        return -1;
      }
    }
    else
      return -1;

    sleep(5);

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      return -1;
    }
    close(fd);
    return 1;
}


int transmitirSET(int flag, char * type){
  SET[0] = F;
  SET[1] = A;
  if (type == "set") 
    SET[2] = C_SET;
  else if (type == "ua")
    SET[2] = C_UA;
  else if (type == "disc")
    SET[2] = C_DISC;
  SET[3] = SET[1]^SET[2];
  SET[4] = F;

  int i = 0;
  while(i < 5){
    res = write(fd,&SET[i],1);  
    i++;
  }
  i=0;
  printf("Send SET: 0x%x 0x%x 0x%x 0x%x 0x%x \n", SET[0], SET[1], SET[2], SET[3], SET[4]);
  
  return 0;
}

int receberSET(int flag, char * type){
  char buf2;
  int res2;

  int i = 0;
  while(i < 5){
    if (i == 0){
      while((res2 = read(fd, &buf2, 1))==0 && buf2!=F)
	     continue;
    }
    else
	    res2 = read(fd, &buf2, 1);

    printf("Received: %x !!! %d \n", buf2, res2);
    i++;
    printf("i = %d\n", i);
    
    state_machine(estado, buf2);
    if(estado == STOP2){
      return 1; 
    }
  
  }
  return 0;
}

void state_machine(int state, char signal, char type){
       
 
        if (state == START){
                if (signal == F){
                        state = FLAG;
                        SET2[0]=signal;
                }
        }
        else if (state == FLAG){
                if (signal == F)
                        state = FLAG;
                else if (signal == A){
                        state = A_STATE;
                        SET2[1]=signal;
                }
                else
                        state = START;
        }
        else if (state == A_STATE){
                if (signal == F){
                        state = FLAG;
                }
                else if ((signal == C_SET && type == "set")
                  || (signal == C_UA && type == "ua")
                  || (signal == C_DISC && type == "disc")){
                        state = C;
                        SET2[2]=signal;
                }
                else
                        state = START;
        }
        else if (state == C){
                if (signal == F)
                        state = FLAG;
                else if (signal == (SET2[1]^SET2[2])){
                        state = BCC_STATE;
                        SET2[3]=signal;
                }
                else
                        state = START;
        }
        else if (state == BCC_STATE){
                if (signal == F){
                        state = STOP2;
                        SET2[4]=signal;
                }
                else
                        state = START;
        }
        estado = state;
       
        printf("estado: %d \n", estado);
}

