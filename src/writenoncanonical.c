#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define F 0x7E
#define A 0x03
#define C_SET 0x07
#define BCC (A^C_SET)
#define C_UA 0x03

volatile int STOP=FALSE;
volatile int flag=FALSE;
unsigned char SET[5];
unsigned char SET2[5];

enum state {START2, FLAG2, A2, C2,BCC2, STOP2};

int estado = START2; 

int fd, res;
int tentativas = 0;
unsigned char UA[5];

int confirmar(){
	
    char buf2;
    int res2;
    int ecx=0;

    while(ecx <5){
	ecx++;
	while( !(res2 = read(fd, &buf2, 1)) )
		continue;
	printf("Received: %x !!! %d \n", buf2, res2);
	//state_machine(estado, buf2);
	state_machine(estado, buf2);
	if(estado == STOP2)
		return 1;
	
   }
	return 0;


}

void atende() // atende alarme
{
	if(!flag){
		send_SET();
	};
}
void send_SET(){
	int i = 0;
    while(i < 5){
	res = write(fd,&SET[i],1);  
	i++;
	}
	i=0;
	printf("Send: 0x%x 0x%x 0x%x 0x%x 0x%x \n", SET[0], SET[1], SET[2], SET[3], SET[4]);
}

       
void state_machine(int state, char signal){
        printf("estado antes: %d \n", estado);
 
        if (state == START2){
                if (signal == F){
                        state = FLAG2;
                        UA[0]=signal;
                }
        }
        else if (state == FLAG2){
                if (signal == F)
                        state = FLAG2;
                else if (signal == A){
                        state = A2;
                        UA[1]=signal;
                }
                else
                        state = START2;
        }
        else if (state == A2){
                if (signal == F){
                        state = FLAG2;
                }
                else if (signal == C_UA){
                        state = C2;
                        UA[2]=signal;
                }
                else
                        state = START2;
        }
        else if (state == C2){
                if (signal == F)
                        state = FLAG2;
                else if (signal == (UA[1]^UA[2])){
                        state = BCC2;
                        UA[3]=signal;
                }
                else
                        state = START2;
        }
        else if (state == BCC2){
                if (signal == F){
                        state = STOP2;
                        UA[4]=signal;
                }
                else
                        state = START2;
        }
        estado = state;
        printf("estado após: %d \n", estado);
       
}

int main(int argc, char** argv)
{
    struct termios oldtio,newtio;
    char buf[255];

    (void) signal(SIGALRM, atende);

    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS4", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }

    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
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
      exit(-1);
    }

	SET[0] = F;
	SET[1] = A;	
	SET[2] = C_SET;
	SET[3] = A^C_SET;
	SET[4] = F;

	send_SET();
	
	while(!confirmar() && tentativas < 3){
		alarm(3);
		tentativas++;
		send_SET();
	}

	
	int i = 0;


	flag = TRUE;

	printf("Recieve: 0x%x 0x%x 0x%x 0x%x 0x%x \n", UA[0], UA[1], UA[2], UA[3], UA[4]);

    sleep(5);
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
    close(fd);
    return 0;
}

