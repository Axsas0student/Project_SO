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
}

int main() {
    shm_id = shmget(SHM_KEY, sizeof(struct PidStorage), 0666 | IPC_CREAT);
    pid_storage = shmat(shm_id, NULL, 0);

    pid_storage->obsluga_pid = getpid();
    printf("Obs³uga: Mój PID to %d.\n", getpid());

    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigaction(SIGINT, &sa, NULL);

    while (1) sleep(1);

    return 0;
}
