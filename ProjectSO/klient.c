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

#define MAX_CLIENTS 100

volatile sig_atomic_t* running;

// Deklaracja funkcji find_table_for_group
int find_table_for_group(struct Table* tables, int group_size);

// Funkcja obs�uguj�ca sygna� SIGINT
void signal_handler(int signal) {
    if (signal == SIGINT) {
        *running = 0; // Ustawienie flagi na 0 w pami�ci wsp�dzielonej
        printf("\nProces (PID %d): Otrzymano sygna� zamkni�cia restauracji (SIGINT).\n", getpid());
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

// Funkcja losuj�ca rozmiar grupy
int random_group_size() {
    int r = rand() % 100;
    if (r < 40) return 1;
    else if (r < 70) return 2;
    else if (r < 90) return 3;
    else return 4;
}

// Funkcja wykonuj�ca kod dla procesu klienta
void client_process(int table_id, int group_size, struct ConveyorBelt* belt, struct PidStorage* pid_storage, struct Table* tables, int sem_id) {
    signal(SIGINT, signal_handler); // Rejestracja obs�ugi sygna�u SIGINT w procesie potomnym

    printf("Grupa %d (PID %d): Siedzimy przy stoliku %d (rozmiar stolika: %d).\n", group_size, getpid(), table_id, tables[table_id].size);

    int time_to_stay = 10 + rand() % 11;
    int max_wait_time = 5;  // Maksymalny czas oczekiwania w sekundach
    int waited_time = 0;

    while (*running && time_to_stay > 0) {
        lock_semaphore(sem_id);
        if (belt->count > 0) {
            belt->count--;
            printf("Grupa %d (PID %d): Zabrali�my talerz. Pozosta�o %d talerzy.\n", group_size, getpid(), belt->count);
            waited_time = 0; // Zresetuj czas oczekiwania, je�li uda�o si� zabra� talerz
        }
        else {
            printf("Grupa %d (PID %d): Ta�ma jest pusta. Czekamy...\n", group_size, getpid());
            waited_time++;
        }
        unlock_semaphore(sem_id);

        if (waited_time >= max_wait_time) {
            printf("Grupa %d (PID %d): Zbyt d�ugo czekali�my. Opuszczamy restauracj�.\n", group_size, getpid());
            break;
        }

        sleep(1);
        time_to_stay--;
    }

    lock_semaphore(sem_id);
    tables[table_id].occupied = 0;
    tables[table_id].client_pid = 0;

    // Usu� PID procesu z listy klient�w
    for (int i = 0; i < pid_storage->klient_count; i++) {
        if (pid_storage->klient_pids[i] == getpid()) {
            pid_storage->klient_pids[i] = pid_storage->klient_pids[pid_storage->klient_count - 1];
            pid_storage->klient_count--;
            break;
        }
    }
    unlock_semaphore(sem_id);

    printf("Grupa %d (PID %d): Opuszczamy restauracj�.\n", group_size, getpid());
    exit(0);
}

// Funkcja do znalezienia odpowiedniego stolika dla grupy
int find_table_for_group(struct Table* tables, int group_size) {
    int best_table = -1;
    int best_size = INT_MAX;

    for (int i = 0; i < MAX_TABLES; i++) {
        if (!tables[i].occupied && tables[i].size >= group_size && tables[i].size < best_size) {
            best_table = i;
            best_size = tables[i].size;
        }
    }
    return best_table; // Zwraca najlepszy stolik lub -1, je�li brak
}

int main() {
    srand(time(NULL));

    signal(SIGINT, signal_handler); // Rejestracja obs�ugi sygna�u SIGINT

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

    int shm_table_id = shmget(SHM_KEY + 2, sizeof(struct Table) * MAX_TABLES, 0666 | IPC_CREAT);
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

    belt->count = 0;
    pid_storage->klient_count = 0;

    for (int i = 0; i < MAX_TABLES; i++) {
        tables[i].occupied = 0;
        tables[i].client_pid = 0;
        tables[i].size = (i < 2) ? 4 : (i < 5) ? 3 : (i < 9) ? 2 : 1;
    }

    printf("Proces nadrz�dny: Restauracja otwarta.\n");

    while (*running) {
        int group_size = random_group_size();

        lock_semaphore(sem_id);
        int table_id = find_table_for_group(tables, group_size);
        unlock_semaphore(sem_id);

        if (!*running) break;

        if (table_id == -1) {
            printf("Brak wolnych stolik�w, grupa %d oczekuje.\n", group_size);
            sleep(1);
            continue;
        }

        if (pid_storage->klient_count >= MAX_CLIENTS) {
            printf("Osi�gni�to maksymaln� liczb� klient�w. Restauracja zostaje zamkni�ta.\n");
            *running = 0; // Ustaw flag� zamkni�cia restauracji
            break;        // Przerwij g��wn� p�tl�
        }

        pid_t pid = fork();
        if (pid == 0) {
            signal(SIGINT, signal_handler);
            lock_semaphore(sem_id);
            tables[table_id].occupied = 1;
            tables[table_id].client_pid = getpid();
            unlock_semaphore(sem_id);
            client_process(table_id, group_size, belt, pid_storage, tables, sem_id);
        }
        else if (pid < 0) {
            perror("B��d fork");
        }
        else {
            lock_semaphore(sem_id);
            pid_storage->klient_pids[pid_storage->klient_count++] = pid;
            unlock_semaphore(sem_id);
            printf("Grupa %d os�b (PID %d): Przydzielono stolik %d (rozmiar stolika: %d).\n", group_size, pid, table_id, tables[table_id].size);
        }

        sleep(1);
        int delay = 500000 + rand() % 1000000; // Op�nienie od 5s do 1.5s
        usleep(delay);
    }

    for (int i = 0; i < pid_storage->klient_count; i++) {
        if (pid_storage->klient_pids[i] != 0) {
            kill(pid_storage->klient_pids[i], SIGINT);
        }
    }

    printf("Proces nadrz�dny: Czekam na zako�czenie klient�w...\n");
    for (int i = 0; i < pid_storage->klient_count; i++) {
        if (pid_storage->klient_pids[i] != 0) {
            waitpid(pid_storage->klient_pids[i], NULL, 0);
        }
    }

    if (shmdt(belt) == -1) perror("B��d shmdt (belt)");
    if (shmdt(pid_storage) == -1) perror("B��d shmdt (pid_storage)");
    if (shmdt(tables) == -1) perror("B��d shmdt (tables)");
    if (shmdt((void*)running) == -1) perror("B��d shmdt (running)");

    shmctl(shm_id, IPC_RMID, NULL);
    shmctl(shm_belt_id, IPC_RMID, NULL);
    shmctl(shm_table_id, IPC_RMID, NULL);
    shmctl(shm_flag_id, IPC_RMID, NULL);

    semctl(sem_id, 0, IPC_RMID);

    printf("Proces nadrz�dny: Restauracja zamkni�ta.\n");
    return 0;
}
