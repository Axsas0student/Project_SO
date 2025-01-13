#include "ipc.h"

struct PidStorage* pid_storage;
int shm_id;

void signal_handler(int signal) {
    if (signal == SIGINT) {
        printf("Obs³uga: Restauracja zamkniêta. Koñczê pracê.\n");
        exit(0);
    }
    else {
        printf("Obs³uga: Otrzymano sygna³ %d, ignorujê.\n", signal);
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

    signal(SIGUSR1, signal_handler);
    signal(SIGUSR2, signal_handler);
    signal(SIGINT, signal_handler);

    while (1) {
        sleep(1);
    }

    return 0;
}
