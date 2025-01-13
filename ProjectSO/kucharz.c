#define _POSIX_C_SOURCE 200809L // Dla funkcji POSIX, takich jak sigaction i usleep
#include "ipc.h"
#include <signal.h>
#include <unistd.h> // Dla usleep

struct ConveyorBelt* belt;
struct PidStorage* pid_storage;
int shm_id, shm_belt_id;
float production_speed = 1.0; // Prêdkoœæ produkcji w sekundach

void signal_handler(int signal) {
    if (signal == SIGUSR1) {
        production_speed = (production_speed > 0.2) ? production_speed - 0.2 : 0.2;
        printf("Kucharz: Przyspieszam produkcjê (nowa prêdkoœæ: co %.1f sekundy).\n", production_speed);
    }
    else if (signal == SIGUSR2) {
        production_speed += 0.2;
        printf("Kucharz: Zwalniam produkcjê (nowa prêdkoœæ: co %.1f sekundy).\n", production_speed);
    }
    else if (signal == SIGINT) {
        printf("Kucharz: Zatrzymujê pracê.\n");
        shmdt(belt);
        shmdt(pid_storage);
        exit(0);
    }
    else {
        printf("Kucharz: Otrzymano nieznany sygna³ %d, ignorujê.\n", signal);
    }
}

void add_plate(int plate_type) {
    if (belt->count < MAX_PLATES) {
        belt->plates[belt->end] = plate_type;
        belt->end = (belt->end + 1) % MAX_PLATES;
        belt->count++;
        printf("Kucharz: Doda³em talerz typu %d (Liczba talerzy: %d).\n", plate_type, belt->count);
    }
    else {
        printf("Kucharz: Taœma pe³na, czekam.\n");
    }
}

int main() {
    shm_id = shmget(SHM_KEY, sizeof(struct PidStorage), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Kucharz: B³¹d shmget dla PID-ów");
        exit(1);
    }

    pid_storage = shmat(shm_id, NULL, 0);
    if (pid_storage == (void*)-1) {
        perror("Kucharz: B³¹d shmat dla PID-ów");
        exit(1);
    }

    if (pid_storage->kucharz_pid == 0 && pid_storage->obsluga_pid == 0 && pid_storage->klient_count == 0) {
        memset(pid_storage, 0, sizeof(struct PidStorage));
    }

    shm_belt_id = shmget(SHM_KEY + 1, sizeof(struct ConveyorBelt), IPC_CREAT | 0666);
    if (shm_belt_id == -1) {
        perror("Kucharz: B³¹d shmget dla taœmy");
        exit(1);
    }

    belt = shmat(shm_belt_id, NULL, 0);
    if (belt == (void*)-1) {
        perror("Kucharz: B³¹d shmat dla taœmy");
        exit(1);
    }

    memset(belt, 0, sizeof(struct ConveyorBelt));
    pid_storage->kucharz_pid = getpid();
    printf("Kucharz: Mój PID to %d.\n", getpid());

    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    while (1) {
        usleep((int)(production_speed * 1e6));
        add_plate(rand() % 3 + 1);
    }

    return 0;
}
