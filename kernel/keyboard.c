#include <n7OS/keyboard.h>
#include <n7OS/cpu.h>
#include <n7OS/irq.h>

// Stub asm défini dans handler_keyboard.S.
extern void handler_keyboard(void);

// IRQ 1 -> entrée 0x21 de l'IDT.
#define IDT_KEYBOARD_VECTOR 0x21

// Ports du PIC maître (déjà utilisés pour le timer).
#define PIC_MASTER_CMD  0x20
#define PIC_MASTER_DATA 0x21
#define PIC_EOI         0x20

// Buffer circulaire des caractères saisis. Taille puissance de 2
// pour transformer les modulos en simples masques binaires.
#define KBD_BUFFER_SIZE 64
#define KBD_BUFFER_MASK (KBD_BUFFER_SIZE - 1)

static volatile char kbd_buffer[KBD_BUFFER_SIZE];

// head : où l'ISR écrit. tail : où kgetch lit. Vide si head==tail,
// plein si (head+1)%SIZE == tail. volatile car partagés ISR / code.
static volatile int kbd_head = 0;
static volatile int kbd_tail = 0;

// Etat de la touche Shift, suivi explicitement (Shift n'est pas un
// caractère mais modifie l'interpretation du suivant).
static volatile int shift_pressed = 0;

// Scancodes du Shift droit (le gauche est dans keyboard.h).
#define RSHIFT_PRESSED  0x36
#define RSHIFT_RELEASED 0xB6

void init_keyboard() {
    init_irq_entry(IDT_KEYBOARD_VECTOR, (uint32_t) handler_keyboard);

    // Démasquage de l'IRQ 1 (bit 1 à 0, autres préservés).
    outb(inb(PIC_MASTER_DATA) & ~(1 << KEYB_PIC_IRQ), PIC_MASTER_DATA);
}

// Ajoute un caractère au buffer circulaire. Si le buffer est plein,
// le caractère est perdu (un ISR ne doit pas bloquer).
static void kbd_buffer_push(char c) {
    int next = (kbd_head + 1) & KBD_BUFFER_MASK;
    if (next != kbd_tail) {
        kbd_buffer[kbd_head] = c;
        kbd_head = next;
    }
}

/*
 * Traitant C de l'IRQ 1 : EOI, lecture du scancode au port 0x60,
 * suivi du Shift, conversion via la table adaptée, et push dans
 * le buffer si on obtient un caractère ASCII (<= 0xFF). Les
 * relâchements de touche (bit 7 à 1) et les codes étendus rares
 * sont ignorés.
 */
void keyboard_handler_C(void) {
    outb(PIC_EOI, PIC_MASTER_CMD);

    uint8_t scancode = inb(KEYB_ENCODER);

    // Touche Shift (pressée / relachée) : on met à jour le drapeau
    // mais on n'envoie rien dans le buffer.
    if (scancode == SHIFT_PRESSED || scancode == RSHIFT_PRESSED) {
        shift_pressed = 1;
        return;
    }
    if (scancode == SHIFT_RELEASED || scancode == RSHIFT_RELEASED) {
        shift_pressed = 0;
        return;
    }

    // On ne traite que les pressions, pas les relachements.
    if (IS_KEY_RELEASED(scancode)) {
        return;
    }

    // Garde-fou contre un éventuel scancode étendu hors table.
    if (scancode >= sizeof(scancode_map) / sizeof(scancode_map[0])) {
        return;
    }

    uint16_t key = shift_pressed ? scancode_map_shift[scancode]
                                 : scancode_map[scancode];

    // On ignore les touches non ASCII (F1-F12, flèches, etc.).
    if (key != 0 && key <= 0xFF) {
        kbd_buffer_push((char) key);
    }
}

/*
 * Renvoie le prochain caractère du buffer. Bloque tant que le
 * buffer est vide. sti+hlt = idiome x86 d'attente passive ; le
 * sti force IF=1 si on a été appelé depuis un contexte où IF=0.
 */
char kgetch() {
    while (kbd_head == kbd_tail) {
        sti();
        hlt();
    }

    char c = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) & KBD_BUFFER_MASK;
    return c;
}

// Version non bloquante : 1 si au moins un caractère en attente.
int kbd_has_input(void) {
    return kbd_head != kbd_tail;
}