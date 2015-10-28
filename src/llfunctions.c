#include "llfunctions.h"

/*
  Função main para testar LL's
*/
int main(int argc, char** argv){
  info = malloc(sizeof(struct Info));
  info->sequenceNumber = atoi(argv[3]);
  info->dados = malloc(255);
  info->timeout = 3;
  install_handler(atende, info->timeout);
  printf("sequenceNumber: %d \n", info->sequenceNumber);


  if (strcmp("0", argv[1])==0){       //RECEIVER
    llopen(atoi(argv[2]), RECEIVER); 
    char * result;
    llread(info->fd, result);
    printf("INICIAR LLCLOSE\n");
    llclose_receiver(info->fd);
  }
  else if (strcmp("1", argv[1])==0){      //Transmitter
    llopen(atoi(argv[2]), TRANSMITTER);
    info->dados[0] = 0x11;
    info->dados[1] = 0x22;
    info->dados[2] = 0x05;
    sleep(1);
    printf("llwrite de %x, %x, %x \n", info->dados[0], info->dados[1], info->dados[2]);
    info->lengthDados = 3;
    llwrite(info->fd, info->dados, info->lengthDados);
    printf("INICIAR LLCLOSE\n");
    llclose_transmitter(info->fd);
  }

 filename = argv[2];
 int file=0;
  if((file=open(filename,O_RDONLY)) < -1)
       return 1;

 struct stat fileStat;
  if(fstat(file,&fileStat) < 0)    
        return 1;

 filesize = fileStat.st_size;	

 if(read(file, buf, filesize) < 0)
	return 1;
}

int llopen(int porta, int flag){

  sprintf(info->endPorta, "/dev/ttyS%d", porta);
  info->fd = open(info->endPorta, O_RDWR | O_NOCTTY);
  if (info->fd < 0) {perror(info->endPorta); exit(-1);}

  if ( tcgetattr(info->fd,&info->oldtio) == -1) { // save current port settings 
    perror("tcgetattr");
    return -1;
  }

  bzero(&info->newtio, sizeof(info->newtio));

  info->newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  info->newtio.c_iflag = IGNPAR;
  info->newtio.c_oflag = OPOST;

  // set input mode (non-canonical, no echo,...) 
  info->newtio.c_lflag = 0;

  info->newtio.c_cc[VTIME]    = 0;   // inter-character timer unused 
  info->newtio.c_cc[VMIN]     = 1;   // blocking read until 5 chars received 

  tcflush(info->fd, TCIFLUSH);

  if ( tcsetattr(info->fd,TCSANOW,&info->newtio) == -1) {
    perror("tcsetattr");
    return -1;
  }

  info->tentativas = 3;

  if (flag == RECEIVER){
    if(receberSET(flag, "set")==1)
      transmitirSET(flag, "ua");
    else{
      fprintf(stderr, "Não recebeu a trama set corretamente no llopen() \n");
      return -1;
    }

  }
  else{
    while(info->tentativas > 0){
      transmitirSET(flag, "set");
      start_alarm();
      fprintf(stderr, "alarm\n");
      sleep(5);
      if (receberSET(flag, "ua") != 1)
        info->tentativas--;
      else{
        stop_alarm();
        break;
      }
    }
  }
  
  return info->fd;
}

int llclose_transmitter(int fd){

    transmitirSET(1, "disc");
    if (receberSET(1, "disc") == 1)
      transmitirSET(1, "ua");
    else
      return -1;


    if ( tcsetattr(info->fd,TCSANOW,&info->oldtio) == -1) {
      perror("tcsetattr");
      return -1;
    }
    close(fd);
    printf("fechou transmissor\n");
    return 1;
}

int llclose_receiver(int fd){

    if (receberSET(1, "disc") == 1){
      transmitirSET(1, "disc");
      if (receberSET(1, "ua") != 1){
        return -1;
      }
    }
    else
      return -1;


    if ( tcsetattr(info->fd,TCSANOW,&info->oldtio) == -1) {
      perror("tcsetattr");
      return -1;
    }
    close(fd);
    printf("fechou recetor\n");
    return 1;
}


