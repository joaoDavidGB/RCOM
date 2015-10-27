#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <signal.h>
#include "alarme.h"

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
#define C_I0 0x0
#define C_I1 0x20
#define TRANSMITTER 1
#define RECEIVER 0

struct Info {
  int fd; // descritor de ficheiro
  struct termios oldtio; //
  struct termios newtio;
  char endPorta[20]; /*Dispositivo /dev/ttySx, x = 0, 1*/
  
  int baudRate; /*Velocidade de transmissão*/
  unsigned int sequenceNumber; /*Número de sequência da trama: 0, 1*/
  unsigned int timeout; /*Valor do temporizador: 1 s*/
  unsigned int numTransmissions; /*Número de tentativas em caso de falha*/
  
  char frame[255]; /*Trama*/

};


int llopen(int porta, int flag);
int llclose(int fd);
int llclose_transmitter(int fd);
int llclose_receiver(int fd);
void state_machine(int state, char signal, char * type);
int trasmitirSET(int flag, char * type);
int receberSET(int flag, char * type);
int llwrite(int fd, char * buffer, int length);
int llread(int fd, char * buffer);
int Is_cmd(int comand);
int campo_endereco(int role, int c);
int transmitirFrame(char * frame, int length);
void stuffing(unsigned char* frame, unsigned int* size);
void destuffing(unsigned char* frame, unsigned int* size);
char * comporTramaI(int flag, char * buffer, int length);
char * receberI(int flag);


volatile int STOP=FALSE;

unsigned char SET[5];
unsigned char SET2[5];

struct Info * info;
int c, res;
//STATES
enum state {START, FLAG, A_STATE, C, UA, BCC_STATE, STOP2};
int estado = START;

int main(int argc, char** argv){
  info = malloc(sizeof(struct Info));
  info->sequenceNumber = 0;
  if (strcmp("0", argv[1])==0){
    llopen(atoi(argv[2]), RECEIVER);
    char * teste;
    teste[0] = 0x11;
    teste[1] = 0x22;
    teste[2] = 0x05;
    llwrite(info->fd, teste, 3);
    llclose_receiver(info->fd);
  }
  else if (strcmp("1", argv[1])==0){
    llopen(atoi(argv[2]), TRANSMITTER);
    char * result;
    llread(info->fd, result);
    printf("Result: %s /n", result);
    llclose_transmitter(info->fd);
  }
}

int llopen(int porta, int flag){

  sprintf(info->endPorta, "/dev/ttyS%d", porta);
  info->fd = open(info->endPorta, O_RDWR | O_NOCTTY);
  if (info->fd < 0) {perror(info->endPorta); exit(-1);}

  if ( tcgetattr(info->fd,&info->oldtio) == -1) { // save current port settings 
    perror("tcgetattr");
    return -1;
  }

  bzero(&info->newtio, sizeof(info->newtio));

  info->newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  info->newtio.c_iflag = IGNPAR;
  info->newtio.c_oflag = OPOST;

  // set input mode (non-canonical, no echo,...) 
  info->newtio.c_lflag = 0;

  info->newtio.c_cc[VTIME]    = 0;   // inter-character timer unused 
  info->newtio.c_cc[VMIN]     = 1;   // blocking read until 5 chars received 

  tcflush(info->fd, TCIFLUSH);

  if ( tcsetattr(info->fd,TCSANOW,&info->newtio) == -1) {
    perror("tcsetattr");
    return -1;
  }

  int tentativas = 3;

  if (flag == RECEIVER){
    if(receberSET(flag, "set")==1)
      transmitirSET(flag, "ua");
    else
      return -1;

    //llclose_receiver(info->fd);
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
    //llclose_transmitter(info->fd);
  }
  
  return info->fd;
}

int llclose_transmitter(int fd){

    transmitirSET(1, "disc");
    if (receberSET(1, "disc") == 1)
      transmitirSET(1, "ua");
    else
      return -1;

    sleep(5);

    if ( tcsetattr(info->fd,TCSANOW,&info->oldtio) == -1) {
      perror("tcsetattr");
      return -1;
    }
    close(fd);
    printf("fechou transmissor\n");
    return 1;
}

int llclose_receiver(int fd){

    if (receberSET(1, "disc") == 1){
      transmitirSET(1, "disc");
      if (receberSET(1, "ua") != 1){
        return -1;
      }
    }
    else
      return -1;

    sleep(5);

    if ( tcsetattr(info->fd,TCSANOW,&info->oldtio) == -1) {
      perror("tcsetattr");
      return -1;
    }
    close(fd);
    printf("fechou recetor\n");
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
    res = write(info->fd,&SET[i],1);  
    i++;
  }
  i=0;
  printf("Send %s: 0x%x 0x%x 0x%x 0x%x 0x%x \n", type, SET[0], SET[1], SET[2], SET[3], SET[4]);
  
  return 0;
}

int receberSET(int flag, char * type){
  char buf2 = 0;
  int res2;
  estado = START;
  int i = 0;
  while(i < 5){
    if (i == 0){
      while((res2 = read(info->fd, &buf2, 1))==0 && buf2!=F)
	     continue;
    }
    else
	    while((res2 = read(info->fd, &buf2, 1))==0)
       continue;

    printf("Received: %x !!! %d \n", buf2, res2);
    i++;
    //printf("i = %d\n", i);
    
    state_machine(estado, buf2, type);
    if(estado == STOP2){
      estado = START;
      return 1; 
    }
  
  }
  estado = START;
  return 0;
}

