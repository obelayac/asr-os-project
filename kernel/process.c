#include <n7OS/process.h>
#include <n7OS/cpu.h>
#include <inttypes.h>
#include <string.h>

// Table des processus, allouée en .bss (donc remplie de zéros au
// démarrage, ce qui correspond bien à LIBRE pour toutes les entrées).
process_t process_table[NB_PROC];

// Une pile par slot, allouée statiquement pour eviter d'avoir besoin
// d'un allocateur. Le slot 0 (idle) ne s'en sert pas : idle reste
// sur la pile de boot mise en place par crt0.S.
static uint32_t stacks[NB_PROC][STACK_SIZE];

// pid du processus actuellement ELU.
static pid_t current_pid = 0;

// Tant que ce drapeau vaut 0, schedule() ne fait rien. Sert à bloquer
// les appels du timer pendant le bootstrap (avant que la table soit
// prête).
static volatile int scheduling_enabled = 0;

// Adresse de la "vraie" fonction de chaque processus, lue par le
// trampoline process_starter au lancement.
static void (*process_entry[NB_PROC])(void);

void init_process(void) {
    for (int i = 0; i < NB_PROC; i++) {
        process_table[i].pid   = (pid_t) i;
        process_table[i].state = LIBRE;
        process_table[i].name[0] = '\0';
        for (int r = 0; r < 5; r++) {
            process_table[i].regs[r] = 0;
        }
    }
}

// Trampoline executé par tout nouveau processus avant son code.
// ctx_sw bascule via "ret" et non "iret", donc IF n'est pas restauré ;
// si on arrivait ici depuis une ISR, on aurait IF=0 et le timer ne
// pourrait plus jamais nous préempter. Le sti() règle ça.
static void process_starter(void) {
    pid_t me = getpid();
    sti();
    process_entry[me]();
    // Si la fonction retourne (ne devrait pas arriver), on s'arrête.
    while (1) {
        hlt();
    }
}

static int find_free_slot(void) {
    for (int i = 0; i < NB_PROC; i++) {
        if (process_table[i].state == LIBRE) {
            return i;
        }
    }
    return -1;
}

/*
 * Création d'un processus. On prépare sa pile de manière à ce que
 * le premier "ret" de ctx_sw saute sur process_starter, qui se
 * chargera ensuite d'appeler la vraie fonction.
 */
pid_t create_process(const char *name, void (*function)(void)) {
    int slot = find_free_slot();
    if (slot < 0) {
        return (pid_t) -1;
    }

    process_t *p = &process_table[slot];

    p->pid   = (pid_t) slot;
    p->state = PRET;

    // Copie du nom, bornée à 31 caractères + '\0'.
    int i;
    for (i = 0; i < 31 && name[i] != '\0'; i++) {
        p->name[i] = name[i];
    }
    p->name[i] = '\0';

    process_entry[slot] = function;

    // Sommet de pile = adresse du trampoline. Le ret de ctx_sw va
    // dépiler ça dans EIP au premier basculement vers ce processus.
    uint32_t *stack_top = &stacks[slot][STACK_SIZE - 1];
    *stack_top = (uint32_t) process_starter;

    // regs[] : tout à 0 sauf esp (regs[1]) qui pointe sur le sommet
    // de pile préparé ci-dessus.
    p->regs[0] = 0;                    // ebx
    p->regs[1] = (uint32_t) stack_top; // esp
    p->regs[2] = 0;                    // ebp
    p->regs[3] = 0;                    // esi
    p->regs[4] = 0;                    // edi

    return p->pid;
}

/*
 * Round-robin sur la table : on repart du slot suivant le courant
 * et on prend le premier PRET qu'on trouve. Si rien d'autre n'est
 * pret on reste sur le processus courant.
 */
void schedule(void) {
    if (!scheduling_enabled) {
        return;
    }

    pid_t old_pid = current_pid;
    pid_t next    = old_pid;

    for (int i = 1; i <= NB_PROC; i++) {
        pid_t candidate = (old_pid + i) % NB_PROC;
        if (process_table[candidate].state == PRET) {
            next = candidate;
            break;
        }
    }

    if (next == old_pid) {
        return;
    }

    // L'ancien repasse PRET (sauf s'il était BLOQUE), le nouveau
    // devient ELU.
    if (process_table[old_pid].state == ELU) {
        process_table[old_pid].state = PRET;
    }
    process_table[next].state = ELU;
    current_pid = next;

    ctx_sw(process_table[old_pid].regs, process_table[next].regs);
}

pid_t getpid(void) {
    return current_pid;
}

// A appeler une fois la table des processus prête, juste avant de
// passer la main au premier processus.
void start_scheduling(void) {
    scheduling_enabled = 1;
}

/*
 * Processus idle (pid 0). Au premier appel il déclenche schedule()
 * pour donner la main aux autres. Quand il reprend la main, il se
 * contente d'attendre les interruptions.
 *
 * Le sti() avant le hlt() est important : si idle revient ici depuis
 * un ctx_sw fait à l'intérieur d'une ISR, IF peut être resté à 0,
 * et un hlt() avec IF=0 bloque indéfiniment.
 */
void idle(void) {
    schedule();

    while (1) {
        sti();
        hlt();
    }
}