#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>

int fd, control;

// SEND THE CONTROL PACKAGE
void send_CONTROL_pck(int arg, int tamanho, char nome){
    if (arg == 1){ // start package
	int t = 0;
	control = write(fd, arg, 1); // enviar o C

	/* ENVIO DO TAMANHO DO FICHEIRO (T1) */
	control = write(fd, t, 1); // enviar o T1
	control = write(fd, sizeof(tamanho), 1); // enviar o tamanho em octetos do valor
	unsigned int i = 0;
	while(i < sizeof(tamanho)){ // enviar o V
	     control = write(fd, tamanho, sizeof(tamanho));
	     ++i;
	}

	/* ENVIO DO NOME DO FICHEIRO (T2) */
	control = write(fd, t++, 1); // enviar o T2
	control = write(fd, sizeof(nome), 1); // enviar o tamanho do nome do ficheiro
	int k = 0;
	while(k < sizeof(nome)){
		control = write(fd, &nome, sizeof(nome)); // enviar o nome do ficheiro
		++k;
	}

    }
    else if (arg == 2){ // end package
	
    }
    else{ // error case
        printf("control package argument invalid");
    }
}

void send_DATA(){

}

/* STUFFING */
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

/* DESTUFFING */
void destuffing(unsigned char* frame, unsigned int* size){
	for(int i = 1; i < (*size-1); ++i){
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

int main(int argc, char** argv){
		
    
    //send_CONTROL_pck(1, 1, 'A');

}
