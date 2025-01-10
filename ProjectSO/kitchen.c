#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "shared_memory.c"
#include "semaphores.c"

void produce_plate() {
    // Funkcja generuje talerze i umieszcza na taœmie
    printf("Kucharz przygotowuje talerz...\n");
    sleep(1); // Symulacja czasu przygotowania
}

int main() {
    init_shared_memory();
    init_semaphores();

    for (int i = 0; i < 10; i++) {
        produce_plate();
        sem_post(get_semaphore(SEM_TASMA)); // Sygnalizacja, ¿e talerz jest gotowy
    }

    cleanup_shared_memory();
    cleanup_semaphores();

    return 0;
}