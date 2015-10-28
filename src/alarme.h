#pragma once

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <signal.h>

extern int flag;
extern int conta;
void atende(int sig);
int install_handler(void(*handler)(int), int timeout);
void start_alarm();
void stop_alarm();
int getFlag();