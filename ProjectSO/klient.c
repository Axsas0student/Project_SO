#include "ipc.h"
#include <signal.h>
#include <sys/wait.h>

int shm_id;
struct ConveyorBelt* belt;

// Funkcja odczytu talerza z ta�my
void take_plate() {
    if (belt->count > 0) {
        int plate_type = belt->plates[belt->start];
        belt->start = (belt->start + 1) % MAX_PLATES;
        belt->count--;
        printf("Klient (PID %d): Zabra�em talerz typu %d. (Liczba talerzy na ta�mie: %d)\n", getpid(), plate_type, belt->count);
    }
    else {
        printf("Klient (PID %d): Ta�ma pusta, czekam...\n", getpid());
    }
}

// Funkcja obs�uguj�ca nowego klienta
void client_process() {
    printf("Nowy klient (PID %d): Wszed�em do restauracji.\n", getpid());

    for (int i = 0; i < 3; i++) { // Ka�dy klient pr�buje zabra� talerz 3 razy
        sleep(1);
        take_plate();
    }

    printf("Klient (PID %d): Wychodz� z restauracji.\n", getpid());
    exit(0); // Zako�czenie procesu klienta
}

// Obs�uga sygna�u zamkni�cia restauracji
void signal_handler(int signal) {
    if (signal == SIGINT) {
        printf("Klient: Otrzymano sygna� zamkni�cia restauracji. Ko�cz� prac�.\n");
        shmdt(belt);
        while (wait(NULL) > 0); // Czekanie na zako�czenie proces�w potomnych
        exit(0);
    }
}

int main() {
    // Obs�uga sygna�u zamkni�cia restauracji
    signal(SIGINT, signal_handler);

    // Inicjalizacja pami�ci wsp�dzielonej
    shm_id = shmget(SHM_KEY, sizeof(struct ConveyorBelt), 0666);
    if (shm_id == -1) {
        perror("Klient: B��d shmget");
        exit(1);
    }

    belt = shmat(shm_id, NULL, 0);
    if (belt == (void*)-1) {
        perror("Klient: B��d shmat");
        exit(1);
    }

    printf("Klient: Rozpoczynam tworzenie klient�w.\n");

    while (1) {
        sleep(3); // Nowy klient co 3 sekundy
        pid_t pid = fork();
        if (pid == 0) {
            client_process();
        }
        else if (pid < 0) {
            perror("Klient: B��d fork");
        }
    }

    return 0;
}
