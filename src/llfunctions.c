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

int llopen(int porta, int flag);
void state_machine(int state, char signal);
int trasmitirSET();
int receberSET();


volatile int STOP=FALSE;

unsigned char SET[5];
unsigned char SET2[5];

int fd,c, res;
//STATES
enum state {START, FLAG, A_STATE, C, UA, BCC_STATE, STOP2};
int estado = START;

int main(int argc, char** argv){
  if (strcmp("0", argv[1])==0)
    llopen(0, 0);
  else if (strcmp("1", argv[1])==0)
    llopen(4, 1);
}

int llopen(int porta, int flag){
  struct termios oldtio,newtio;

  char * endPorta = "/dev/ttyS" + porta;
  fd = open(endPorta, O_RDWR | O_NOCTTY);
  if (fd < 0) {perror(endPorta); exit(-1);}

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

  if (flag = 0){
    if(receberSET(flag)==1)
      transmitirSET(flag);
    else
      return -1;
  }
  else{
    while(tentativas > 0){
      transmitirSET(flag);
      if (receberSET(flag) != 1)
        tentativas--;
    }
  }


  return fd;
} 


int transmitirSET(int flag){
  SET[0] = F;
  SET[1] = A;
  if (flag == 1) 
    SET[2] = C_SET;
  else
    SET[2] = C_UA;
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

int receberSET(int flag){
  char buf2;
  int res2;

  int i = 0;
  while(i < 5){
    if (i = 0)
      while(!(res2 = read(fd, &buf2, 1)) && buf2!=F)
        continue;

    printf("Received: %x !!! %d \n", buf2, res2);
    
    state_machine(estado, buf2);
    if(estado == STOP2){
      return 1; 
    }
  
  }
  return 0;
}

void state_machine(int state, char signal){
        printf("estado antes: %d \n", estado);
 
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
                else if (signal == C_SET || signal == C_UA){
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
        printf("estado após: %d \n", estado);
       
}