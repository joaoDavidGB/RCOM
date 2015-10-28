#include "alarme.h"

int timeout = 0;

int flag=1, conta=1;

void atende(int sig)                   // atende alarme
{
	printf("alarme # %d\n", conta);
	flag=1;
	conta++;
}

int install_handler(void(*handler)(int), int timeOut){
 	struct sigaction sa;
 	sigaction(SIGALRM, NULL, &sa);

 	timeout = timeOut;

    sa.sa_handler = handler;

    if (sigaction(SIGALRM, &sa, NULL) == -1)
    	return -1;
}

void start_alarm(){
	alarm(timeout);
}

void stop_alarm(){
	alarm(0);
}



/*
int main()
{

	(void) signal(SIGALRM, atende);  // instala  rotina que atende interrupcao

	while(conta < 4){
	   	if(flag){
	      alarm(3);                 // activa alarme de 3s
	      flag=0;
	   }
	}
	printf("Tentativas maximas atingidas.\n");
	return 0;
}
*/
