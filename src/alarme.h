#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <signal.h>

int flag=1, conta=1;
void atende(int sig);
int install_handler(void(*handler)(int), int timeout);