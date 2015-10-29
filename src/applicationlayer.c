/*
* Aplication Layer
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

#define MAX_FRAME_SIZE 100

char* filename; //file name
int filesize; //file size

int main(int argc, char** argv){

 filename = argv[2]; //Nome do ficheiro passado como argumento
 int file=0;
  if((file=open(filename,O_RDONLY)) < -1)
       return 1;

 struct stat fileStat;
  if(fstat(file,&fileStat) < 0)    
        return 1;

 filesize = fileStat.st_size;	
	
	if (fileLength == -1)
	 return 1;

int lengthDados = (MAX_FRAME_SIZE - 2 - 8 -4)/2;
int numDataPack = filesize/lengthDados;


char* buf; //escrevemos sempre no mesmo buffer ele é sempre reescrito
int n1 = makeCONTROLpackage(buf,1);

if(n1!=0)
 return 1;

unsigned char seqNumb = 0;
int i = 0;
for(i=0; i<numDataPack ; i++){
	
	char * dados;
	int res;
	if(res = read(file, dados, lengthDados) < 0)  //CONFIRMAR SE ESTA A LER PARA OS DADOS!!!!!
		return 1;
 	int suc = makeDATApackage(buf, seqNumb, res, dados);
	
	if(suc != 0)
	  return 1;
	
  }
  
int n2 = makeCONTROLpackage(buf,2);

if(n2!=0)
 return 1;

return 0;


}


// Cria control packages que são enviadas no antes e depois da transferência de dados
int makeCONTROLpackage(char* buf,int c){

	if (c == 1 || c == 2)
	  buf[0] = c; // pacote enviado no início (start) e no final (end)
	else
		return 1;

	//primeiro é enviado o tamanho e depois o nome
	buf[1] = 0;
	buf[2] = sizeof(filesize);
	memcpy(buf+3, &filesize, sizeof(filesize));

	
	buf[3 + sizeof(filesize)] = 1;
 	buf[4 + sizeof(filesize)] = sizeof(filename);
	memcpy(buf + 4 +sizeof(filesize), &filesize, strlen(filename));

	return 0;

}


// Cria data package que envia o ficheiro
int makeDATApackage(char* buf,int seqNumb, int lengthDados, char* dados){

 buf[0] = 0;
 buf[1] = seqNumb;
 buf[2] = lengthDados/256;  
 buf[3] = lengthDados%256;
  int i;
  for(i=0; i < lengthDados; i++){

 	buf[4+i] = dados[i];

  }	

}
