#include "ipc.h"
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>
#include <errno.h>

volatile sig_atomic_t* running;

// Funkcja obs�uguj�ca sygna� SIGINT
void signal_handler(int signal) {
    if (signal == SIGINT) {
        *running = 0;
        printf("\nProces nadrz�dny: Restauracja zamkni�ta.\n");
    }
}

// Funkcje semafor�w z obs�ug� przerwa� (EINTR)
int lock_semaphore(int sem_id) {
    struct sembuf op = { 0, -1, 0 };
    int res;
    do {
        res = semop(sem_id, &op, 1);
    } while (res == -1 && errno == EINTR);
    if (res == -1) {
        perror("B��d semop (lock)");
        exit(1);
    }
    return res;
}

int unlock_semaphore(int sem_id) {
    struct sembuf op = { 0, 1, 0 };
    int res;
    do {
        res = semop(sem_id, &op, 1);
    } while (res == -1 && errno == EINTR);
    if (res == -1) {
        perror("B��d semop (unlock)");
        exit(1);
    }
    return res;
}

// Funkcja wykonuj�ca kod dla procesu klienta
void client_process(int table_id, struct ConveyorBelt* belt, struct PidStorage* pid_storage, struct Table* tables, int sem_id) {
    printf("Klient (PID %d): Siedz� przy stoliku %d.\n", getpid(), table_id);

    int time_to_stay = 10 + rand() % 11;
    while (*running && time_to_stay > 0) {
        sleep(1);
        time_to_stay--;

        if (!*running) {
            printf("Klient (PID %d): Restauracja zamkni�ta. Opuszczam stolik %d.\n", getpid(), table_id);
            break;
        }

        if (belt->count > 0) {
            lock_semaphore(sem_id);
            belt->count--;
            unlock_semaphore(sem_id);
            printf("Klient (PID %d): Zabra�em talerz. Pozosta�o %d talerzy.\n", getpid(), belt->count);
        }
        else {
            printf("Klient (PID %d): Ta�ma jest pusta. Czekam...\n", getpid());
        }
    }

    lock_semaphore(sem_id);
    tables[table_id].occupied = 0;
    tables[table_id].client_pid = 0;

    for (int i = 0; i < pid_storage->klient_count; i++) {
        if (pid_storage->klient_pids[i] == getpid()) {
            pid_storage->klient_pids[i] = pid_storage->klient_pids[pid_storage->klient_count - 1];
            pid_storage->klient_pids[pid_storage->klient_count - 1] = 0;
            pid_storage->klient_count--;
            break;
        }
    }
    unlock_semaphore(sem_id);

    shmdt(belt);
    shmdt(pid_storage);
    shmdt(tables);
    shmdt((void*)running);

    printf("Klient (PID %d): Opuszczam restauracj�.\n", getpid());
    exit(0);
}

// Funkcja do znalezienia wolnego stolika
int find_free_table(struct Table* tables) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!tables[i].occupied) {
            return i;
        }
    }
    return -1;
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

    int sem_id = semget(SHM_KEY + 4, 1, 0666 | IPC_CREAT);
    if (sem_id == -1) {
        perror("B��d semget");
        exit(1);
    }
    if (semctl(sem_id, 0, SETVAL, 1) == -1) {
        perror("B��d semctl");
        exit(1);
    }

    signal(SIGINT, signal_handler);

    belt->count = 0;
    pid_storage->klient_count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        pid_storage->klient_pids[i] = 0;
        tables[i].occupied = 0;
        tables[i].client_pid = 0;
    }

    printf("Proces nadrz�dny: Restauracja otwarta.\n");

    while (*running) {
        lock_semaphore(sem_id);
        int table_id = find_free_table(tables);
        unlock_semaphore(sem_id);

        if (table_id == -1) {
            printf("Brak wolnych stolik�w, klient czeka.\n");
            sleep(1);
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) {
            lock_semaphore(sem_id);
            tables[table_id].occupied = 1;
            tables[table_id].client_pid = getpid();
            pid_storage->klient_pids[pid_storage->klient_count++] = getpid();
            unlock_semaphore(sem_id);
            client_process(table_id, belt, pid_storage, tables, sem_id);
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

    semctl(sem_id, 0, IPC_RMID);

    printf("Proces nadrz�dny: Restauracja zamkni�ta.\n");
    return 0;
}
