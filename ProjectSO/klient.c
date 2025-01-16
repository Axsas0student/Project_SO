#define _POSIX_C_SOURCE 200809L
#include "ipc.h"
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/wait.h>

struct PidStorage* pid_storage;
struct ConveyorBelt* belt;
struct Table* tables;
int shm_id, shm_belt_id, shm_table_id;
volatile sig_atomic_t running = 1;  // Flaga kontroluj¹ca dzia³anie programu

void signal_handler(int signal) {
    if (signal == SIGINT) {
        running = 0;  // Ustawienie flagi na 0
        printf("\nProces nadrzêdny: Otrzymano SIGINT. Zamykam restauracjê.\n");
    }
}

void client_process(int table_id) {
    printf("Klient (PID %d): Siedzê przy stoliku %d.\n", getpid(), table_id);

    int time_to_stay = 10 + rand() % 11;  // Klient pozostaje przez 10–20 sekund
    while (running && time_to_stay > 0) {
        sleep(1);
        time_to_stay--;

        if (!running) {
            printf("Klient (PID %d): Restauracja zamkniêta. Opuszczam stolik %d.\n", getpid(), table_id);
            break;
        }

        if (belt->count > 0) {
            belt->count--;
            printf("Klient (PID %d): Zabra³em talerz. Pozosta³o %d talerzy.\n", getpid(), belt->count);
        }
        else {
            printf("Klient (PID %d): Taœma jest pusta. Czekam...\n", getpid());
        }
    }

    tables[table_id].occupied = 0;
    tables[table_id].client_pid = 0;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (pid_storage->klient_pids[i] == getpid()) {
            pid_storage->klient_pids[i] = 0;
            break;
        }
    }

    shmdt(belt);
    shmdt(pid_storage);
    shmdt(tables);

    printf("Klient (PID %d): Opuszczam restauracjê.\n", getpid());
    exit(0);
}

int find_free_table() {
    for (int i = 0; i < MAX_TABLES; i++) {
        if (!tables[i].occupied) return i;
    }
    return -1;
}

void close_all_clients() {
    printf("Proces nadrzêdny: Wysy³am sygna³ zamkniêcia do wszystkich klientów.\n");
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (pid_storage->klient_pids[i] > 0) {
            printf("Proces nadrzêdny: Wysy³am SIGINT do klienta (PID %d).\n", pid_storage->klient_pids[i]);
            if (kill(pid_storage->klient_pids[i], SIGINT) == -1) {
                perror("Nie uda³o siê wys³aæ SIGINT do klienta");
            }
        }
    }
}

int main() {
    srand(time(NULL));

    // Konfiguracja pamiêci wspó³dzielonej
    shm_id = shmget(SHM_KEY, sizeof(struct PidStorage), 0666 | IPC_CREAT);
    if (shm_id == -1) {
        perror("B³¹d shmget dla PID-ów");
        exit(1);
    }
    pid_storage = shmat(shm_id, NULL, 0);
    if (pid_storage == (void*)-1) {
        perror("B³¹d shmat dla PID-ów");
        exit(1);
    }

    shm_belt_id = shmget(SHM_KEY + 1, sizeof(struct ConveyorBelt), 0666 | IPC_CREAT);
    if (shm_belt_id == -1) {
        perror("B³¹d shmget dla taœmy");
        exit(1);
    }
    belt = shmat(shm_belt_id, NULL, 0);
    if (belt == (void*)-1) {
        perror("B³¹d shmat dla taœmy");
        exit(1);
    }

    shm_table_id = shmget(SHM_KEY + 2, sizeof(struct Table) * MAX_TABLES, 0666 | IPC_CREAT);
    if (shm_table_id == -1) {
        perror("B³¹d shmget dla stolików");
        exit(1);
    }
    tables = shmat(shm_table_id, NULL, 0);
    if (tables == (void*)-1) {
        perror("B³¹d shmat dla stolików");
        exit(1);
    }

    // Konfiguracja obs³ugi sygna³u SIGINT
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = SA_RESTART;  // Wznowienie przerwanych wywo³añ systemowych
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("B³¹d sigaction");
        exit(1);
    }

    // G³ówna pêtla
    while (running) {
        sleep(1);

        if (!running) {
            break;
        }

        int table_id = find_free_table();
        if (table_id == -1) {
            printf("Brak wolnych stolików, klient czeka.\n");
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) {
            // Proces klienta
            if (!running) {
                printf("Klient (PID %d): Restauracja zamkniêta przed rozpoczêciem. Wychodzê.\n", getpid());
                exit(0);
            }

            tables[table_id].occupied = 1;
            tables[table_id].client_pid = getpid();

            for (int i = 0; i < MAX_PROCESSES; i++) {
                if (pid_storage->klient_pids[i] == 0) {
                    pid_storage->klient_pids[i] = getpid();
                    break;
                }
            }

            client_process(table_id);
        }
        else if (pid < 0) {
            perror("B³¹d fork");
        }
    }

    close_all_clients();

    printf("Czekam na zakoñczenie klientów...\n");
    while (wait(NULL) > 0);

    printf("Wszyscy klienci zakoñczyli pracê. Restauracja zamkniêta.\n");

    shmdt(pid_storage);
    shmdt(belt);
    shmdt(tables);

    return 0;
}