int transmitirSET(int flag, char * type){
  SET[0] = F;
  SET[1] = A;
  if (type == "set") 
    SET[2] = C_SET;
  else if (type == "ua")
    SET[2] = C_UA;
  else if (type == "disc")
    SET[2] = C_DISC;
  else if (!strcmp(type, "rr1"))
    SET[2] = RR(1);
  else if (!strcmp(type,"rr0"))
    SET[2] = RR(0);
  SET[3] = SET[1]^SET[2];
  SET[4] = F;


  int i = 0;
  while(i < 5){
    res = write(info->fd,&SET[i],1);  
    i++;
  }
  i=0;
  printf("Send %s: 0x%x 0x%x 0x%x 0x%x 0x%x \n", type, SET[0], SET[1], SET[2], SET[3], SET[4]);
  
  return 0;
}

int receberSET(int flag, char * type){
  char buf2 = 0;
  int res2;
  estado = START;
  int i = 0;
  while(i < 5){
    if (i == 0){
      while((res2 = read(info->fd, &buf2, 1))==0 && buf2!=F)
	     continue;
    }
    else
	    while((res2 = read(info->fd, &buf2, 1))==0)
       continue;

    printf("Received: %x !!! %d \n", buf2, res2);
    i++;
    //printf("i = %d\n", i);
    
    state_machine(estado, buf2, type);
    if(estado == STOP2){
      estado = START;
      return 1; 
    }
  
  }
  estado = START;
  return 0;
}

char * receberI(int flag){
  //char * dados;
  //dados = malloc(sizeof(255));
  char buf2 = 0;
  int res2;
  int i;
  estado = START;
  for (i = 0; i < 4; i++){
    if (i == 0){
      while((res2 = read(info->fd, &buf2, 1))==0 && buf2!=F)
       continue;
    }
    else{
      while((res2 = read(info->fd, &buf2, 1))==0)
       continue;
   }

     printf("ReceivedI[%d]: %x !!! %d \n", i, buf2, res2);
     state_machine(estado, buf2, "I");
  }
  if (estado != BCC_STATE)
    return "fail";

  printf("nao falhou na receçao dos primeiros do I\n");
  char BBC2 = 0;
  i = 0;
  buf2 = 1;
  while(BBC2 != buf2){
    while((res2 = read(info->fd, &buf2, 1))==0)
      continue;
    printf("ReceivedDados[%d]: %x !!! %d \n", i, buf2, res2);
    printf("BBC2=%x -- buf2=%x \n", BBC2, buf2);
    if (BBC2 == buf2)
      break;
    BBC2 = BBC2^buf2;
    info->dados[i] = buf2;
    if (i == 0)
      buf2 = 1;
    i++;
  }

  printf("acabaram os dados\n");
  while((res2 = read(info->fd, &buf2, 1))==0)
    continue;
  //state_machine(estado, buf2, "I");
  if (estado == STOP2){
    printf("recebeu a trama I corretamente\n");
    return info->dados;
  }
  else
    return "fail";

}

void state_machine(int state, char signal, char * type){
       
 
        if (state == START){
                if (signal == F){
                        state = FLAG;
                        SET2[0]=signal;
                }
        }
        else if (state == FLAG){
                if (signal == F)
                        state = FLAG;
                else if ((signal == A && type != "I")
                  || (signal == campo_endereco(flag, info->sequenceNumber) && type == "I")){
                        state = A_STATE;
                        SET2[1]=signal;
                }
                else
                        state = START;
        }
        else if (state == A_STATE){
                if (signal == F){
                        state = FLAG;
                }
                else if ((signal == C_SET && type == "set")
                  || (signal == C_UA && type == "ua")
                  || (signal == C_DISC && type == "disc")
                  || (signal == RR(1) && type == "rr1")
                  || (signal == RR(0) && type == "rr0")
                  || (signal == info->sequenceNumber && type == "I")){
                        state = C;
                        SET2[2]=signal;
                }
                else
                        state = START;
        }
        else if (state == C){
                if (signal == F)
                        state = FLAG;
                else if (signal == (SET2[1]^SET2[2])){
                        state = BCC_STATE;
                        SET2[3]=signal;
                }
                else
                        state = START;
        }
        else if (state == BCC_STATE){
                if (signal == F){
                        state = STOP2;
                        SET2[4]=signal;
                }
                else
                        state = START;
        }
        estado = state;
       
        printf("estado: %d \n", estado);
}

