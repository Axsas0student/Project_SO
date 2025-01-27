#define _POSIX_C_SOURCE 200809L
#include "ipc.h"
#include <signal.h>
#include <unistd.h>
#include <time.h>

struct ConveyorBelt* belt;
struct PidStorage* pid_storage;
struct Table* tables;
int shm_id, shm_belt_id, shm_table_id;
float production_speed = 1.0;

void signal_handler(int signal) {
    if (signal == SIGUSR1) {
        production_speed = (production_speed > 0.1) ? production_speed - 0.1 : 0.1;
        printf("Kucharz: Przyspieszam produkcjê (co %.1f sekundy).\n", production_speed);
    }
    else if (signal == SIGUSR2) {
        production_speed += 0.1;
        printf("Kucharz: Zwalniam produkcjê (co %.1f sekundy).\n", production_speed);
    }
    else if (signal == SIGINT) {
        printf("Kucharz: Zatrzymujê pracê.\n");
        shmdt(belt);
        shmdt(pid_storage);
        shmdt(tables);
        exit(0);
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
    shm_belt_id = shmget(SHM_KEY + 1, sizeof(struct ConveyorBelt), IPC_CREAT | 0666);
    shm_table_id = shmget(SHM_KEY + 2, sizeof(struct Table) * MAX_TABLES, IPC_CREAT | 0666);

    pid_storage = shmat(shm_id, NULL, 0);
    belt = shmat(shm_belt_id, NULL, 0);
    tables = shmat(shm_table_id, NULL, 0);

    memset(belt, 0, sizeof(struct ConveyorBelt));
    pid_storage->kucharz_pid = getpid();
    printf("Kucharz: Mój PID to %d.\n", getpid());

    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    srand(time(NULL));
    while (1) {
        usleep((int)(production_speed * 1e6));

        // Obs³uga zamówieñ specjalnych
        for (int i = 0; i < MAX_TABLES; i++) {
            if (tables[i].special_order > 0) {
                int order = tables[i].special_order;
                tables[i].special_order = 0; // Zamówienie zrealizowane
                printf("Kucharz: Przygotowujê specjalny talerz typu %d dla stolika %d.\n", order, i);
                add_plate(order);
            }
        }

        // Produkcja standardowych talerzy
        add_plate(rand() % 3 + 1);
    }

    return 0;
}
