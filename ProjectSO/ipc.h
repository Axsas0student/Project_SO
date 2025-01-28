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
#define MSG_KEY 1235680
#define MAX_PLATES 14
#define MAX_PROCESSES 15
#define MAX_TABLES 14

// Struktura ta�my sushi
struct ConveyorBelt {
    int plates[MAX_PLATES];     //typy talerzy
    int start;                  //pocz�tek i koniec w buforze
    int end;
    int count;                  //liczba talerzy na ta�mie
};

// Struktura przechowuj�ca PID-y proces�w
struct PidStorage {
    pid_t kucharz_pid;
    pid_t obsluga_pid;
    pid_t klient_pids[MAX_PROCESSES];
    int klient_count;           //l aktywnych klient�w
};

// Struktura stolik�w
struct Table {
    int occupied;      // 0 - wolny, 1 - zaj�ty
    pid_t client_pid;  // PID klienta, kt�ry zajmuje stolik
    int size;           //rozmiar sotlika
    int special_order;  //zamowienie specjalne
};

struct Msg {
    long mtype;          // Typ wiadomo�ci (PID klienta lub 1 dla obs�ugi)
    int group_id;        // ID grupy (PID klienta)
    int plates[6];       // Liczba zjedzonych talerzyk�w wed�ug rodzaju
};

#endif
