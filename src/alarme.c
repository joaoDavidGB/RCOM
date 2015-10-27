#include "alarme.h"

void atende(int sig)                   // atende alarme
{
	printf("alarme # %d\n", conta);
	flag=1;
	conta++;
}

int install_handler(void(*handler)(int), int timeout){
 	struct sigaction sa;
 	sigaction(SIGALRM, NULL, &sa);


    sa.sa_handler = handler;

    if (sigaction(SIGALRM, &sa, NULL) == -1)
    	return -1;
    alarm(timeout);
}


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

