#include "link_layer.h"

int estado = START;

/*
  Lê uma frame para o parametro "frame" e retorna o tamanho da frame
*/
int readFrame(char * frame){
  char buf2 = 0;
  int res2;
  int i = 0;
  int j = 0;
  int primeiroF = 1; //passa a 0 assim que a primeira FLAG é encontrada
  //printf("Received: ");
  while(1){
    while((res2 = read(info->fd, &buf2, 1))==0)
      continue;

    if (res2 == -1)
      return 0;

    //start_alarm();

    //printf("0x%02x ", buf2);
    

    if (buf2 == F){
      if (!primeiroF){  //se não for a primeira FLAG, será a ultima e termina
        if (i < 4){ //se a trama não tiver pelo menos 5 chars é invalida e começa a ler outra
          i = 1;
          frame[0] = buf2;
          continue;
        }
        else{ //se a trama tiver tamanho 5 ou superior termina
          frame[i] = buf2;
          i++; 
          break;
        }
      }
      else{  //se for a primeira FLAG, começa a ler.
        primeiroF = 0;
        frame[i] = buf2;
        i++;
        continue;
      }
    }
    else if (!primeiroF){ //se não for FLAG e ja tiver sido encontrada primeira FLAG, adiciona à frame
      frame[i] = buf2;
    }
    else
      continue;


    i++;
  }

  //printf("\n");
  return i;
}

/*
  retorna o tipo de frame verificando o campo de controlo
*/
char * verifyFrameType(char * frame){
  switch(frame[2]){
    case C_SET:
      return "set";
      break;
    case C_DISC:
      return "disc";
      break;
    case C_UA:
      return "ua";
      break;
    case RR(1):
      return "rr1";
      break;
    case RR(0):
      return "rr0";
      break;
    case REJ(0):
      return "rej0";
      break;
    case REJ(1):
      return "rej1";
      break;
    case C_I0:
      return "I0";
      break;
    case C_I1:
      return "I1";
      break;
  }
}

/*
  Verifica se a frame está correta segundo o seu tipo ("type")
*/
int verifyFrame(char * frame, int length, char * type){
  int i=0;
  int j=0;
  estado = START;
  char BBC2 = 0;
  for(i = 0; i < length; i++){

   //printf("verifyFrame %s[%d]: %x \n", type, i, frame[i]);

    if (type != "I0" && type != "I1"){
      if (length != 5){
        info->lostPack++;
        printf("frame do tipo %s com tamanho irregular = %d \n", type, length);
        return 0;
      }
      state_machine(estado, frame[i], type);
      
    }
    else{
      if (i < 4 || (i == (length-1))){
        state_machine(estado, frame[i], type);
      }
      else if (i == (length - 2)){
        if (frame[i] != BBC2){
	  info->lostPack++;
          //printf("\n\n\n\nBCC2 devia ser 0x%02x, mas é 0x%02x\n\n\n\n\n", BBC2, frame[i]);
          return 0;
        }
      }
      else{
        BBC2 = BBC2^frame[i];
        info->dados[j] = frame[i];
        j++;
        info->lengthDados = j;
      }
    }

  }

  if (estado == STOP2){
    estado = START;
    return 1;
  }
  else{
    info->lostPack++;
    estado = START;
    return 0;
  }
}

/*
  Cria e envia uma trama do tipo type (Não funcional para tipo I)
*/
int buildFrame(int flag, char * type){
  info->frameSend[0] = F;
  if (type == "set") 
    info->frameSend[2] = C_SET;
  else if (type == "ua")
    info->frameSend[2] = C_UA;
  else if (type == "disc")
    info->frameSend[2] = C_DISC;
  else if (!strcmp(type, "rr1"))
    info->frameSend[2] = RR(1);
  else if (!strcmp(type,"rr0"))
    info->frameSend[2] = RR(0);
  else if (!strcmp(type,"rej0"))
    info->frameSend[2] = REJ(0);
  else if (!strcmp(type,"rej1"))
    info->frameSend[2] = REJ(0);
  else
    return 0;
  info->frameSend[1] = campo_endereco(info->flag, info->frameSend[2]);
  info->frameSend[3] = info->frameSend[1]^info->frameSend[2];
  info->frameSend[4] = F;
  info->frameSendLength = 5;

  //printf("Trama composta %s: 0x%x 0x%x 0x%x 0x%x 0x%x \n", type, info->frameSend[0], info->frameSend[1], info->frameSend[2], info->frameSend[3], info->frameSend[4]);
  
  return 1;
}

