#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

// Definicje sta³ych
#define BELT_CAPACITY 10
#define NUM_TABLES 5
#define SHM_KEY 1234
#define SEM_KEY 5678
#define SEM_TASMA 0

// Struktury danych
typedef struct {
    int price; // Cena talerzyka
} Plate;

typedef struct {
    Plate belt[BELT_CAPACITY];
    int belt_position; // Liczba talerzyków na taœmie
} SharedMemory;

// Zmienne globalne
SharedMemory* shared_memory = NULL;
int shm_id;
int sem_id;

// Funkcje zarz¹dzaj¹ce pamiêci¹ dzielon¹
void init_shared_memory() {
    shm_id = shmget(SHM_KEY, sizeof(SharedMemory), IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("B³¹d przy tworzeniu pamiêci dzielonej");
        exit(1);
    }
    shared_memory = (SharedMemory*)shmat(shm_id, NULL, 0);
    if (shared_memory == (void*)-1) {
        perror("B³¹d przy do³¹czaniu pamiêci dzielonej");
        exit(1);
    }

    // Inicjalizacja taœmy
    for (int i = 0; i < BELT_CAPACITY; i++) {
        shared_memory->belt[i].price = -1;
    }
    shared_memory->belt_position = 0;
}

void cleanup_shared_memory() {
    if (shmdt(shared_memory) == -1) {
        perror("B³¹d przy od³¹czaniu pamiêci dzielonej");
    }
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("B³¹d przy usuwaniu pamiêci dzielonej");
    }
    printf("Pamiêæ dzielona usuniêta.\n");
}

// Funkcje zarz¹dzaj¹ce semaforami
void init_semaphores() {
    sem_id = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (sem_id < 0) {
        perror("B³¹d przy tworzeniu semaforów");
        exit(1);
    }
    semctl(sem_id, SEM_TASMA, SETVAL, 0);
}

void cleanup_semaphores() {
    if (semctl(sem_id, 0, IPC_RMID) == -1) {
        perror("B³¹d przy usuwaniu semaforów");
    }
}

void sem_wait(int sem_num) {
    struct sembuf operation = { sem_num, -1, 0 };
    semop(sem_id, &operation, 1);
}

void sem_post(int sem_num) {
    struct sembuf operation = { sem_num, 1, 0 };
    semop(sem_id, &operation, 1);
}

// Funkcje kucharza
void produce_plates() {
    for (int i = 0; i < 10; i++) {
        sleep(1); // Symulacja przygotowania talerzyka
        Plate plate;
        plate.price = 10 + (rand() % 3) * 5; // Cena: 10, 15 lub 20 z³

        if (shared_memory->belt_position >= BELT_CAPACITY) {
            printf("Taœma pe³na! Oczekiwanie...\n");
            continue;
        }

        shared_memory->belt[shared_memory->belt_position] = plate;
        shared_memory->belt_position++;
        printf("Kucharz doda³ talerz o cenie %d.\n", plate.price);

        sem_post(SEM_TASMA); // Sygnalizacja, ¿e talerz jest gotowy
    }
}

// Funkcje obs³ugi
void serve_customers() {
    for (int i = 0; i < 10; i++) {
        sem_wait(SEM_TASMA); // Oczekiwanie na dostêpny talerz

        if (shared_memory->belt_position == 0) {
            printf("Taœma pusta! Oczekiwanie...\n");
            continue;
        }

        Plate plate = shared_memory->belt[0];
        for (int j = 1; j < shared_memory->belt_position; j++) {
            shared_memory->belt[j - 1] = shared_memory->belt[j];
        }
        shared_memory->belt_position--;
        printf("Obs³uga poda³a talerz o cenie %d.\n", plate.price);

        sleep(2); // Symulacja podania talerzyka
    }
}

// G³ówna funkcja
int main() {
    init_shared_memory();
    init_semaphores();

    pid_t pid;

    // Uruchom proces kucharza
    if ((pid = fork()) == 0) {
        produce_plates();
        exit(0);
    }

    // Uruchom proces obs³ugi
    if ((pid = fork()) == 0) {
        serve_customers();
        exit(0);
    }

    // Oczekiwanie na zakoñczenie procesów
    wait(NULL);
    wait(NULL);

    cleanup_shared_memory();
    cleanup_semaphores();

    return 0;
}