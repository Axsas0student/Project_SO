#include "ipc.h"

void send_signal(int signal) {
    // Wysy³amy sygna³ do wszystkich procesów w grupie
    kill(0, signal);
}

int main() {
    printf("Kierownik: Rozpoczynam pracê.\n");

    while (1) {
        printf("Kierownik: Wybierz opcjê:\n");
        printf("1 - Przyspieszenie produkcji\n");
        printf("2 - Zmniejszenie produkcji\n");
        printf("3 - Zamkniêcie restauracji (tylko kucharz i obs³uga)\n");
        printf("4 - Wyjœcie\n");

        int choice;
        scanf("%d", &choice);

        switch (choice) {
        case 1:
            send_signal(SIGUSR1); // Przyspiesz produkcjê
            break;
        case 2:
            send_signal(SIGUSR2); // Zwolnij produkcjê
            break;
        case 3:
            printf("Kierownik: Wysy³am sygna³ zamkniêcia restauracji.\n");
            send_signal(SIGINT); // Zatrzymaj kucharza i obs³ugê
            break;
        case 4:
            printf("Kierownik: Koñczê pracê.\n");
            return 0;
        default:
            printf("Kierownik: Nieprawid³owy wybór.\n");
        }
    }

    return 0;
}
