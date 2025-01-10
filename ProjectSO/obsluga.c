#include "ipc.h"

int msg_id;

// Funkcja obs�uguj�ca komunikaty klient�w
void process_message(struct Message* message) {
    printf("Obs�uga: Otrzymano komunikat: %s\n", message->text);

    // Reakcja na tre�� komunikatu
    if (strcmp(message->text, "zam�wienie") == 0) {
        printf("Obs�uga: Przyj�to zam�wienie.\n");
    }
    else if (strcmp(message->text, "zap�a�") == 0) {
        printf("Obs�uga: Klient poprosi� o rachunek.\n");
    }
    else {
        printf("Obs�uga: Nieznane polecenie.\n");
    }
}

int main() {
    // Inicjalizacja kolejki komunikat�w
    msg_id = msgget(MSG_KEY, 0666 | IPC_CREAT);
    if (msg_id == -1) {
        perror("Obs�uga: B��d msgget");
        exit(1);
    }

    printf("Obs�uga: Rozpoczynam prac�.\n");
    struct Message message;

    while (1) {
        // Odbi�r komunikatu (bez blokowania, oczekiwanie na nowe wiadomo�ci)
        if (msgrcv(msg_id, &message, sizeof(message.text), 0, IPC_NOWAIT) != -1) {
            process_message(&message);
        }
        sleep(1);
    }

    return 0;
}
