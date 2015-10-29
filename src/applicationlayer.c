/*
* Aplication Layer
*/
#include "applicationlayer.h"

#define MAX_FRAME_SIZE 100

int porta;

int main(int argc, char** argv){

	if(argc != 5){
		printf("numero de argumentos errado. \n");
		pirntf("%s (porta(/dev/ttySN)) ficheiro flag(1-transmitter, 0-receiver) \n", argv[0]);
	}
	applicationlayer appLayer = malloc(sizeof(struct applicationlayer));
	appLayer->flag = argv[3];
	appLayer->filename = malloc(100);

	appLayer->filename = argv[2]; //Nome do ficheiro passado como argumento

	if (appLayer->flag == TRANSMITTER)
		app_layer_transmitter();
	else if (appLayer->flag == RECEIVER)
		app_layer_receiver();
	
 	porta = argv[3]; //estou a passar a porta como argumento.... 

	
	/*
	for(i=0; i<=numDataPack ; i++){
		int res;
		fprintf(stderr, "lengthDados = %d \n", appLayer->lengthDados);

		do{res = read(appLayer->fd, dados, appLayer->lengthDados); }while(res == 0); //CONFIRMAR SE ESTA A LER PARA OS DADOS!!!!!
			

		fprintf(stderr, "Dados = %s | res = %d \n", dados, res);

		int suc = makeDATApackage(appLayer->buf, seqNumb, res, dados);

		writeToFile(dados, appLayer->buf);


		//if(suc != 0)
			//return 0;
	}
	*/
/*
	int n2 = makeCONTROLpackage(buf,2);

	if(n2!=0)
		return 0;
*/
	return 1;
}

int app_layer_transmitter(){
	printf("ficheiro aberto: %s \n", appLayer->filename);
 	appLayer->fd=0;
 	if((appLayer->fd=open(appLayer->filename,O_RDONLY,0666)) < 0)
 		return 0;

	printf("LEU O FICHEIRO!\n");

	struct stat fileStat;

	if(fstat(appLayer->fd,&fileStat) < 0){ 
 		printf("DEU ASNEIRA FSTAT\n");   
 		return 0;
 	}

 	appLayer->filesize = fileStat.st_size;		
 	if (appLayer->filesize < 0){
 		printf("DEU ASNEIRA file size\n");
 		return 0;
 	}

 	printf("file size %d\n", appLayer->filesize);

 	appLayer->lengthDados = (MAX_FRAME_SIZE - 2 - 8 -4)/2; 
 	appLayer->numDataPack = (int)(((float)appLayer->filesize)/appLayer->lengthDados+.5);

	appLayer->buf = malloc(100); //escrevemos sempre no mesmo buffer ele é sempre reescrito
	int n1 = makeCONTROLpackage(appLayer->buf,1);

	if(n1==0)
		return 0;

	appLayer->seqNumb = 0;
	int i = 0;
	char * dados = malloc(100);
	printf("numDataPack = %d \n", appLayer->numDataPack);

	/*
	llopen....
	*/

	 int llo = llopen(porta, 1);
	

	for(i=0; i<=numDataPack ; i++){
		int res;
		fprintf(stderr, "lengthDados = %d \n", appLayer->lengthDados);

		do{res = read(appLayer->fd, dados, appLayer->lengthDados); }while(res == 0); //CONFIRMAR SE ESTA A LER PARA OS DADOS!!!!!
			

		fprintf(stderr, "Dados = %s | res = %d \n", dados, res);

		int suc = makeDATApackage(appLayer->buf, seqNumb, res, dados);

		/*
			Escrever aqui o código que usa o link_layer para enviar os dados
			llwrite
		*/

		int llw = llwrite(appLayer->fd, dados, appLayer->lengthDados);
	
		seqNumb = !seqNumb;

		//if(suc != 0)
			//return 0;
	}


	int llc = llclose_transmitter(appLayer->fd);
	

}

int app_layer_receiver(){
	appLayer->fd=0;
	appLayer->fd = open(argv[2], O_CREAT | O_TRUNC | O_WRONLY, 0666);
	if(fd < 0 ){
		printf("Não abriu corretamente para escrita\n");
		return 0;
	}
}

// Cria control packages que são enviadas no antes e depois da transferência de dados
int makeCONTROLpackage(char* buf,int c){

	if (c == 1 || c == 2)
	  	buf[0] = c; // pacote enviado no início (start) e no final (end)
	else
		return 0;

	//primeiro é enviado o tamanho e depois o nome
	buf[1] = 0;
	buf[2] = sizeof(appLayer->filesize);
	memcpy(buf+3, &appLayer->filesize, sizeof(appLayer->filesize));
	
	buf[3 + sizeof(appLayer->filesize)] = 1;
	buf[4 + sizeof(appLayer->filesize)] = sizeof(appLayer->filename);
	memcpy(buf + 4 +sizeof(appLayer->filesize), &appLayer->filesize, strlen(appLayer->filename));

	return 1;
}

// Cria data package que envia o ficheiro
int makeDATApackage(char* buf,int seqNumb, int appLayer->lengthDados, char* dados){
	buf[0] = 0;
	buf[1] = seqNumb;
	buf[2] = appLayer->lengthDados/256;  
	buf[3] = appLayer->lengthDados%256;
	int i;
	for(i=0; i < appLayer->lengthDados; i++){
		buf[4+i] = dados[i];
	}	
}

int writeToFile(char* dados, char* buf){
	int res = write(fd,dados,  256 * buf[2] + buf[3]);

	if(res == 0)
		return 0;

	else return 1;
} 


