#include <stdio.h>
#include <n7OS/cpu.h>

/*
 * Processus de démonstration de la préemption. Muet par défaut ;
 * n'imprime que si p1_active vaut 1, ce qui est activé depuis la
 * demo étape 5. Quand inactif, dort sur sti+hlt sans consommer de
 * CPU.
 */

volatile int p1_active = 0;
volatile int p1_iter   = 0;

void processus1(void) {
    while (1) {
        while (p1_active) {
            printf("P1 iter %d\n", p1_iter++);

            // Boucle d'attente pour ralentir l'affichage.
            for (volatile int j = 0; j < 5000000; j++);
        }

        // Inactif : attente passive jusqu'à la prochaine IT.
        sti();
        hlt();
    }
}