char * comporTramaI(int flag, char * buffer, int length){
  /*
    int ecx = 0;
    
    printf("tamanho dos dados a enviar: %d \n Dados: ", length);
    for(ecx = 0; ecx < length; ecx++){
      printf("%x.", buffer[ecx]);
    }
    printf("\n");
  */

  int index;
  info->frameSend[0] = F;
  if (info->sequenceNumber == 1){
    info->frameSend[2] = C_I1;
  }
  else
    info->frameSend[2] = C_I0;
  info->frameSend[1] = campo_endereco(flag, info->frameSend[2]);
  info->frameSend[3] = info->frameSend[1]^info->frameSend[2];
  info->frameSend[4 + length] = 0;
  for(index = 0; index < length; index++){
    info->frameSend[4 + index] = buffer[index];
    info->frameSend[4 + length] = info->frameSend[4 + length]^info->frameSend[4 + index];
  }
  info->frameSend[4 + length + 1] = F;
  /*
    int i;
    for(i = 0; i <= (5+length); i++){
      printf("I[%d]=%x ", i, info->frameSend[i]);
    }
    printf("\n");
  */
  info->frameSendLength = 6+length;
  //fprintf(stderr,"Construida ");
 /* int i;
  for(i = 0; i < info->frameSendLength; i++){
    fprintf(stderr,"0x%x ", info->frameSend[i]);
  }
  fprintf(stderr,"\n");*/
  return info->frameSend;
}

int llopen(char * porta, int flag){

  printf("FLAG (TRANS/REC) = %d \n", flag);
  info = malloc(sizeof(struct Info));
  info->dados = malloc(255);
  info->frameTemp = malloc(255);
  info->frameSend = malloc(255);
  info->timeout = timeOut;
  install_handler(atende, info->timeout);
  //printf("sequenceNumber: %d \n", info->sequenceNumber);
  info->tentativas = tentativas;
  info->flag = flag;

  info->endPorta = malloc(255);
  info->endPorta = porta;
  info->fd = open(info->endPorta, O_RDWR | O_NOCTTY);
  if (info->fd < 0) {perror(info->endPorta); exit(-1);}

  if ( tcgetattr(info->fd,&info->oldtio) == -1) { // save current port settings 
    perror("tcgetattr");
    return -1;
  }

  bzero(&info->newtio, sizeof(info->newtio));

  info->newtio.c_cflag = BaudRate | CS8 | CLOCAL | CREAD;
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

  if (flag == TRANSMITTER){
    printf("llopen de transmissor \n");
    buildFrame(flag, "set");
    transmitirFrame(info->frameSend, info->frameSendLength);
    while(info->tentativas > 0){
      //printf("tentativasOpen = %d \n", info->tentativas);
      start_alarm();
      info->frameTempLength = readFrame(info->frameTemp);
      if (verifyFrame(info->frameTemp, info->frameTempLength, "ua")){
        stop_alarm();
        info->tentativas = tentativas;
        return 1;
      }
    }
    if (info->tentativas == 0){
	printf("Número de tentativas chegou ao fim. \n");
	exit(-1);
    }
  }
  else{
    printf("llopen de recetor \n");
    info->frameTempLength = readFrame(info->frameTemp);
    if (verifyFrame(info->frameTemp, info->frameTempLength, "set")){
      buildFrame(flag, "ua");
      transmitirFrame(info->frameSend, info->frameSendLength);
      //printf("terminar llopen recetor \n");
      return 1;
    }
  }
  
  return info->fd;
}

int llwrite(int fd, char * buffer, int length){
  
  comporTramaI(TRANSMITTER, buffer, length);
  stuffing(info->frameSend, &info->frameSendLength);
  //printf("partes: %x, %x, %x, %x, %x, %x, %x, %x, %x \n", tramaI[0],tramaI[1],tramaI[2],tramaI[3],tramaI[4],tramaI[5],tramaI[6],tramaI[7],tramaI[8]);
  transmitirFrame(info->frameSend, info->frameSendLength);
  //printf("enviar frame I com sequenceNumber = %d \n", info->sequenceNumber);
  info->tentativas = tentativas;
  while(info->tentativas > 0){
    start_alarm();
    info->frameTempLength = readFrame(info->frameTemp);
    if (info->sequenceNumber == 1){
      if (verifyFrame(info->frameTemp, info->frameTempLength, "rr0")){
        //printf("recebeu rr corretamente \n");
        stop_alarm();
        info->tentativas = tentativas;
        break;
      }
      else if (verifyFrame(info->frameTemp, info->frameTempLength, "rej0")){
        //printf("recebeu rej0\n");
        transmitirFrame(info->frameSend, info->frameSendLength);
        continue;
      }
    }
    else if (info->sequenceNumber == 0){
      if (verifyFrame(info->frameTemp, info->frameTempLength, "rr1")){
        //printf("recebeu rr corretamente \n");
        stop_alarm();
        info->tentativas = tentativas;
        break;
      }
      else if (verifyFrame(info->frameTemp, info->frameTempLength, "rej1")){
        //printf("recebeu rej1\n");
        transmitirFrame(info->frameSend, info->frameSendLength);
        continue;
      }
    }
  }
  if (info->tentativas == 0){
	printf("Número de tentativas chegou ao fim. \n");
	exit(-1);
  }
  info->sequenceNumber = !info->sequenceNumber;
  //printf("retornar llwrite\n");
  return 1;
}

