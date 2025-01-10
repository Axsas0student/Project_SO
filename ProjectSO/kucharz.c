#include "ipc.h"

struct ConveyorBelt* belt;
int shm_id;

void signal_handler(int signal) {
    if (signal == SIGUSR1) {
        printf("Kucharz: Przyspieszam produkcjê.\n");
    }
    else if (signal == SIGUSR2) {
        printf("Kucharz: Zwalniam produkcjê.\n");
    }
    else if (signal == SIGINT) {
        printf("Kucharz: Zatrzymujê pracê.\n");
        shmdt(belt);
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
    signal(SIGUSR1, signal_handler);
    signal(SIGUSR2, signal_handler);
    signal(SIGINT, signal_handler);

    shm_id = shmget(SHM_KEY, sizeof(struct ConveyorBelt), 0666 | IPC_CREAT);
    if (shm_id == -1) {
        perror("Kucharz: B³¹d shmget");
        exit(1);
    }

    belt = shmat(shm_id, NULL, 0);
    if (belt == (void*)-1) {
        perror("Kucharz: B³¹d shmat");
        exit(1);
    }

    printf("Kucharz: Rozpoczynam produkcjê.\n");

    while (1) {
        sleep(1);
        add_plate(rand() % 3 + 1);
    }

    return 0;
}
