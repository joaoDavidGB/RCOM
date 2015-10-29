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

int makeCONTROLpackage(char* buf,int c);
int makeDATApackage(char* buf,int seqNumb, int lengthDados, char* dados);
int writeToFile(char* dados, char* buf);
int writeToFile2(char* dados);