int llread(int fd, char * buffer){

  //printf("iniciar llread \n");
  while(1){
    info->frameTempLength = readFrame(info->frameTemp);
    char * type = NULL;
    type = verifyFrameType(info->frameTemp);
    if (type == "set"){
      buildFrame(info->flag, "ua");
      transmitirFrame(info->frameSend, info->frameSendLength);
      continue;
    }
    else if (type == "I0" || type == "I1"){
      destuffing(info->frameTemp, &info->frameTempLength);
        //fprintf(stderr,"Destuffing ");
        /*int bb;
        for(bb = 0; bb < info->frameTempLength; bb++){
          fprintf(stderr,"0x%x ", info->frameTemp[bb]);
        }
        fprintf(stderr,"\n");*/
      if (verifyFrame(info->frameTemp, info->frameTempLength, type)){

        if(type == "I0" && !info->sequenceNumber
          || type == "I1" && info->sequenceNumber){
          //printf("recebeu a trama I correspondente aos sequenceNumber %d \n", info->sequenceNumber);
          char * typeRR = malloc(5);
          sprintf(typeRR, "rr%d", !info->sequenceNumber);
          //printf("criar frame de %s \n", typeRR);
          buildFrame(info->flag, typeRR);
          transmitirFrame(info->frameSend, info->frameSendLength);
          free(typeRR);
          info->sequenceNumber = !info->sequenceNumber;
        }
        else{
          char * typeRR = malloc(5);
          sprintf(typeRR, "rr%d", info->sequenceNumber);
          //printf("criar frame de %s \n", typeRR);
          buildFrame(info->flag, typeRR);
          transmitirFrame(info->frameSend, info->frameSendLength);
          free(typeRR);
          continue;
        }

        int j;
        //printf("frameTempLength: %d\n", info->frameTempLength);
        //printf("dados recebidos: ");
        

        for(j = 0; j < (info->frameTempLength-6); j++){
          info->dados[j] = info->frameTemp[4+j];
          //printf(" %x ", info->dados[j]);
          buffer[j] = info->dados[j];
          //printf(" %x \n", info->dados[j]);
        }
        //printf("\n");
        info->lengthDados = j;
      }
      else{
        char * typeREJ = malloc(5);
        sprintf(typeREJ, "rej%d", !info->sequenceNumber);
        //printf("criar frame de %s \n", typeREJ);
        buildFrame(info->flag, typeREJ);
        transmitirFrame(info->frameSend, info->frameSendLength);
        free(typeREJ);
	continue;
      }
    }
    break;
  }

  return 1;
}

int llclose_transmitter(int fd){
  info->tentativas = tentativas;

  while(info->tentativas > 0){
    buildFrame(info->flag, "disc");
    transmitirFrame(info->frameSend, info->frameSendLength);
    start_alarm();

    info->frameTempLength = readFrame(info->frameTemp);
    char * type = malloc(5);
    type = verifyFrameType(info->frameTemp);
    if (verifyFrame(info->frameTemp, info->frameTempLength, "disc")){
      buildFrame(info->flag, "ua");
      if(transmitirFrame(info->frameSend, info->frameSendLength))
        break;
    }  
  }

  if (info->tentativas == 0){
	printf("Número de tentativas chegou ao fim. \n");
	exit(-1);
  }

   sleep(1);
    if ( tcsetattr(info->fd,TCSANOW,&info->oldtio) == -1) {
      perror("tcsetattr");
      return -1;
    }
    close(fd);
    printf("fechou transmissor\n");
    return 1;
}

