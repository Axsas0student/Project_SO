#include "ipc.h"

struct PidStorage* pid_storage;
int shm_id;

void send_signal_to_kucharz(int signal) {
    if (pid_storage->kucharz_pid > 0) {
        printf("Kierownik: Wysy�am sygna� %d do kucharza (PID %d).\n", signal, pid_storage->kucharz_pid);
        kill(pid_storage->kucharz_pid, signal);
    }
}

void send_signal_to_all(int signal) {
    printf("Kierownik: Wysy�am sygna� %d do wszystkich proces�w.\n", signal);

    if (pid_storage->kucharz_pid > 0) kill(pid_storage->kucharz_pid, signal);
    if (pid_storage->obsluga_pid > 0) kill(pid_storage->obsluga_pid, signal);

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (pid_storage->klient_pids[i] > 0) {
            printf("Kierownik: Wysy�am sygna� %d do klienta (PID %d).\n", signal, pid_storage->klient_pids[i]);
            kill(pid_storage->klient_pids[i], signal);
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
        printf("\n1 - Przyspieszenie produkcji\n");
        printf("2 - Zmniejszenie produkcji\n");
        printf("3 - Zamkni�cie restauracji\n");
        printf("4 - Wyj�cie\n");

        int choice;
        scanf("%d", &choice);

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
            printf("Kierownik: Ko�cz� prac�.\n");
            return 0;
        }
    }

    return 0;
}
