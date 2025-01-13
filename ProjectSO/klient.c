#include "ipc.h"
#include <sys/wait.h>
#include <signal.h>

struct ConveyorBelt* belt;
struct PidStorage* pid_storage;
int shm_id, shm_belt_id, shm_flag_id;
volatile sig_atomic_t* running; // Flaga wsp�dzielona

void signal_handler(int signal) {
    if (signal == SIGINT) {
        *running = 0; // Ustaw flag� zatrzymania w pami�ci wsp�dzielonej
        printf("Proces nadrz�dny klienta: Restauracja zamkni�ta.\n");
    }
}

void take_plate() {
    if (belt->count > 0) {
        int plate_type = belt->plates[belt->start];
        belt->start = (belt->start + 1) % MAX_PLATES;
        belt->count--;
        printf("Klient (PID %d): Zabra�em talerz typu %d. (Liczba talerzy: %d)\n", getpid(), plate_type, belt->count);
    }
    else {
        printf("Klient (PID %d): Ta�ma pusta, czekam...\n", getpid());
    }
}

void client_process() {
    printf("Nowy klient (PID %d): Wszed�em do restauracji.\n", getpid());
    for (int i = 0; i < 5; i++) {
        if (!*running) break; // Sprawd�, czy restauracja zosta�a zamkni�ta
        sleep(1);
        take_plate();
    }
    printf("Klient (PID %d): Wychodz� z restauracji.\n", getpid());

    // Usu� PID klienta z listy
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
    shmdt(running);
    exit(0);
}

int main() {
    // Utworzenie pami�ci wsp�dzielonej
    shm_id = shmget(SHM_KEY, sizeof(struct PidStorage), 0666 | IPC_CREAT);
    if (shm_id == -1) {
        perror("Klient: B��d shmget dla PID-�w");
        exit(1);
    }
    pid_storage = shmat(shm_id, NULL, 0);
    if (pid_storage == (void*)-1) {
        perror("Klient: B��d shmat dla PID-�w");
        exit(1);
    }

    shm_belt_id = shmget(SHM_KEY + 1, sizeof(struct ConveyorBelt), 0666 | IPC_CREAT);
    if (shm_belt_id == -1) {
        perror("Klient: B��d shmget dla ta�my");
        exit(1);
    }
    belt = shmat(shm_belt_id, NULL, 0);
    if (belt == (void*)-1) {
        perror("Klient: B��d shmat dla ta�my");
        exit(1);
    }

    shm_flag_id = shmget(SHM_KEY + 2, sizeof(sig_atomic_t), 0666 | IPC_CREAT);
    if (shm_flag_id == -1) {
        perror("Klient: B��d shmget dla flagi");
        exit(1);
    }
    running = shmat(shm_flag_id, NULL, 0);
    if (running == (void*)-1) {
        perror("Klient: B��d shmat dla flagi");
        exit(1);
    }

    // Inicjalizacja flagi
    *running = 1;

    // Rejestracja obs�ugi sygna�u SIGINT
    signal(SIGINT, signal_handler);

    while (*running) {
        sleep(3); // Czas pomi�dzy pojawianiem si� nowych klient�w
        if (!*running) break; // Je�li restauracja zamkni�ta, zako�cz p�tl�

        pid_t pid = fork();
        if (pid == 0) {
            // Proces potomny (klient)
            if (pid_storage->klient_count < MAX_PROCESSES) {
                pid_storage->klient_pids[pid_storage->klient_count++] = getpid();
                client_process();
            }
            else {
                printf("Klient: Nie mo�na doda� wi�cej klient�w.\n");
                shmdt(pid_storage);
                shmdt(belt);
                shmdt(running);
                exit(1);
            }
        }
        else if (pid < 0) {
            perror("Klient: B��d fork");
        }
    }

    // Poczekaj na zako�czenie wszystkich klient�w
    printf("Proces nadrz�dny klienta: Czekam na zako�czenie klient�w...\n");
    for (int i = 0; i < pid_storage->klient_count; i++) {
        waitpid(pid_storage->klient_pids[i], NULL, 0);
    }

    printf("Proces nadrz�dny klienta: Wszyscy klienci zako�czyli prac�.\n");

    shmdt(pid_storage);
    shmdt(belt);
    shmdt(running);

    return 0;
}
