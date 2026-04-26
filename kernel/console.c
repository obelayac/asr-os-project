#include <n7OS/console.h>
#include <n7OS/cpu.h>

// Mémoire vidéo VGA texte. Chaque case = uint16_t : octet faible
// ASCII, octet fort attributs (clignotement | fond | texte).
uint16_t *scr_tab;

// Position courante du curseur (mise à jour à chaque caractère et
// synchronisée avec la carte via set_cursor).
static uint8_t cursor_line;
static uint8_t cursor_col;

/*
 * Indique au matériel la nouvelle position du curseur via les
 * ports 0x3D4 (commande) et 0x3D5 (donnée). On envoie la valeur
 * 80*line+col en deux moitiés (LSB puis MSB), chacune précédée
 * de sa commande.
 */
static void set_cursor(uint8_t line, uint8_t col) {
    uint16_t pos = (uint16_t)(VGA_WIDTH * line + col);

    outb(CMD_LOW, PORT_CMD);
    outb((uint8_t)(pos & 0xFF), PORT_DATA);

    outb(CMD_HIGH, PORT_CMD);
    outb((uint8_t)((pos >> 8) & 0xFF), PORT_DATA);
}

// Remplit l'écran d'espaces et remet le curseur en (0,0). Utilisé
// à l'initialisation et pour le caractère '\f'.
static void clear_screen(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        scr_tab[i] = (CHAR_COLOR << 8) | ' ';
    }
    cursor_line = 0;
    cursor_col  = 0;
    set_cursor(cursor_line, cursor_col);
}

// Efface la ligne donnée. Appelé à chaque saut de ligne pour
// éviter que le nouveau contenu se mélange avec les restes du
// tour précédent (on n'a pas de vrai scrolling).
static void clear_line(uint8_t line) {
    int start = VGA_WIDTH * line;
    for (int i = 0; i < VGA_WIDTH; i++) {
        scr_tab[start + i] = (CHAR_COLOR << 8) | ' ';
    }
}

void init_console() {
    scr_tab = (uint16_t *) SCREEN_ADDR;
    clear_screen();
}

/*
 * Affiche un caractère à la position courante, ou interprète un
 * caractère de contrôle (\b \t \n \f \r). Caractères non
 * imprimables et non reconnus ignorés. Wrap automatique en bas
 * de colonne et en bas d'écran (retour ligne 0, pas de scroll).
 */
void console_putchar(const char c) {
    if (c > 31 && c < 127) {
        // Caractère imprimable
        int pos = VGA_WIDTH * cursor_line + cursor_col;
        scr_tab[pos] = (CHAR_COLOR << 8) | (uint8_t)c;
        cursor_col++;
    } else if (c == '\b') {
        // Backspace : recule sans franchir la ligne précédente
        if (cursor_col > 0) {
            cursor_col--;
        }
    } else if (c == '\t') {
        cursor_col += 8;
    } else if (c == '\n') {
        cursor_line++;
        cursor_col = 0;
    } else if (c == '\f') {
        clear_screen();
        return; // clear_screen repositionne déjà le curseur
    } else if (c == '\r') {
        cursor_col = 0;
    }
    // Tout autre caractère est ignoré.

    // Wrap colonne -> ligne suivante.
    if (cursor_col >= VGA_WIDTH) {
        cursor_col = 0;
        cursor_line++;
    }

    // Wrap écran -> retour en haut.
    if (cursor_line >= VGA_HEIGHT) {
        cursor_line = 0;
    }

    // Si on a changé de ligne, on l'efface d'abord pour ne pas
    // mélanger avec les restes du tour précédent. La zone de
    // l'horloge (ligne 0, col 72-79) est rafraîchie toutes les ms
    // par tick_handler_C, donc le bref clear est invisible.
    static uint8_t last_line = 0;
    if (cursor_line != last_line) {
        clear_line(cursor_line);
        last_line = cursor_line;
    }

    set_cursor(cursor_line, cursor_col);
}

// Fonction appelée par printf : affiche len caractères de s.
void console_putbytes(const char *s, int len) {
    for (int i = 0; i < len; i++) {
        console_putchar(s[i]);
    }
}