char * receberI(int flag){
  char * dados;
  char buf2 = 0;
  int res2;
  int i;
  estado = START;
  for (i = 0; i < 4; i++){
    if (i == 0){
      while((res2 = read(info->fd, &buf2, 1))==0 && buf2!=F)
       continue;
    }
    else
      while((res2 = read(info->fd, &buf2, 1))==0)
       continue;

     printf("ReceivedI: %x !!! %d \n", buf2, res2);
     state_machine(estado, buf2, "I");
  }
  if (estado != BCC_STATE)
    return "fail";

  char BBC2 = 0;
  i = 0;
  buf2 = 1;
  while(BBC2 != buf2){
    while((res2 = read(info->fd, &buf2, 1))==0)
      continue;
    BBC2 = BBC2^buf2;
    dados[i] = buf2;
    if (i == 0)
      buf2 = 1;
    i++;
  }

  while((res2 = read(info->fd, &buf2, 1))==0)
    continue;
  state_machine(estado, buf2, "I");
  if (estado == STOP2)
    return dados;
  else
    return "fail";

}

void state_machine(int state, char signal, char * type){
       
 
        if (state == START){
                if (signal == F){
                        state = FLAG;
                        SET2[0]=signal;
                }
        }
        else if (state == FLAG){
                if (signal == F)
                        state = FLAG;
                else if ((signal == A)
                  || (signal == campo_endereco(flag, info->sequenceNumber) && type == "I")){
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
                  || (signal == C_DISC && type == "disc")
                  || (signal == RR(0) && type == "rr1")
                  || (signal == RR(1) && type == "rr0")
                  || (info->sequenceNumber && type == "I")){
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

int llwrite(int fd, char * buffer, int length){
  char * tramaI;
  strcpy(tramaI, comporTramaI(TRANSMITTER, buffer, length));
  transmitirFrame(tramaI, length);
  alarm(3);
  if (info->sequenceNumber == 1){
    if (receberSET(TRANSMITTER, "rr1"))
      alarm(0);
  }
  else if (info->sequenceNumber == 0){
    if (receberSET(TRANSMITTER, "rr0"))
      alarm(0);
  }

}

int llread(int fd, char * buffer){
  char * tramaI;
  tramaI = receberI(RECEIVER);
  if (tramaI == "fail"){
    //enviar frame REJ
    return 0;
  }
  printf("tramaI: %s \n", tramaI);
  buffer = tramaI;
  char * rrtype;
  sprintf(rrtype, "rr%d", info->sequenceNumber+1);
  transmitirSET(RECEIVER, rrtype);
  return 1;
}

char * comporTramaI(int flag, char * buffer, int length){
  char * trama;
  int index;
  trama[0] = F;
  trama[1] = campo_endereco(flag, info->sequenceNumber);
  trama[2] = info->sequenceNumber;
  trama[3] = trama[1]^trama[2];
  trama[4 + length] = 0;
  for(index = 0; index < length; index++){
    /*
      ADICIONAR STUFFING E DESTUFFING
    */
    trama[4 + index] = buffer[index];
    trama[4 + length] = trama[4 + length]^trama[4 + index];
  }
  trama[4 + length + 1] = F;

  return trama;
}

/*
//DESTUFFING feito pela Filipa
void destuffing(unsigned char* frame, unsigned int* size){
    if(frame[i] == 0x7d && frame[i++] == 0x5e){
       frame[i] = 0x7e;
       memcpy(frame + i + 1, frame + i + 2, *size-i-1);
      (*size--);
    }
    else if (frame[i] == 0x7d && frame[i++]== 0x5d){
      frame[i] = 0x7d;
      memcpy(frame + i + 1, frame + i + 2, *size-i-1);
     (*size--);
    }
  }
}

//STUFFING feito pela Filipa
void stuffing(unsigned char* frame, unsigned int* size){
  for (int i = 1; i < (*size-1); i++){
      if (frame[i] == 0x7e){
    frame[i] = 0x7d;
    memcpy(frame + i+2,frame+i+1,*size-i-1); 
    frame[i++] = 0x5e;
    (*size)++;
      } 
      else if (frame[i] == 0x7d){
    frame[i] = 0x7d;
    memcpy(frame + i + 2,frame + i + 1,*size-i-1); 
    frame[i++] = 0x5d;
    (*size)++;
    }
  }
 }

*/
int transmitirFrame(char * frame, int length){
  int i;
  printf("Enviar frame: ");
  for(i = 0; i < length; i++){
    res = write(info->fd,&frame[i],1);
    printf("0x%x", frame[i]);
  }
  printf("/n");
}

int RR(int N){
  return (N<<5 | 1);
}

int REJ(int N){
  return (N<<5 | 5);
}

int campo_endereco(int role, int c){
  if (role == TRANSMITTER){
    if (Is_cmd(c))
      return 0x03;
    else
      return 0x01;
  }
  else if (role == RECEIVER){
    if (Is_cmd(c))
      return 0x01;
    else
      return 0x03;
  }

  printf("fail no campo_endereco \n");
}

int Is_cmd(int comand){
  if (comand == C_I0 || comand == C_I1 || comand == C_SET || comand == C_DISC)
    return 1;
  else
    return 0;

}