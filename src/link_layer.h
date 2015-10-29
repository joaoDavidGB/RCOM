#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
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
#define RR(N) (N<<5 | 1)
#define REJ(N) (N<<5 | 5)

struct Info {
  int fd; // descritor de ficheiro
  struct termios oldtio; 
  struct termios newtio;
  char endPorta[20]; /*Dispositivo /dev/ttySx, x = 0, 1*/
  
  int baudRate; /*Velocidade de transmissão*/
  unsigned int sequenceNumber; /*Número de sequência da trama: 0, 1*/
  unsigned int timeout; /*Valor do temporizador: 1 s*/
  unsigned int numTransmissions; /*Número de tentativas em caso de falha*/

  int flag;
  
  char * dados; /*dados a enviar/receber*/
  int lengthDados;
  int tentativas;

  char * frameTemp; // serve para guardar uma frame temporariamente
  int frameTempLength;
  char * frameSend;
  int frameSendLength;

};

//volatile int STOP=FALSE;

unsigned char SET[5];
unsigned char SET2[5];

struct Info * info;
int c, res;
char* buf; //file buffer
int bf;
char* filename; //file name
int filesize; //file size

//STATES
enum state {START, FLAG, A_STATE, C, BCC_STATE, STOP2};
//int estado = START;

int readFrame(char * frame);
char * verifyFrameType(char * frame);
int verifyFrame(char * frame, int length, char * type);
int llopen(char * porta, int flag);
void state_machine(int state, char signal, char * type);
int llopen_tramas(char * frame, int flag);
int sendFrame(int flag, char * type);
int campo_endereco(int role, int c);
int Is_cmd(int comand);
void comporPacotesControlo(int c);
void comporPacotesDados(int seqNumb, int sizeCampoI, int lengthDados, char* dados);
int transmitirFrame(char * frame, int length);
void atende(int sig);
int llwrite(int fd, char * buffer, int length);
int llread(int fd, char * buffer);
char * comporTramaI(int flag, char * buffer, int length);
int buildFrame(int flag, char * type);
void stuffing(unsigned char* frame, unsigned int* size);
void destuffing(unsigned char* frame, unsigned int* size);