int llclose_receiver(int fd){
  info->tentativas = tentativas;
  while(1){
    info->frameTempLength = readFrame(info->frameTemp);
    char * type = malloc(5);
    type = verifyFrameType(info->frameTemp);

    if (type == "I0" || type == "I1"){
      if (verifyFrame(info->frameTemp, info->frameTempLength, type)){
        char * typeRR = malloc(5);
        sprintf(typeRR, "rr%d", !info->sequenceNumber);
        //printf("criar frame de %s \n", typeRR);
        buildFrame(info->flag, typeRR);
        transmitirFrame(info->frameSend, info->frameSendLength);
        free(typeRR);
        int j;
        //printf("dados recebidos: ");
        for(j = 0; j < (info->frameTempLength-6); j++){
          info->dados[j] = info->frameTemp[4+j];
          //printf(" %x ", info->dados[j]);
        }
        //printf("\n");
        info->lengthDados = j;
        continue;
      }
      else{
        char * typeREJ = malloc(5);
        sprintf(typeREJ, "rej%d", !info->sequenceNumber);
        //printf("criar frame de %s \n", typeREJ);
        buildFrame(info->flag, typeREJ);
        transmitirFrame(info->frameSend, info->frameSendLength);
        free(typeREJ);
        continue;
      }
    }
    else if (verifyFrame(info->frameTemp, info->frameTempLength, "disc")){
      buildFrame(info->flag, "disc");
      transmitirFrame(info->frameSend, info->frameSendLength);
      start_alarm();

      info->frameTempLength = readFrame(info->frameTemp);
      type = verifyFrameType(info->frameTemp);

      if (verifyFrame(info->frameTemp, info->frameTempLength, "ua")){
        break;
      }
    }
    else{
      printf("llclose_receiver não recebeu nem I nem disc \n");
    }
  }


  if ( tcsetattr(info->fd,TCSANOW,&info->oldtio) == -1) {
    perror("tcsetattr");
    return 0;
  }
  close(fd);
  printf("fechou recetor\n");
  return 1;
}

int transmitirFrame(char * frame, int length){
  int i;
  //fprintf(stderr, "Enviar frame tamanho %d : ", length);
  for(i = 0; i < length; i++){
    res = write(info->fd,&frame[i],1);
    if (res == 0 || res == -1)
      return 0;
    //fprintf(stderr,"0x%x ", frame[i]);
  }
  //fprintf(stderr,"\n");
  return 1;
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
                else if ((signal == campo_endereco(!info->flag, C_SET) && type == "set")
                  || (signal == campo_endereco(!info->flag, C_UA) && type == "ua")
                  || (signal == campo_endereco(!info->flag, C_DISC) && type == "disc")
                  || (signal == campo_endereco(!info->flag, RR(1)) && type == "rr1")
                  || (signal == campo_endereco(!info->flag, RR(0)) && type == "rr0")
                  || (signal == campo_endereco(!info->flag, REJ(1)) && type == "rej1")
                  || (signal == campo_endereco(!info->flag, REJ(0)) && type == "rej0")
                  || (signal == campo_endereco(!info->flag, C_I0) && type == "I0")
                  || (signal == campo_endereco(!info->flag, C_I1) && type == "I1")){
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
                  || (signal == REJ(1) && type == "rej1")
                  || (signal == REJ(0) && type == "rej0")
                  || (signal == C_I0 && type == "I0")
                  || (signal == C_I1 && type == "I1")){
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
       
        //printf("estado: %d \n", estado);
}

int campo_endereco(int role, int c){
  if (role == TRANSMITTER){
    if (Is_cmd(c)){
      return 0x03;
    }
    else{
      return 0x01;
    }
  }
  else if (role == RECEIVER){
    if (Is_cmd(c)){
      return 0x01;
    }
    else{
      return 0x03;
    }
  }

  //printf("fail no campo_endereco \n");
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

void atende(int sig) {
  printf("alarme # %d\n", conta);
  flag=1;
  conta++;
  printf("tentativas = %d \n", info->tentativas);
  if (info->tentativas > 0){
    transmitirFrame(info->frameSend, info->frameSendLength);
    info->tentativas--;
  }
  else{
    fprintf(stderr, "0 tentativas restantes \n");
    stop_alarm();
    exit(-1);
  }
}

// Stuffing
void stuffing(unsigned char* frame, unsigned int* size){
  int i;
  for (i = 1; i < (*size-1); i++){
      if (frame[i] == 0x7e){

        memmove(frame + i + 1,frame + i ,*size-i); 
        frame[i] = 0x7d;
        frame[++i] = 0x5e;
        (*size)++;
      } 
      else if (frame[i] == 0x7d){
        
        memmove(frame + i + 1,frame + i ,*size-i); 
        frame[i] = 0x7d;
        frame[++i] = 0x5d;
        (*size)++;
      }
  }
}

//DESTUFFING
void destuffing(unsigned char* frame, unsigned int* size){
  int i;
  for (i = 1; i < (*size-1); i++){
    if(frame[i] == 0x7d && frame[i+1] == 0x5e){
      
       memmove(frame + i + 1, frame + i + 2, *size-i-2);
       frame[i] = 0x7e;
      (*size)--;
    }
    else if (frame[i] == 0x7d && frame[i+1]== 0x5d){
      
      memmove(frame + i + 1, frame + i + 2, *size-i-2);
      //frame[i] = 0x7d;
     (*size)--;
    }

  }
}
