#include "ipc.h"

int msg_id;

// Funkcja obs³uguj¹ca komunikaty klientów
void process_message(struct Message* message) {
    printf("Obs³uga: Otrzymano komunikat: %s\n", message->text);

    // Reakcja na treœæ komunikatu
    if (strcmp(message->text, "zamówienie") == 0) {
        printf("Obs³uga: Przyjêto zamówienie.\n");
    }
    else if (strcmp(message->text, "zap³aæ") == 0) {
        printf("Obs³uga: Klient poprosi³ o rachunek.\n");
    }
    else {
        printf("Obs³uga: Nieznane polecenie.\n");
    }
}

int main() {
    // Inicjalizacja kolejki komunikatów
    msg_id = msgget(MSG_KEY, 0666 | IPC_CREAT);
    if (msg_id == -1) {
        perror("Obs³uga: B³¹d msgget");
        exit(1);
    }

    printf("Obs³uga: Rozpoczynam pracê.\n");
    struct Message message;

    while (1) {
        // Odbiór komunikatu (bez blokowania, oczekiwanie na nowe wiadomoœci)
        if (msgrcv(msg_id, &message, sizeof(message.text), 0, IPC_NOWAIT) != -1) {
            process_message(&message);
        }
        sleep(1);
    }

    return 0;
}
