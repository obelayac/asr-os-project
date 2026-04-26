#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Défini dans bin/demo.c.
extern void demo(void);

#define LINE_MAX 64

// Retourne 1 si s commence par prefix suivi de la fin de chaîne ou
// d'un espace. Evite de matcher "help123" comme "help".
static int str_starts_with(const char *s, const char *prefix) {
    int i = 0;
    while (prefix[i] != '\0') {
        if (s[i] != prefix[i]) {
            return 0;
        }
        i++;
    }
    return s[i] == '\0' || s[i] == ' ';
}

static void cmd_help(void) {
    printf("Commandes disponibles :\n");
    printf("  help     liste des commandes\n");
    printf("  clear    efface l'ecran\n");
    printf("  time     etat du systeme\n");
    printf("  echo X   reaffiche X\n");
    printf("  demo     menu de demonstration des etapes\n");
}

static void cmd_clear(void) {
    printf("\f");
}

static void cmd_time(void) {
    printf("Voir l'horloge en haut a droite (HH:MM:SS).\n");
}

static void cmd_echo(const char *arg) {
    printf("%s\n", arg);
}

/*
 * Shell minimal : prompt -> lecture ligne -> dispatch sur le premier
 * mot. Commandes : help, clear, time, echo X, demo. Toute autre
 * saisie -> message d'erreur.
 */
void shell(void) {
    char line[LINE_MAX];

    printf("Bienvenue dans n7sh. Tapez 'help' pour la liste des commandes.\n");

    while (1) {
        printf("n7OS> ");
        int n = read(line, LINE_MAX);

        if (n == 0) {
            continue;
        }

        if (str_starts_with(line, "help")) {
            cmd_help();
        } else if (str_starts_with(line, "clear")) {
            cmd_clear();
        } else if (str_starts_with(line, "time")) {
            cmd_time();
        } else if (str_starts_with(line, "demo")) {
            demo();
        } else if (str_starts_with(line, "echo")) {
            // On saute "echo" + l'espace eventuel.
            const char *arg = line + 4;
            if (*arg == ' ') {
                arg++;
            }
            cmd_echo(arg);
        } else {
            printf("Commande inconnue : %s\n", line);
            printf("Tapez 'help' pour la liste.\n");
        }
    }
}