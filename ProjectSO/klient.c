#include "ipc.h"
#include <signal.h>
#include <sys/wait.h>

int shm_id;
struct ConveyorBelt* belt;

// Funkcja odczytu talerza z taœmy
void take_plate() {
    if (belt->count > 0) {
        int plate_type = belt->plates[belt->start];
        belt->start = (belt->start + 1) % MAX_PLATES;
        belt->count--;
        printf("Klient (PID %d): Zabra³em talerz typu %d. (Liczba talerzy na taœmie: %d)\n", getpid(), plate_type, belt->count);
    }
    else {
        printf("Klient (PID %d): Taœma pusta, czekam...\n", getpid());
    }
}

// Funkcja obs³uguj¹ca nowego klienta
void client_process() {
    printf("Nowy klient (PID %d): Wszed³em do restauracji.\n", getpid());

    for (int i = 0; i < 3; i++) { // Ka¿dy klient próbuje zabraæ talerz 3 razy
        sleep(1);
        take_plate();
    }

    printf("Klient (PID %d): Wychodzê z restauracji.\n", getpid());
    exit(0); // Zakoñczenie procesu klienta
}

// Obs³uga sygna³u zamkniêcia restauracji
void signal_handler(int signal) {
    if (signal == SIGINT) {
        printf("Klient: Otrzymano sygna³ zamkniêcia restauracji. Koñczê pracê.\n");
        shmdt(belt);
        while (wait(NULL) > 0); // Czekanie na zakoñczenie procesów potomnych
        exit(0);
    }
}

int main() {
    // Obs³uga sygna³u zamkniêcia restauracji
    signal(SIGINT, signal_handler);

    // Inicjalizacja pamiêci wspó³dzielonej
    shm_id = shmget(SHM_KEY, sizeof(struct ConveyorBelt), 0666);
    if (shm_id == -1) {
        perror("Klient: B³¹d shmget");
        exit(1);
    }

    belt = shmat(shm_id, NULL, 0);
    if (belt == (void*)-1) {
        perror("Klient: B³¹d shmat");
        exit(1);
    }

    printf("Klient: Rozpoczynam tworzenie klientów.\n");

    while (1) {
        sleep(3); // Nowy klient co 3 sekundy
        pid_t pid = fork();
        if (pid == 0) {
            client_process();
        }
        else if (pid < 0) {
            perror("Klient: B³¹d fork");
        }
    }

    return 0;
}
