#define _POSIX_C_SOURCE 200809L
#include "ipc.h"
#include <signal.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct PidStorage* pid_storage;
int shm_id, msg_id;

int total_plates[10] = { 0 };
int total_revenue[10] = { 0 };
const int prices[10] = { 10, 15, 20, 40, 50, 60 };

void process_partial_bills() {
    struct Msg msg;
    while (msgrcv(msg_id, &msg, sizeof(msg) - sizeof(long), 1, IPC_NOWAIT) > 0) {
        int group_total = 0;

        for (int i = 0; i < 6; i++) {
            total_plates[i] += msg.plates[i];
            total_revenue[i] += msg.plates[i] * prices[i];
            group_total += msg.plates[i] * prices[i];
        }

        printf("Obs³uga: Grupa %d (zamkniêcie restauracji) zap³aci³a %d z³.\n", msg.group_id, group_total);
    }
}

void print_report() {
    int total_sum = 0;

    printf("\n--- Raport koñcowy ---\n");
    for (int i = 0; i < 6; i++) {
        printf("Talerz [%d]: Sprzedano %d, Zarobiono %d z³\n",
            i + 1, total_plates[i], total_revenue[i]);
        total_sum += total_revenue[i];
    }
    printf("\n£¹cznie zarobiono: %d z³\n", total_sum);
    printf("--------------------\n");
}

void cleanup() {
    process_partial_bills();
    print_report();
    shmdt(pid_storage);
    msgctl(msg_id, IPC_RMID, NULL);
    printf("Obs³uga: Zasoby zosta³y zwolnione.\n");
}

void signal_handler(int signal) {
    if (signal == SIGINT) {
        cleanup();
        exit(0);
    }
}

int main() {
    shm_id = shmget(SHM_KEY, sizeof(struct PidStorage), 0666 | IPC_CREAT);
    pid_storage = shmat(shm_id, NULL, 0);

    msg_id = msgget(MSG_KEY, IPC_CREAT | 0666);

    pid_storage->obsluga_pid = getpid();
    printf("Obs³uga: Mój PID to %d.\n", getpid());

    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigaction(SIGINT, &sa, NULL);

    while (1) {
        struct Msg msg;
        msgrcv(msg_id, &msg, sizeof(msg) - sizeof(long), 1, 0);

        int group_total = 0;
        for (int i = 0; i < 6; i++) {
            total_plates[i] += msg.plates[i];
            total_revenue[i] += msg.plates[i] * prices[i];
            group_total += msg.plates[i] * prices[i];
        }

        printf("Obs³uga: Grupa %d zap³aci³a %d z³.\n", msg.group_id, group_total);
    }

    return 0;
}
