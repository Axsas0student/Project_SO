#include "ipc.h"
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>
#include <limits.h>
#include <errno.h>

volatile sig_atomic_t* running;

// Obs³uga sygna³u SIGINT (zamkniêcie restauracji)
void signal_handler(int signal) {
    if (signal == SIGINT) {
        *running = 0; // Flaga ustawiana na 0
        printf("Proces (PID %d): Otrzymano sygna³ zamkniêcia restauracji (SIGINT).\n", getpid());
    }
}

// Funkcja blokuj¹ca semafor
void lock_semaphore(int sem_id) {
    struct sembuf op = { 0, -1, 0 };
    semop(sem_id, &op, 1);
}

// Funkcja odblokowuj¹ca semafor
void unlock_semaphore(int sem_id) {
    struct sembuf op = { 0, 1, 0 };
    semop(sem_id, &op, 1);
}

// Funkcja do znalezienia odpowiedniego stolika
int find_table_for_group(struct Table* tables, int group_size, int sem_id) {
    int best_table = -1;
    int best_size = INT_MAX;

    lock_semaphore(sem_id);

    // Najpierw szukamy stolika idealnie dopasowanego
    for (int i = 0; i < MAX_TABLES; i++) {
        if (!tables[i].occupied && tables[i].size == group_size) {
            tables[i].occupied = 1;
            best_table = i;
            unlock_semaphore(sem_id);
            return best_table; // Idealny stolik znaleziony
        }
    }

    // Jeœli nie ma idealnego, szukamy wiêkszego
    for (int i = 0; i < MAX_TABLES; i++) {
        if (!tables[i].occupied && tables[i].size > group_size && tables[i].size < best_size) {
            best_table = i;
            best_size = tables[i].size;
        }
    }

    if (best_table != -1) {
        tables[best_table].occupied = 1; // Rezerwujemy stolik
    }

    unlock_semaphore(sem_id);
    return best_table; // Zwraca -1, jeœli brak wolnych stolików
}

// Funkcja do rejestracji PID-u klienta
void register_client_pid(struct PidStorage* pid_storage, int sem_id) {
    lock_semaphore(sem_id);
    if (pid_storage->klient_count < MAX_PROCESSES) {
        pid_storage->klient_pids[pid_storage->klient_count++] = getpid();
    }
    else {
        printf("Proces (PID %d): Osi¹gniêto maksymaln¹ liczbê klientów!\n", getpid());
    }
    unlock_semaphore(sem_id);
}

// Funkcja do usuniêcia PID-u klienta
void unregister_client_pid(struct PidStorage* pid_storage, int sem_id) {
    lock_semaphore(sem_id);
    for (int i = 0; i < pid_storage->klient_count; i++) {
        if (pid_storage->klient_pids[i] == getpid()) {
            pid_storage->klient_pids[i] = pid_storage->klient_pids[pid_storage->klient_count - 1];
            pid_storage->klient_count--;
            break;
        }
    }
    unlock_semaphore(sem_id);
}

void client_process(int table_id, int group_size, struct ConveyorBelt* belt, struct Table* tables, struct PidStorage* pid_storage, int sem_id) {
    signal(SIGINT, signal_handler); // Rejestracja obs³ugi sygna³u SIGINT

    int required_plates = group_size * 3;
    int plates_eaten = 0;
    int max_wait_time = 5;
    int waited_time = 0;

    printf("Grupa %d (PID %d): Przy stoliku %d (rozmiar stolika: %d).\n", group_size, getpid(), table_id, tables[table_id].size);

    while (*running && plates_eaten < required_plates) {
        lock_semaphore(sem_id);

        if (belt->count > 0) {
            int plate_type = belt->plates[belt->start];
            belt->start = (belt->start + 1) % MAX_PLATES;
            belt->count--;
            plates_eaten++;
            printf("Grupa %d (PID %d): Zjedliœmy talerz [%d]. Liczba zjedzonych: %d/%d.\n", group_size, getpid(), plate_type, plates_eaten, required_plates);
            waited_time = 0;
        }
        else {
            waited_time++;
            printf("Grupa %d (PID %d): Czekamy... (%d sekundy)\n", group_size, getpid(), waited_time);
        }

        unlock_semaphore(sem_id);

        // SprawdŸ, czy sygna³ zamkniêcia zosta³ otrzymany
        if (!*running) {
            printf("Grupa %d (PID %d): Otrzymano sygna³ zamkniêcia restauracji. Opuszczamy lokal.\n", group_size, getpid());
            break;
        }

        if (waited_time >= max_wait_time) {
            int special_order = 4 + rand() % 3;
            printf("Grupa %d (PID %d): Sk³adamy zamówienie specjalne na talerz %d.\n", group_size, getpid(), special_order);
            lock_semaphore(sem_id);
            tables[table_id].special_order = special_order;
            unlock_semaphore(sem_id);
            waited_time = 0;
        }

        sleep(1);
    }

    if (plates_eaten >= required_plates) {
        printf("Grupa %d (PID %d): Opuszczamy restauracjê po zjedzeniu %d talerzyków.\n", group_size, getpid(), plates_eaten);
    }

    unregister_client_pid(pid_storage, sem_id);

    lock_semaphore(sem_id);
    tables[table_id].occupied = 0;
    tables[table_id].client_pid = 0;
    unlock_semaphore(sem_id);

    exit(0);
}

int main() {
    srand(time(NULL));

    int shm_id = shmget(SHM_KEY, sizeof(struct PidStorage), 0666 | IPC_CREAT);
    struct PidStorage* pid_storage = shmat(shm_id, NULL, 0);

    int shm_belt_id = shmget(SHM_KEY + 1, sizeof(struct ConveyorBelt), 0666 | IPC_CREAT);
    struct ConveyorBelt* belt = shmat(shm_belt_id, NULL, 0);

    int shm_table_id = shmget(SHM_KEY + 2, sizeof(struct Table) * MAX_TABLES, 0666 | IPC_CREAT);
    struct Table* tables = shmat(shm_table_id, NULL, 0);

    int shm_flag_id = shmget(SHM_KEY + 3, sizeof(sig_atomic_t), 0666 | IPC_CREAT);
    running = shmat(shm_flag_id, NULL, 0);
    *running = 1;

    int sem_id = semget(SEM_KEY, 1, 0666 | IPC_CREAT);
    semctl(sem_id, 0, SETVAL, 1);

    // Inicjalizacja stolików
    for (int i = 0; i < MAX_TABLES; i++) {
        tables[i].occupied = 0;
        tables[i].client_pid = 0;
        tables[i].size = (i < 2) ? 4 : (i < 5) ? 3 : (i < 9) ? 2 : 1;
        tables[i].special_order = 0;
    }

    signal(SIGINT, signal_handler); // Rejestracja obs³ugi sygna³u SIGINT

    while (*running) {
        int group_size = (rand() % 4) + 1;

        int table_id = find_table_for_group(tables, group_size, sem_id);

        if (table_id == -1) {
            printf("Grupa %d: Brak wolnych stolików. Oczekujemy...\n", group_size);
            sleep(1);
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) {
            register_client_pid(pid_storage, sem_id);
            lock_semaphore(sem_id);
            tables[table_id].client_pid = getpid();
            unlock_semaphore(sem_id);

            client_process(table_id, group_size, belt, tables, pid_storage, sem_id);
        }
    }

    printf("Proces nadrzêdny: Restauracja zamkniêta.\n");

    return 0;
}
