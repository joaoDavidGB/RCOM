/*
* Aplication Layer
*/
#include "applicationlayer.h"

#define MAX_FRAME_SIZE 100

char* filename; //file name
int filesize = 0; //file size
int fd;


int main(int argc, char** argv){


 filename = argv[1]; //Nome do ficheiro passado como argumento
 int file=0;
  if((file=open(filename,O_RDONLY)) < 0)
       return 0;


	printf("LEU O FICHEIRO!\n");

	
 struct stat fileStat;


  if(fstat(file,&fileStat) < 0){ 
	printf("DEU ASNEIRA FSTAT\n");   
        return 0;
  }


    filesize = fileStat.st_size;		
	if (filesize < 0){
	printf("DEU ASNEIRA file size\n");
	 return 0;
}

printf("file size %d\n", filesize);

int lengthDados = (MAX_FRAME_SIZE - 2 - 8 -4)/2; 
int numDataPack = filesize/lengthDados;


char* buf; //escrevemos sempre no mesmo buffer ele é sempre reescrito
int n1 = makeCONTROLpackage(buf,1);

if(n1!=0)
 return 0;

fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY);
unsigned char seqNumb = 0;
int i = 0;
for(i=0; i<numDataPack ; i++){
	
	char * dados;
	int res;
	if(res = read(file, dados, lengthDados) < 0)  //CONFIRMAR SE ESTA A LER PARA OS DADOS!!!!!
		return 0;
 	int suc = makeDATApackage(buf, seqNumb, res, dados);

	writeToFile(dados, buf);
	
	if(suc != 0)
	  return 0;
	
  }



  
int n2 = makeCONTROLpackage(buf,2);

if(n2!=0)
 return 0;

return 1;


}

// Cria control packages que são enviadas no antes e depois da transferência de dados
int makeCONTROLpackage(char* buf,int c){

	if (c == 1 || c == 2)
	  buf[0] = c; // pacote enviado no início (start) e no final (end)
	else
		return 0;

	//primeiro é enviado o tamanho e depois o nome
	buf[1] = 0;
	buf[2] = sizeof(filesize);
	memcpy(buf+3, &filesize, sizeof(filesize));

	
	buf[3 + sizeof(filesize)] = 1;
 	buf[4 + sizeof(filesize)] = sizeof(filename);
	memcpy(buf + 4 +sizeof(filesize), &filesize, strlen(filename));

	return 1;

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



int writeToFile(char* dados, char* buf){

  int res = write(fd,dados,  256 * buf[2] + buf[3]);

if(res == 0)
  return 0;

else return 1;





} 














