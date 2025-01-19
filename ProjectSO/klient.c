#include "ipc.h"
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <time.h> // Dodano dla funkcji time()

volatile sig_atomic_t* running;

void signal_handler(int signal) {
    if (signal == SIGINT) {
        *running = 0; // Ustaw flag� w pami�ci wsp�dzielonej
        printf("\nProces nadrz�dny: Restauracja zamkni�ta.\n");
    }
}

void client_process(int table_id, struct ConveyorBelt* belt, struct PidStorage* pid_storage, struct Table* tables) {
    printf("Klient (PID %d): Siedz� przy stoliku %d.\n", getpid(), table_id);

    int time_to_stay = 10 + rand() % 11; // Klient zostaje od 10 do 20 sekund
    while (*running && time_to_stay > 0) {
        sleep(1);
        time_to_stay--;

        if (!*running) {
            printf("Klient (PID %d): Restauracja zamkni�ta. Opuszczam stolik %d.\n", getpid(), table_id);
            break;
        }

        if (belt->count > 0) {
            belt->count--;
            printf("Klient (PID %d): Zabra�em talerz. Pozosta�o %d talerzy.\n", getpid(), belt->count);
        }
        else {
            printf("Klient (PID %d): Ta�ma jest pusta. Czekam...\n", getpid());
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
    shmdt((void*)running); // Konwersja do void* usuwaj�ca ostrze�enie

    printf("Klient (PID %d): Opuszczam restauracj�.\n", getpid());
    exit(0);
}

int find_free_table(struct Table* tables) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!tables[i].occupied) {
            return i;
        }
    }
    return -1; // Brak wolnych stolik�w
}

int main() {
    srand(time(NULL));

    int shm_id = shmget(SHM_KEY, sizeof(struct PidStorage), 0666 | IPC_CREAT);
    if (shm_id == -1) {
        perror("B��d shmget dla PID-�w");
        exit(1);
    }
    struct PidStorage* pid_storage = shmat(shm_id, NULL, 0);
    if (pid_storage == (void*)-1) {
        perror("B��d shmat dla PID-�w");
        exit(1);
    }

    int shm_belt_id = shmget(SHM_KEY + 1, sizeof(struct ConveyorBelt), 0666 | IPC_CREAT);
    if (shm_belt_id == -1) {
        perror("B��d shmget dla ta�my");
        exit(1);
    }
    struct ConveyorBelt* belt = shmat(shm_belt_id, NULL, 0);
    if (belt == (void*)-1) {
        perror("B��d shmat dla ta�my");
        exit(1);
    }

    int shm_table_id = shmget(SHM_KEY + 2, sizeof(struct Table) * MAX_PROCESSES, 0666 | IPC_CREAT);
    if (shm_table_id == -1) {
        perror("B��d shmget dla stolik�w");
        exit(1);
    }
    struct Table* tables = shmat(shm_table_id, NULL, 0);
    if (tables == (void*)-1) {
        perror("B��d shmat dla stolik�w");
        exit(1);
    }

    int shm_flag_id = shmget(SHM_KEY + 3, sizeof(sig_atomic_t), 0666 | IPC_CREAT);
    if (shm_flag_id == -1) {
        perror("B��d shmget dla flagi");
        exit(1);
    }
    running = shmat(shm_flag_id, NULL, 0);
    if (running == (void*)-1) {
        perror("B��d shmat dla flagi");
        exit(1);
    }
    *running = 1;

    signal(SIGINT, signal_handler);

    belt->count = 0;
    belt->start = 0;
    pid_storage->klient_count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        pid_storage->klient_pids[i] = 0;
        tables[i].occupied = 0;
        tables[i].client_pid = 0;
    }

    printf("Proces nadrz�dny: Restauracja otwarta.\n");

    while (*running) {
        int table_id = find_free_table(tables);
        if (table_id == -1) {
            printf("Brak wolnych stolik�w, klient czeka.\n");
            sleep(1);
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) {
            tables[table_id].occupied = 1;
            tables[table_id].client_pid = getpid();
            pid_storage->klient_pids[pid_storage->klient_count++] = getpid();
            client_process(table_id, belt, pid_storage, tables);
        }
        else if (pid < 0) {
            perror("B��d fork");
        }

        sleep(1);
    }

    printf("Proces nadrz�dny: Czekam na zako�czenie klient�w...\n");
    for (int i = 0; i < pid_storage->klient_count; i++) {
        if (pid_storage->klient_pids[i] != 0) {
            waitpid(pid_storage->klient_pids[i], NULL, 0);
        }
    }

    shmdt(belt);
    shmdt(pid_storage);
    shmdt(tables);
    shmdt((void*)running);

    shmctl(shm_id, IPC_RMID, NULL);
    shmctl(shm_belt_id, IPC_RMID, NULL);
    shmctl(shm_table_id, IPC_RMID, NULL);
    shmctl(shm_flag_id, IPC_RMID, NULL);

    printf("Proces nadrz�dny: Restauracja zamkni�ta.\n");
    return 0;
}
