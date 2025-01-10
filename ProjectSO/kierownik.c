#include "ipc.h"

void send_signal(int signal) {
    // Wysy�amy sygna� do wszystkich proces�w w grupie
    kill(0, signal);
}

int main() {
    printf("Kierownik: Rozpoczynam prac�.\n");

    while (1) {
        printf("Kierownik: Wybierz opcj�:\n");
        printf("1 - Przyspieszenie produkcji\n");
        printf("2 - Zmniejszenie produkcji\n");
        printf("3 - Zamkni�cie restauracji (tylko kucharz i obs�uga)\n");
        printf("4 - Wyj�cie\n");

        int choice;
        scanf("%d", &choice);

        switch (choice) {
        case 1:
            send_signal(SIGUSR1); // Przyspiesz produkcj�
            break;
        case 2:
            send_signal(SIGUSR2); // Zwolnij produkcj�
            break;
        case 3:
            printf("Kierownik: Wysy�am sygna� zamkni�cia restauracji.\n");
            send_signal(SIGINT); // Zatrzymaj kucharza i obs�ug�
            break;
        case 4:
            printf("Kierownik: Ko�cz� prac�.\n");
            return 0;
        default:
            printf("Kierownik: Nieprawid�owy wyb�r.\n");
        }
    }

    return 0;
}
