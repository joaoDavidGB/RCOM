/*
* Aplication Layer
*/
#include "applicationlayer.h"

#define MAX_FRAME_SIZE 100

char* filename; //file name
int filesize = 0; //file size
int fd;
int lengthDados2;


int main(int argc, char** argv){

	filename = argv[1]; //Nome do ficheiro passado como argumento
	printf("ficheiro aberto: %s \n", filename);
 	int file=0;
 	if((file=open(filename,O_RDONLY,0666)) < 0)
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
 	int numDataPack = (int)(((float)filesize)/lengthDados+.5);

	char* buf; //escrevemos sempre no mesmo buffer ele é sempre reescrito
	int n1 = makeCONTROLpackage(buf,1);

	if(n1==0)
		return 0;

	printf("cenas\n");
	fd = open("cenas2.txt", O_CREAT | O_TRUNC | O_WRONLY, 0666);
	if(fd < 0 ){
		printf("DEU ASNEIRA\n");
		return 0;
	}
	printf("cenas2\n");
	unsigned char seqNumb = 0;
	int i = 0;
	char * dados = malloc(100);
	printf("numDataPack = %d \n", numDataPack);
	for(i=0; i<numDataPack ; i++){
		int res;
		fprintf(stderr, "lengthDados = %d \n", lengthDados);

		do{res = read(file, dados, lengthDados); }while(res == 0); //CONFIRMAR SE ESTA A LER PARA OS DADOS!!!!!
			

		fprintf(stderr, "Dados = %x | res = %d \n", dados[0], res);

		lengthDados2 = res;

		//int suc = makeDATApackage(buf, seqNumb, res, dados);

		//writeToFile(dados, buf);
		writeToFile2(dados);


		//if(suc != 0)
			//return 0;
	}
/*
	int n2 = makeCONTROLpackage(buf,2);

	if(n2!=0)
		return 0;
*/
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
	//int res = write(fd,dados,  256 * buf[2] + buf[3]);
	int res = write(fd,dados,  lengthDados2);

	if(res == 0)
		return 0;

	else return 1;
} 

int writeToFile2(char* dados){
	//int res = write(fd,dados,  256 * buf[2] + buf[3]);
	int res = write(fd,dados, lengthDados2);

	if(res == 0)
		return 0;

	else return 1;
} 