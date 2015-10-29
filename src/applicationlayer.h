/*
* Application Layer .h
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include "link_layer.h"

struct apllicationLayer {
  	int fd; // descritor de ficheiro
  	int flag; /*TRANSMITTER | RECEIVER*/

  	char* filename; //file name
  	int filesize = 0; //file size
	int lengthDados;
	char* buf; //escrevemos sempre no mesmo buffer ele é sempre reescrito
	int numDataPack;
	unsigned char seqNumb;
};

int makeCONTROLpackage(char* buf,int c);
int makeDATApackage(char* buf,int seqNumb, int lengthDados, char* dados);
int writeToFile(char* dados, char* buf);
