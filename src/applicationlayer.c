/*
* Aplication Layer
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

/* STUFFING */
void stuffing(unsigned char* frame, unsigned int* size){
	for (int i = 1; i < (*size-1); i++){
	    if (frame[i] == 0x7e){
		frame[i] = 0x7d;
		memmove(frame + i+2,frame+i+1,*size-i-1); 
		frame[i++] = 0x5e;
		(*size)++;
	    } 
	    else if (frame[i] == 0x7d){
		frame[i] = 0x7d;
		memmove(frame + i + 2,frame + i + 1,*size-i-1); 
		frame[i++] = 0x5d;
		(*size)++;
		}
	}
}

/* DESTUFFING */
void destuffing(unsigned char* frame, unsigned int* size){
	for(int i = 1; i < (*size-1); ++i){
		if(frame[i] == 0x7d && frame[i++] == 0x5e){
		   frame[i] = 0x7e;
		   memmove(frame + i + 1, frame + i + 2, *size-i-1);
		   (*size--);
		}
		else if (frame[i] == 0x7d && frame[i++]== 0x5d){
			frame[i] = 0x7d;
			memmove(frame + i + 1, frame + i + 2, *size-i-1);
			(*size--);
		}
	}
}

// Cria control packages que são enviadas no antes e depois da transferência de dados
int setCONTROLpackage(int type, char* package, char* filename, FILE* file){

	if (type == 1)
		package[0] = 0x01; // pacote enviado no início (start)
	else if (type == 2)
		package[0] = 0x02; // pacote enviado no final (end)
	else
		return 0;

	package[1] = 0x00; // campo a ser enviado é o tamanho do ficheiro

	long fileLength;
	fseek(file, 0L, SEEK_END);
	fileLength = ftell(file); // tamanho em bytes do ficheiro

	if (fileLength == -1L)
		return 0;

	package[2] = sizeof(fileLength); // T1

	memcpy(package + 3, &fileLength, sizeof(fileLength)); // V1 = tamanho do ficheiro

	package[3 + sizeof(fileLength)] = 0x01; // campo a ser enviado é o nome do ficheiro

	int n = 3 + sizeof(fileLength)] + 1;

	package[n] = strlen(filename); // T2

	memcpy(package + n + 1, filename, strlen(filename)); // V2 = nome do ficheiro

	return 1;

}

// Cria data package que envia o ficheiro
void setDATApackage(char* package, int n, int dataLength, char* data){

	package[0] = 0x00;

	package[1] = n; // nr de sequência

	package[2] = dataLength/256; 

	package[3] = dataLength%256;

	int k = 4;

	for (int i = 0; i < dataLength; i++){

		package[k] = data[i];
		k++; 

	}

}