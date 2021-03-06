/*
* Aplication Layer
*/
#include "applicationlayer.h"

#define MAX_FRAME_SIZE 100

int porta;

int main(int argc, char** argv){

	if(argc != 2){
		printf("numero de argumentos errado. \n");
		printf("%s (porta(/dev/ttySN)) \n", argv[0]);
		return 0;
	}
	appLayer = malloc(sizeof(struct applicationLayer));
	appLayer->porta = argv[1];
	appLayer->buf = malloc(1000); //escrevemos sempre no mesmo buffer ele é sempre reescrito

	int alterouOption = 0;
	while(1){
		printf("*****MENU*****\n");
		printf("1) Transmitir\n");
		printf("2) Receber\n");
		printf("3) Settings\n");
		printf("4) Exit\n");

		int option = 0;
		do{
			scanf("%d", &option);
		} while(option < 1 || option > 4);

		if (option == 4)
			return 0;
		else if (option == 3){
			changeSettings();
			alterouOption = 1;
			continue;
		}
		else{
			if (!alterouOption)
				Settings(3, 3, 38400, 43);
			if(option == 1)
				appLayer->flag = TRANSMITTER;
			else if (option == 2)
				appLayer->flag = RECEIVER;

			break;
		}
	}

	if (appLayer->flag == TRANSMITTER){
		char fileC[20];
		printf("File: ");
		scanf("%s", fileC);
	
		appLayer->filename = fileC; //Nome do ficheiro perguntado no menu 
		
		app_layer_transmitter();
	}
	else if (appLayer->flag == RECEIVER)
		app_layer_receiver();
	
	free(appLayer->buf);
	free(appLayer);

	return 1;
}

int app_layer_transmitter(){
 	appLayer->fd=0;
 	if((appLayer->fd=open(appLayer->filename,O_RDONLY,0666)) < 0)
 		return 0;

	printf("ficheiro aberto: %s \n", appLayer->filename);

	struct stat fileStat;

	if(fstat(appLayer->fd,&fileStat) < 0){ 
 		printf("Erro no FSTAT\n");   
 		return 0;
 	}

 	appLayer->filesize = fileStat.st_size;		
 	if (appLayer->filesize < 0){
 		printf("Erro no file size\n");
 		return 0;
 	}

 	appLayer->lengthDados = (Max_Frame_Size - 2 - 8 -4)/2; 
 	appLayer->numDataPack = (int)(((float)appLayer->filesize)/appLayer->lengthDados+.5);

	int n1 = makeCONTROLpackage(appLayer->buf,1);
	

	if(n1==0)
		return 0;

	appLayer->seqNumb = 0;
	int i = 0;
	char * dados = malloc(1000);
	printf("numDataPack = %d \n", appLayer->numDataPack);

	/*
	llopen....
	*/

	int llo = llopen(appLayer->porta, TRANSMITTER);
	
	llwrite(1, appLayer->buf, n1);

	for(i=0; i <= appLayer->numDataPack ; i++){
		int res;

		do{res = read(appLayer->fd, dados, appLayer->lengthDados); }while(res == 0); 

		int datalength = makeDATApackage(appLayer->buf, appLayer->seqNumb, res, dados);

		/*
			Escrever aqui o código que usa o link_layer para enviar os dados
			llwrite
		*/
		//llwrite(0, appLayer->buf, datalength);

		int llw = llwrite(appLayer->fd, appLayer->buf,datalength);

		printf("Percentagem de dados enviados: %f \n", ((float)i*100)/appLayer->numDataPack);
	
		appLayer->seqNumb++;
	}

	int n2 = makeCONTROLpackage(appLayer->buf,2);
	llwrite(appLayer->fd, appLayer->buf,n2);


	int llc = llclose_transmitter(appLayer->fd);
}

