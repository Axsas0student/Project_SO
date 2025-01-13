#define _POSIX_C_SOURCE 200809L
#include "ipc.h"
#include <signal.h>
#include <unistd.h>

struct PidStorage* pid_storage;
int shm_id;

void signal_handler(int signal) {
    if (signal == SIGINT) {
        printf("Obs³uga: Restauracja zamkniêta. Koñczê pracê.\n");
        exit(0);
    }
    else if (signal == SIGUSR1 || signal == SIGUSR2) {
        printf("Obs³uga: Otrzymano sygna³ %d, ignorujê.\n", signal);
    }
    else {
        printf("Obs³uga: Nieznany sygna³ %d, ignorujê.\n", signal);
    }
}

int main() {
    shm_id = shmget(SHM_KEY, sizeof(struct PidStorage), 0666 | IPC_CREAT);
    if (shm_id == -1) {
        perror("Obs³uga: B³¹d shmget");
        exit(1);
    }

    pid_storage = shmat(shm_id, NULL, 0);
    if (pid_storage == (void*)-1) {
        perror("Obs³uga: B³¹d shmat");
        exit(1);
    }

    pid_storage->obsluga_pid = getpid();
    printf("Obs³uga: Mój PID to %d.\n", getpid());

    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    // Ustawienie obs³ugi sygna³ów
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    while (1) {
        sleep(1); // Symulacja pracy obs³ugi
    }

    return 0;
}
