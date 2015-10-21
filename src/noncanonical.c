/*Non-Canonical Input Processing*/

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
#define FLAG_RCV 0x7E
#define A_RCV 0x03
#define C_RCV 0x07
#define BCC_RCV (A_RCV^C_RCV)
#define C_UA 0x03


volatile int STOP=FALSE;

unsigned char SET[5];

int fd,c, res;
//STATES

int estado;

enum state {START, FLAG, A, C, BCC, STOP2};


void state_machine(int state, char signal){
	printf("estado antes: %d \n", estado);

	if (state == START){
		if (signal == FLAG_RCV){
			state = FLAG;
			SET[0]=signal;
		}
	}
	else if (state == FLAG){
		if (signal == FLAG_RCV)
			state = FLAG;
		else if (signal == A_RCV){
			state = A;
			SET[1]=signal;
		}
		else 
			state = START;
	}
	else if (state == A){
		if (signal == FLAG_RCV){
			state = FLAG;
		}
		else if (signal == C_RCV || C_RCV == 1){
			state = C;
			SET[2]=signal;
		}
		else 
			state = START;
	}
	else if (state == C){
		if (signal == FLAG_RCV)
			state = FLAG;
		else if (signal == (SET[1]^SET[2])){
			state = BCC;
			SET[3]=signal;
		}
		else 
			state = START;
	}
	else if (state == BCC){
		if (signal == FLAG_RCV){
			state = STOP2;
			SET[4]=signal;
		}
		else
			state = START;
	}
	estado = state;
	printf("estado após: %d \n", estado);
	
}

void send_UA(){

	SET[0] = FLAG_RCV;
	SET[1] = A_RCV;	
	SET[2] = C_UA;
	SET[3] = SET[1]^SET[2];
	SET[4] = FLAG_RCV;

	int i = 0;
    	while(i < 5){
		res = write(fd,&SET[i],1); 
		i++;

	}
	i=0;
	printf("Send: 0x%x 0x%x 0x%x 0x%x 0x%x \n", SET[0], SET[1], SET[2], SET[3], SET[4]);
}

int receber(){
	char buf2;
   	int res2;

	estado = START;

	while(1){
		res2 = read(fd, &buf2, 1);
		printf("Received: %x !!! %d \n", buf2, res2);
		//state_machine(estado, buf2);
		state_machine(estado, buf2);
		if(estado == STOP2){
			return 1; 
			//send_SET();
			//res = write(fd,&SET[0],1);  
			//estado=START;

		}
	
	}
	return 0;	


}



int main(int argc, char** argv)
{
    struct termios oldtio,newtio;
    char buf[255];

    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS4", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }

  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */
      
    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    tcgetattr(fd,&oldtio); /* save current port settings */

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

    tcflush(fd, TCIFLUSH);
    tcsetattr(fd,TCSANOW,&newtio);

   	printf("New termios structure set\n");

	while(1){
		if(receber()){
			send_UA();
			break;		
		}
	}

/*    while (STOP==FALSE) {       /* loop for input */
/*	res = read(fd,buf,8);   /* returns after 5 chars have been input */
/*     	buf[res]=0;               /* so we can printf... */
/*     	if (buf[0]=='z') STOP=TRUE;
    }*/



    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
