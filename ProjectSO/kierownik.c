#include "ipc.h"

struct PidStorage* pid_storage;
int shm_id;

void send_signal_to_all(int signal) {
    if (pid_storage->kucharz_pid > 0) {
        if (kill(pid_storage->kucharz_pid, signal) == -1) {
            perror("Kierownik: Nie uda³o siê wys³aæ sygna³u do kucharza");
        }
        else {
            printf("Kierownik: Wys³ano sygna³ %d do kucharza (PID %d).\n", signal, pid_storage->kucharz_pid);
        }
    }
    if (pid_storage->obsluga_pid > 0) {
        if (kill(pid_storage->obsluga_pid, signal) == -1) {
            perror("Kierownik: Nie uda³o siê wys³aæ sygna³u do obs³ugi");
        }
        else {
            printf("Kierownik: Wys³ano sygna³ %d do obs³ugi (PID %d).\n", signal, pid_storage->obsluga_pid);
        }
    }
    for (int i = 0; i < pid_storage->klient_count; i++) {
        if (pid_storage->klient_pids[i] > 0) {
            if (kill(pid_storage->klient_pids[i], signal) == -1) {
                perror("Kierownik: Nie uda³o siê wys³aæ sygna³u do klienta");
            }
            else {
                printf("Kierownik: Wys³ano sygna³ %d do klienta (PID %d).\n", signal, pid_storage->klient_pids[i]);
            }
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
        printf("Kierownik: Wybierz opcjê:\n");
        printf("1 - Przyspieszenie produkcji\n");
        printf("2 - Zmniejszenie produkcji\n");
        printf("3 - Zamkniêcie restauracji\n");
        printf("4 - Wyjœcie\n");

        int choice;
        scanf("%d", &choice);

        switch (choice) {
        case 1:
            send_signal_to_all(SIGUSR1);
            break;
        case 2:
            send_signal_to_all(SIGUSR2);
            break;
        case 3:
            printf("Kierownik: Wysy³am sygna³ zamkniêcia restauracji.\n");
            send_signal_to_all(SIGINT);
            break;
        case 4:
            printf("Kierownik: Koñczê pracê.\n");
            shmctl(shm_id, IPC_RMID, NULL);
            return 0;
        default:
            printf("Kierownik: Nieprawid³owy wybór.\n");
        }
    }

    return 0;
}
