#ifndef IPC_H
#define IPC_H

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

#define SHM_KEY 1235678
#define SEM_KEY 1235679
#define MAX_PLATES 14
#define MAX_PROCESSES 10
#define MAX_TABLES 14

// Struktura taœmy sushi
struct ConveyorBelt {
    int plates[MAX_PLATES];
    int start;
    int end;
    int count;
};

// Struktura przechowuj¹ca PID-y procesów
struct PidStorage {
    pid_t kucharz_pid;
    pid_t obsluga_pid;
    pid_t klient_pids[MAX_PROCESSES];
    int klient_count;
};

// Struktura stolików
struct Table {
    int occupied;      // 0 - wolny, 1 - zajêty
    pid_t client_pid;  // PID klienta, który zajmuje stolik
    int size;
};

#endif
