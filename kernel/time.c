#include <n7OS/time.h>
#include <n7OS/cpu.h>
#include <n7OS/irq.h>
#include <n7OS/console.h>
#include <n7OS/process.h>
#include <inttypes.h>

// Quantum d'exécution en ticks (= ms). Au-delà, on rend la main au
// processus suivant.
#define TIME_QUANTUM_MS 50

// Fréquence de l'oscillateur du PIT 8253/8254. Le diviseur à
// programmer est PIT_OSCILLATOR_HZ / TIMER_FREQ_HZ.
#define PIT_OSCILLATOR_HZ 1193182

// Ports d'E/S du PIT
#define PIT_CHANNEL_0   0x40
#define PIT_COMMAND     0x43

/*
 * Octet de commande pour le canal 0 :
 *   bits 7-6 = 00  -> canal 0
 *   bits 5-4 = 11  -> accès LSB puis MSB
 *   bits 3-1 = 010 -> mode 2 (générateur d'impulsions périodique)
 *   bit  0   = 0   -> binaire (pas BCD)
 */
#define PIT_CMD_CHANNEL_0_RATE 0x34

// Ports du PIC maître. L'IRQ 0 (timer) y est cablée.
#define PIC_MASTER_CMD  0x20
#define PIC_MASTER_DATA 0x21
#define PIC_EOI         0x20

// IRQ 0 -> entrée 0x20 de l'IDT.
#define IDT_TIMER_VECTOR 0x20

// Compteur de ticks (1 tick = 1 ms). volatile car partagé entre
// l'ISR et le code normal.
static volatile uint32_t system_ticks = 0;

// Stub asm défini dans handler_timer.S.
extern void handler_timer(void);

void init_timer(void) {
    uint16_t divider = (uint16_t)(PIT_OSCILLATOR_HZ / TIMER_FREQ_HZ);

    // Configuration du canal 0 du PIT
    outb(PIT_CMD_CHANNEL_0_RATE, PIT_COMMAND);
    outb((uint8_t)(divider & 0xFF), PIT_CHANNEL_0);
    outb((uint8_t)((divider >> 8) & 0xFF), PIT_CHANNEL_0);

    // Installation du traitant dans l'IDT
    init_irq_entry(IDT_TIMER_VECTOR, (uint32_t) handler_timer);

    // Démasquage de l'IRQ 0 dans le PIC (bit 0 à 0, autres préservés).
    outb(inb(PIC_MASTER_DATA) & 0xFE, PIC_MASTER_DATA);
}

// Décompose un nombre de ms en h:m:s (modulo 24h).
static void convert_ticks(uint32_t ticks, uint8_t *h, uint8_t *m, uint8_t *s) {
    uint32_t total_seconds = ticks / TIMER_FREQ_HZ;
    *s = (uint8_t)(total_seconds % 60);
    *m = (uint8_t)((total_seconds / 60) % 60);
    *h = (uint8_t)((total_seconds / 3600) % 24);
}

// Ecrit deux chiffres ASCII (dizaine puis unite) avec l'attribut
// de couleur passé en argument.
static void print_two_digits(uint16_t *dest, uint8_t value, uint8_t color) {
    dest[0] = ((uint16_t)color << 8) | (uint8_t)('0' + (value / 10));
    dest[1] = ((uint16_t)color << 8) | (uint8_t)('0' + (value % 10));
}

/*
 * Affichage de l'heure HH:MM:SS en haut à droite (ligne 0, colonnes
 * 72-79). On écrit directement en mémoire vidéo pour ne pas déplacer
 * le curseur courant.
 */
static void display_clock(void) {
    uint8_t h, m, s;
    convert_ticks(system_ticks, &h, &m, &s);

    uint16_t *vga = (uint16_t *) 0xB8000;
    uint16_t *clock_pos = vga + (0 * 80 + 72);

    const uint8_t color = 0x0E;  // jaune sur noir

    print_two_digits(clock_pos + 0, h, color);
    clock_pos[2] = ((uint16_t)color << 8) | ':';
    print_two_digits(clock_pos + 3, m, color);
    clock_pos[5] = ((uint16_t)color << 8) | ':';
    print_two_digits(clock_pos + 6, s, color);
}

/*
 * Traitant C de l'IRQ 0. EOI en tête (sinon plus aucune IRQ ne
 * passe), incrément du compteur, mise à jour de l'horloge, et tous
 * les TIME_QUANTUM_MS ticks on déclenche schedule() pour la
 * préemption.
 *
 * L'EOI doit être fait avant un éventuel ctx_sw, sinon le PIC reste
 * bloqué côté nouveau processus.
 */
void tick_handler_C(void) {
    outb(PIC_EOI, PIC_MASTER_CMD);
    system_ticks++;
    display_clock();

    if ((system_ticks % TIME_QUANTUM_MS) == 0) {
        schedule();
    }
}

uint32_t get_ticks(void) {
    return system_ticks;
}