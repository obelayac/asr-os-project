#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <n7OS/process.h>
#include <n7OS/keyboard.h>
#include <n7OS/cpu.h>
#include <n7OS/mem.h>

#define DEMO_LINE_MAX 16

static int fibonacci(int n) {
    if (n < 2) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// --- Etape 1 : caractères imprimables et caractères de contrôle.
static void demo_console(void) {
    printf("\n--- Etape 1 : Console ---\n");

    printf("Bienvenue sur n7OS !\n");
    printf("Fibonacci(5) = %d\n", fibonacci(5));

    // Tabulation
    printf("Avant\tApres tab\n");

    // Retour chariot : la deuxième moitie ecrase la premiere
    printf("Premiere version d'une ligne\rDeuxieme\n");

    // Backspace : "XYZ\b\bAB" -> "XAB"
    printf("XYZ\b\bAB\n");
}

// --- Etape 2 : interruption logicielle (vecteur 50).
static void demo_interrupt(void) {
    printf("\n--- Etape 2 : Interruption logicielle ---\n");
    printf("Avant l'interruption 50...\n");
    __asm__("int $50");
    printf("Apres l'interruption 50.\n");
}

// --- Etape 3 : appel système example (retourne 1).
static void demo_syscall(void) {
    printf("\n--- Etape 3 : Appel systeme example ---\n");
    int r = example();
    printf("example() a retourne %d (attendu : 1)\n", r);
    if (r == 1) {
        printf("Appel systeme example OK\n");
    } else {
        printf("Appel systeme example ECHEC\n");
    }
}

// --- Etape 4 : timer. L'horloge en haut à droite parle d'elle-même.
static void demo_timer(void) {
    printf("\n--- Etape 4 : Timer ---\n");
    printf("L'horloge en haut a droite est cadencee par le PIT a 1 kHz.\n");
    printf("Le handler de l'IRQ 0 incremente un compteur de ticks et\n");
    printf("rafraichit l'affichage en ecrivant directement en memoire video.\n");
    printf("Regardez en haut a droite : HH:MM:SS doit avancer en temps reel.\n");
}

static const char *etat_to_str(int state) {
    switch (state) {
        case LIBRE:  return "LIBRE";
        case PRET:   return "PRET";
        case ELU:    return "ELU";
        case BLOQUE: return "BLOQUE";
        default:     return "?";
    }
}

static void print_process_table(void) {
    printf("  pid   nom              etat\n");
    for (int i = 0; i < NB_PROC; i++) {
        if (process_table[i].state != LIBRE) {
            printf("  %d     %s\t  %s\n",
                   (int) process_table[i].pid,
                   process_table[i].name,
                   etat_to_str(process_table[i].state));
        }
    }
}

// Drapeaux de processus1 (bin/processus1.c). On les active pendant
// la demo etape 5 pour rendre la préemption visible.
extern volatile int p1_active;
extern volatile int p1_iter;

/*
 * --- Etape 5 : processus et ordonnancement.
 * On affiche la table, on active P1, puis on attend 'q'. P1 et la
 * boucle d'attente partagent le CPU via la preemption par le timer,
 * donc on voit "P1 iter N" defiler en parallèle de l'attente.
 */
static void demo_process(void) {
    printf("\n--- Etape 5 : Processus et ordonnancement ---\n");
    printf("Table des processus :\n");
    print_process_table();

    printf("\nOrdonnancement : tourniquet (round-robin) sur la table,\n");
    printf("preemption toutes les 50 ms par l'IRQ 0 (timer).\n");
    printf("idle dort entre les tours, le shell execute cette demo.\n");

    printf("\nActivation de processus1. Tapez 'q' pour le stopper.\n\n");
    p1_iter = 0;
    p1_active = 1;

    // Attente passive sur sti+hlt. A chaque reveil (timer ou
    // clavier), on regarde si une touche est arrivée ; 'q' sort.
    while (1) {
        sti();
        hlt();

        if (kbd_has_input()) {
            char c = kgetch();
            if (c == 'q' || c == 'Q') {
                break;
            }
        }
    }

    p1_active = 0;
    printf("\nProcessus1 mis en sommeil.\n");
    printf("Les iterations P1 ont progresse en parallele de cette\n");
    printf("demo : preuve visible de la preemption.\n");
}

// --- Etape 6 : clavier. Lit 3 touches et affiche leur code ASCII.
static void demo_keyboard(void) {
    printf("\n--- Etape 6 : Clavier ---\n");
    printf("Tapez 3 caracteres (chacun suivi d'Entree).\n");
    printf("Pour chacun on affichera son code ASCII.\n");

    for (int i = 1; i <= 3; i++) {
        char buf[2];
        printf("Caractere %d > ", i);
        read(buf, 2);
        printf("  ASCII = %d (caractere : %c)\n",
               (int) (unsigned char) buf[0],
               buf[0]);
    }
}

// --- Etape 7 : appel système read. Lit une ligne et la réaffiche.
static void demo_read(void) {
    printf("\n--- Etape 7 : Appel systeme read ---\n");
    printf("Tapez une phrase courte puis Entree :\n");
    printf("> ");

    char buf[64];
    int n = read(buf, 64);
    printf("Vous avez tape %d caracteres : \"%s\"\n", n, buf);
}


// --- Etape 8 : Pagination. Vérifie que CR0.PG est à 1 et affiche
// l'état du bitmap mémoire.
static void demo_paging(void) {
    printf("\n--- Etape 8 : Pagination ---\n");

    // Lit CR0 et regarde le bit 31 (PG = paging enabled).
    uint32_t cr0;
    __asm__ __volatile__("mov %%cr0, %0" : "=r" (cr0));

    if (cr0 & 0x80000000) {
        printf("CR0.PG = 1 -> pagination active.\n");
    } else {
        printf("CR0.PG = 0 -> pagination inactive (anormal).\n");
    }

    // Lit CR3 (adresse du répertoire de pages).
    uint32_t cr3;
    __asm__ __volatile__("mov %%cr3, %0" : "=r" (cr3));
    printf("CR3 (repertoire de pages) = 0x%x\n", cr3);

    // Etat du bitmap mémoire physique.
    print_mem();
}


static void print_menu(void) {
    printf("\n=== Demo des etapes du projet n7OS ===\n");
    printf("  1. Console (caracteres de controle)\n");
    printf("  2. Interruption logicielle (int 50)\n");
    printf("  3. Appel systeme example\n");
    printf("  4. Timer (deja visible en haut a droite)\n");
    printf("  5. Processus et ordonnancement\n");
    printf("  6. Clavier\n");
    printf("  7. Appel systeme read\n");
    printf("  8. Pagination\n");
    printf("  q. Quitter le mode demo\n");
}

void demo(void) {
    char choice[DEMO_LINE_MAX];

    while (1) {
        print_menu();
        printf("Choix > ");
        int n = read(choice, DEMO_LINE_MAX);

        if (n == 0) {
            continue;
        }

        switch (choice[0]) {
            case '1': demo_console();   break;
            case '2': demo_interrupt(); break;
            case '3': demo_syscall();   break;
            case '4': demo_timer();     break;
            case '5': demo_process();   break;
            case '6': demo_keyboard();  break;
            case '7': demo_read();      break;
            case '8': demo_paging();    break;
            case 'q': case 'Q':
                printf("Sortie du mode demo.\n");
                return;
            default:
                printf("Choix invalide : %c\n", choice[0]);
                break;
        }
    }
}