int llwrite(int fd, char * buffer, int length){
  char * tramaI;
  //strcpy(tramaI, comporTramaI(TRANSMITTER, buffer, length));
  tramaI = comporTramaI(TRANSMITTER, buffer, info->lengthDados);
  printf("partes: %x, %x, %x, %x, %x, %x, %x, %x, %x \n", tramaI[0],tramaI[1],tramaI[2],tramaI[3],tramaI[4],tramaI[5],tramaI[6],tramaI[7],tramaI[8]);
  transmitirFrame(tramaI, 6+length);
  alarm(3);
  free(tramaI);
  if (info->sequenceNumber == 1){
    if (receberSET(TRANSMITTER, "rr0")){
      printf("recebeu rr corretamente \n");
      alarm(0);
    }
  }
  else if (info->sequenceNumber == 0){
    if (receberSET(TRANSMITTER, "rr1")){
      printf("recebeu rr corretamente \n");
      alarm(0);
    }
  }
  printf("retornar llwrite\n");
  return 1;
}

int llread(int fd, char * buffer){
  //char * dados;
  //dados = receberI(RECEIVER);
  receberI(RECEIVER);
  printf("Dados recebidos: %x, %x, %x \n", info->dados[0],info->dados[1],info->dados[2]);
  if (info->dados == "fail"){
    //transmitirSET(RECEIVER, "rej");
    fprintf(stderr, "falhou a receber a I: %s \n", info->dados);
    return 0;
  }
  char * rrtype = malloc(5);
  printf("cenas dos rr\n");
  sprintf(rrtype, "rr%d", !info->sequenceNumber);
  fprintf(stderr, "enviar %s\n", rrtype);
  transmitirSET(RECEIVER, rrtype);
  free(rrtype);
  return 1;
}

char * comporTramaI(int flag, char * buffer, int length){
  char * trama;
  trama = malloc(sizeof(5 + length));
  int index;
  trama[0] = F;
  trama[1] = campo_endereco(flag, info->sequenceNumber);
  trama[2] = info->sequenceNumber;
  trama[3] = trama[1]^trama[2];
  trama[4 + length] = 0;
  for(index = 0; index < length; index++){
    /*
      ADICIONAR STUFFING E DESTUFFING
    */
    trama[4 + index] = buffer[index];
    trama[4 + length] = trama[4 + length]^trama[4 + index];
  }
  trama[4 + length + 1] = F;

  int i;
  for(i = 0; i <= (5+length); i++){
    printf("I[%d]=%x ", i, trama[i]);
  }
  printf("\n");
  return trama;
}

/*
//DESTUFFING feito pela Filipa
void destuffing(unsigned char* frame, unsigned int* size){
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

//STUFFING feito pela Filipa
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
 */


int transmitirFrame(char * frame, int length){
  int i;
  fprintf(stderr, "Enviar frame tamanho %d : ", length);
  for(i = 0; i < length; i++){
    res = write(info->fd,&frame[i],1);
    fprintf(stderr,"0x%x ", frame[i]);
  }
  fprintf(stderr,"/n");
}

int campo_endereco(int role, int c){
  if (role == TRANSMITTER){
    if (Is_cmd(c))
      return 0x03;
    else
      return 0x01;
  }
  else if (role == RECEIVER){
    if (Is_cmd(c))
      return 0x01;
    else
      return 0x03;
  }

  printf("fail no campo_endereco \n");
}

int Is_cmd(int comand){
  if (comand == C_I0 || comand == C_I1 || comand == C_SET || comand == C_DISC)
    return 1;
  else
    return 0;

}



void comporPacotesControlo(int c){

 buf[0] = c;
  //primeiro vou por o tamanho e depois o nome
 buf[1] = 0;
 buf[2] = sizeof(filesize);
 memcpy(buf+3, &filesize, sizeof(filesize));

 buf[3 +sizeof(filesize)] = 1;
 buf[5] = sizeof(filename);
 memcpy(buf + 4 +sizeof(filesize), &filesize, strlen(filename));
	
}

void comporPacotesDados(int seqNumb, int sizeCampoI, int lengthDados, char* dados){

 buf[0] = 0;
 buf[1] = seqNumb;
 buf[2] = lengthDados/256;  
 buf[3] = lengthDados%256;
  int i;
  for(i=0; i < lengthDados; i++){

 buf[4] = dados[i];

  }	
 
}
