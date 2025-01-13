#include "ipc.h"

struct PidStorage* pid_storage;
int shm_id;

void send_signal_to_all(int signal) {
    if (pid_storage->kucharz_pid > 0) {
        if (kill(pid_storage->kucharz_pid, signal) == -1) {
            perror("Kierownik: Nie uda�o si� wys�a� sygna�u do kucharza");
        }
        else {
            printf("Kierownik: Wys�ano sygna� %d do kucharza (PID %d).\n", signal, pid_storage->kucharz_pid);
        }
    }
    if (pid_storage->obsluga_pid > 0) {
        if (kill(pid_storage->obsluga_pid, signal) == -1) {
            perror("Kierownik: Nie uda�o si� wys�a� sygna�u do obs�ugi");
        }
        else {
            printf("Kierownik: Wys�ano sygna� %d do obs�ugi (PID %d).\n", signal, pid_storage->obsluga_pid);
        }
    }
    for (int i = 0; i < pid_storage->klient_count; i++) {
        if (pid_storage->klient_pids[i] > 0) {
            if (kill(pid_storage->klient_pids[i], signal) == -1) {
                perror("Kierownik: Nie uda�o si� wys�a� sygna�u do klienta");
            }
            else {
                printf("Kierownik: Wys�ano sygna� %d do klienta (PID %d).\n", signal, pid_storage->klient_pids[i]);
            }
        }
    }
}

int main() {
    shm_id = shmget(SHM_KEY, sizeof(struct PidStorage), 0666 | IPC_CREAT);
    if (shm_id == -1) {
        perror("Kierownik: B��d shmget");
        exit(1);
    }

    pid_storage = shmat(shm_id, NULL, 0);
    if (pid_storage == (void*)-1) {
        perror("Kierownik: B��d shmat");
        exit(1);
    }

    printf("Kierownik: Rozpoczynam prac�.\n");

    while (1) {
        printf("Kierownik: Wybierz opcj�:\n");
        printf("1 - Przyspieszenie produkcji\n");
        printf("2 - Zmniejszenie produkcji\n");
        printf("3 - Zamkni�cie restauracji\n");
        printf("4 - Wyj�cie\n");

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
            printf("Kierownik: Wysy�am sygna� zamkni�cia restauracji.\n");
            send_signal_to_all(SIGINT);
            break;
        case 4:
            printf("Kierownik: Ko�cz� prac�.\n");
            shmctl(shm_id, IPC_RMID, NULL);
            return 0;
        default:
            printf("Kierownik: Nieprawid�owy wyb�r.\n");
        }
    }

    return 0;
}