int app_layer_receiver(){
	int llo = llopen(appLayer->porta, RECEIVER);
	appLayer->dados = malloc(150);
	appLayer->filename = malloc(150);

	
	llread(RECEIVER, appLayer->buf);
	if(appLayer->buf[0] != 1){
		printf("pacote de controlo inicial com campo de controlo errado: %d\n", appLayer->buf[0]);
		return 0;
	}
	int fileSize;
	

	int j = 1; 
	int ite = 0;
	int octSize;
	for(ite = 0; ite < 2; ite++){
		if (appLayer->buf[j] == 0){
			octSize = appLayer->buf[j+1];
			//printf("octSize do fileSize: %d \n", octSize);
			memcpy(&appLayer->filesize, appLayer->buf+(j+2), octSize);
			//printf("fileSize: %d \n", appLayer->filesize);
		}
		else if (appLayer->buf[j] == 1){
			octSize = appLayer->buf[j+1];
			memcpy(appLayer->filename, appLayer->buf+(j+2), octSize);
			appLayer->filename[octSize] = 0;
			//printf("received filename %s\n", appLayer->filename);
		}
		j+= 2+octSize;
	}

	appLayer->fd=0;
	appLayer->fd = open(appLayer->filename, O_CREAT | O_TRUNC | O_WRONLY, 0666);
	if(appLayer->fd < 0 ){
		printf("Não abriu corretamente para escrita: %s\n", appLayer->filename);
		return 0;
	}

	

	appLayer->lengthDados = (Max_Frame_Size - 2 - 8 -4)/2; 
 	appLayer->numDataPack = (int)(((float)appLayer->filesize)/appLayer->lengthDados+.5);
 	printf("numDataPack do receiver = %d \n", appLayer->numDataPack);

 	int x;
	for(x = 0; x <= appLayer->numDataPack; x++){
		int llr = llread(0, appLayer->buf);
		appLayer->dados = processBuf(appLayer->seqNumb);
		if (appLayer->dados == "rip" || appLayer->dados == 0){
			x--;
			continue;
		}
		printf("Percentagem de dados recebidos: %3f \n", ((float)x*100)/appLayer->numDataPack);
		//printf("escrever no ficherio\n\n\n\n");
		while(!writeToFile(appLayer->dados))
			continue;
		appLayer->seqNumb++;
	}
	//printf("acabaram\n");

	llread(0, appLayer->buf);

	if(appLayer->buf[0] != 2){
		printf("pacote de controlo final com campo de controlo errado: %d\n", appLayer->buf[0]);
		return 0;
	}
	else{
		//printf("ultimo pacote lido\n");
	}


	int llc = llclose_receiver(appLayer->fd);

	printf("Número de pacotes perdidos: %d \n", info->lostPack);

	if (llc){
		//printf("llclose_receiver funcionou \n");
		return 1;
	}
	else
		return 0;

}

// Cria control packages que são enviadas no antes e depois da transferência de dados
int makeCONTROLpackage(char* buf,int c){

	if (c == 1 || c == 2){
	  	buf[0] = c; // pacote enviado no início (start) e no final (end)
	}
	else{
		return 0;
	}

	//printf("fileSize: %d \n", appLayer->filesize);

	//primeiro é enviado o tamanho e depois o nome
	buf[1] = 0;
	buf[2] = sizeof(appLayer->filesize);
	memcpy(buf + 3, &appLayer->filesize, sizeof(appLayer->filesize));
	
	
	buf[3 +sizeof(appLayer->filesize)] = 1;
	buf[4+sizeof(appLayer->filesize)] = strlen(appLayer->filename);
	memcpy(buf + 5 + sizeof(appLayer->filesize), appLayer->filename, strlen(appLayer->filename));

	//int i = 0;
	//printf("trama de controlo %d: ", c);
	/*for (i = 0; i < (4+sizeof(appLayer->filesize)+strlen(appLayer->filename)+1); i++){
		printf("buf[%d] = %x", i, buf[i]);
	}
	printf("\n");*/

	return 4+sizeof(appLayer->filesize)+strlen(appLayer->filename)+1;
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
	return (4+i);	
}

int writeToFile(char* dados){

	int res = write(appLayer->fd, dados,  256 * appLayer->buf[2] + appLayer->buf[3]);
	//printf("Writing to ficherio %d\n", 256 * appLayer->buf[2] + appLayer->buf[3]);
	//write(STDIN_FILENO, dados,  256 * appLayer->buf[2] + appLayer->buf[3]);
	//printf("\ndone writing to ficherio\n");
	if(res == 0)
		return 0;

	else return 1;
} 


char* processBuf(unsigned char seqnumb){

	if(appLayer->buf[0] != 0)
		return 0;

	if(appLayer->buf[1] != (char)seqnumb){
		//printf("rip seqnumb. buf[1] = 0x%x em vez de seqnumb = 0x%x\n", appLayer->buf[1], seqnumb);
		return "rip";
	}

	char * bf = malloc(150);
	int i=0;
	for(i=0; i< 256 * appLayer->buf[2] + appLayer->buf[3]; i++) {
		bf[i] =  appLayer->buf[4 + i];
	}
	return bf;

}

int changeSettings(){
	char fileC[20];
	int tentativasC;
	int timeOutC;
	int baudRateC;
	int max_size; 

	printf("tentativas: ");
	scanf("%d", &tentativasC);

	printf("timeOut: ");
	scanf("%d", &timeOutC);

	printf("BaudRate: ");
	scanf("%d", &baudRateC);
	
	printf("Max_Frame_Size: ");
	scanf("%d", &max_size);

	Settings(tentativasC, timeOutC, baudRateC, max_size);
	return 1;
	
}

int Settings(int trys, int timeO, int BR, int FrameSize){
	tentativas = trys;
	timeOut = timeO;
	BaudRate = convertBaudrate(BR);
	Max_Frame_Size = 2*FrameSize+2+4+8;

	return 1;
}

int convertBaudrate(int baudrate){
	switch(baudrate){
	case 300:
		return B300;
	case 1200:
		return B1200;
	case 2400:
		return B2400;
	case 4800:
		return B4800;
	case 9600:
		return B9600;
	case 19200:
		return B19200;
	case 38400:
		return B38400;
	case 57600:
		return B57600;
	case 115200:
		return B115200;
	case 230400:
		return B230400;
	default:
	 	return -1;
	 }
}




