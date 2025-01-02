#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "shared_memory.c"
#include "semaphores.c"

void serve_customer() {
    printf("Obs³uga podaje klientowi...\n");
    sleep(2); // Symulacja czasu obs³ugi
}

int main() {
    init_shared_memory();
    init_semaphores();

    for (int i = 0; i < 10; i++) {
        sem_wait(get_semaphore(SEM_TASMA)); // Oczekiwanie na dostêpne talerze
        serve_customer();
    }

    cleanup_shared_memory();
    cleanup_semaphores();

    return 0;
}
