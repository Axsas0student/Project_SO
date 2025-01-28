#include "ipc.h"

struct PidStorage* pid_storage;
int shm_id;

void send_signal_to_kucharz(int signal) {
    if (pid_storage->kucharz_pid > 0) {
        printf("Kierownik: Wysy³am sygna³ %d do kucharza (PID %d).\n", signal, pid_storage->kucharz_pid);
        kill(pid_storage->kucharz_pid, signal);
    }
}

void send_signal_to_all(int signal) {
    printf("Kierownik: Wysy³am sygna³ %d do wszystkich procesów.\n", signal);

    if (pid_storage->kucharz_pid > 0) kill(pid_storage->kucharz_pid, signal);
    if (pid_storage->obsluga_pid > 0) kill(pid_storage->obsluga_pid, signal);

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (pid_storage->klient_pids[i] > 0) {
            printf("Kierownik: Wysy³am sygna³ %d do klienta (PID %d).\n", signal, pid_storage->klient_pids[i]);
            kill(pid_storage->klient_pids[i], signal);
        }
    }
}

int main() {
    shm_id = shmget(SHM_KEY, sizeof(struct PidStorage), 0666 | IPC_CREAT);
    if (shm_id == -1) {
        perror("Kierownik: B³¹d shmget");
        exit(1);
    }

    pid_storage = shmat(shm_id, NULL, 0);
    if (pid_storage == (void*)-1) {
        perror("Kierownik: B³¹d shmat");
        exit(1);
    }

    printf("Kierownik: Rozpoczynam pracê.\n");

    while (1) {
        printf("\n1 - Przyspieszenie produkcji\n");
        printf("2 - Zmniejszenie produkcji\n");
        printf("3 - Zamkniêcie restauracji\n");
        printf("4 - Wyjœcie\n");

        int choice;
        printf("Wybierz opcjê: ");
        if (scanf("%d", &choice) != 1) {
            // Obs³uga b³êdnego wejœcia
            printf("B³êdna opcja. WprowadŸ liczbê od 1 do 4.\n");
            while (getchar() != '\n'); // Oczyszczanie bufora wejœciowego
            continue;
        }

        switch (choice) {
        case 1:
            send_signal_to_kucharz(SIGUSR1);
            break;
        case 2:
            send_signal_to_kucharz(SIGUSR2);
            break;
        case 3:
            send_signal_to_all(SIGINT);
            break;
        case 4:
            shmctl(shm_id, IPC_RMID, NULL);
            printf("Kierownik: Koñczê pracê.\n");
            return 0;
        default:
            printf("B³êdna opcja. WprowadŸ liczbê od 1 do 4.\n");
        }
    }

    return 0;
}
