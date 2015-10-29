/*
* Application Layer .h
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <llfunctions.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>

int makeCONTROLpackage(char* buf,int c);
int makeDATApackage(char* buf,int seqNumb, int lengthDados, char* dados);