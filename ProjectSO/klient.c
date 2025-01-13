#include "ipc.h"

struct ConveyorBelt* belt;
struct PidStorage* pid_storage;
int shm_id, shm_belt_id;
volatile sig_atomic_t running = 1;

void signal_handler(int signal) {
    if (signal == SIGINT) {
        running = 0; // Ustaw flagê zatrzymania
        printf("Proces nadrzêdny klienta: Restauracja zamkniêta.\n");
    }
}

void take_plate() {
    if (belt->count > 0) {
        int plate_type = belt->plates[belt->start];
        belt->start = (belt->start + 1) % MAX_PLATES;
        belt->count--;
        printf("Klient (PID %d): Zabra³em talerz typu %d. (Liczba talerzy: %d)\n                                                                                                                                                             ", getpid(), plate_type, belt->count);
    }
    else {
        printf("Klient (PID %d): Taœma pusta, czekam...\n", getpid());
    }
}

void client_process() {
    printf("Nowy klient (PID %d): Wszed³em do restauracji.\n", getpid());
    for (int i = 0; i < 5; i++) {
        sleep(1);
        take_plate();
    }
    printf("Klient (PID %d): Wychodzê z restauracji.\n", getpid());

    for (int i = 0; i < pid_storage->klient_count; i++) {
        if (pid_storage->klient_pids[i] == getpid()) {
            for (int j = i; j < pid_storage->klient_count - 1; j++) {
                pid_storage->klient_pids[j] = pid_storage->klient_pids[j + 1];
            }
            pid_storage->klient_count--;
            break;
        }
    }
    shmdt(belt);
    shmdt(pid_storage);
    exit(0);
}

int main() {
    shm_id = shmget(SHM_KEY, sizeof(struct PidStorage), 0666 | IPC_CREAT);
    if (shm_id == -1) {
        perror("Klient: B³¹d shmget dla PID-ów");
        exit(1);
    }

    pid_storage = shmat(shm_id, NULL, 0);
    if (pid_storage == (void*)-1) {
        perror("Klient: B³¹d shmat dla PID-ów");
        exit(1);
    }

    shm_belt_id = shmget(SHM_KEY + 1, sizeof(struct ConveyorBelt), 0666 | IPC_CR                                                                                                                                                             EAT);
    if (shm_belt_id == -1) {
        perror("Klient: B³¹d shmget dla taœmy");
        exit(1);
    }

    belt = shmat(shm_belt_id, NULL, 0);
    if (belt == (void*)-1) {
        perror("Klient: B³¹d shmat dla taœmy");
        exit(1);
    }

    signal(SIGINT, signal_handler);

    while (running) {
        sleep(3);
        pid_t pid = fork();
        if (pid == 0) {
            if (pid_storage->klient_count < MAX_PROCESSES) {
                pid_storage->klient_pids[pid_storage->klient_count++] = getpid();
                client_process();
            }
            else {
                printf("Klient: Nie mo¿na dodaæ wiêcej klientów.\n");
                shmdt(pid_storage);
                shmdt(belt);
                exit(1);
            }
        }
        else if (pid < 0) {
            perror("Klient: B³¹d fork");
        }
    }

    shmdt(pid_storage);
    shmdt(belt);
    return 0;
}
