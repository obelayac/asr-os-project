#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <inttypes.h>

// Nombre maximum de processus simultanés.
#define NB_PROC 16

// Taille de pile par processus, en mots de 32 bits (256 mots = 1 Ko).
#define STACK_SIZE 256

typedef uint32_t pid_t;

/*
 * Etats d'un processus :
 *   LIBRE  : emplacement non utilisé dans la table
 *   PRET   : prêt à être exécuté
 *   ELU    : en cours d'execution
 *   BLOQUE : en attente (non utilisé pour le moment)
 */
typedef enum {
    LIBRE  = 0,
    PRET,
    ELU,
    BLOQUE
} process_state_t;

// Descripteur de processus. regs[] = ebx, esp, ebp, esi, edi (dans
// cet ordre), sauvés/restaurés par ctx_sw.
typedef struct {
    pid_t            pid;
    process_state_t  state;
    char             name[32];
    uint32_t         regs[5];
} process_t;

extern process_t process_table[NB_PROC];

void init_process(void);

// Crée un processus dont le code est `function`. Retourne le pid
// attribué, ou -1 si la table est pleine.
pid_t create_process(const char *name, void (*function)(void));

// Choisit le prochain processus PRET (round-robin) et bascule.
void schedule(void);

pid_t getpid(void);

// Processus de pid 0. Lance l'ordonnancement initial puis attend
// les interruptions sur sti+hlt.
void idle(void);

// A appeler une fois la table prête, avant le premier processus.
// Tant que ce n'est pas fait, schedule() est un no-op.
void start_scheduling(void);

// Fonction asm (kernel/ctx_sw.S). Sauve ebx/esp/ebp/esi/edi dans
// ctx_old, restaure depuis ctx_new ; son "ret" final saute dans
// le code du nouveau processus.
extern void ctx_sw(uint32_t *ctx_old, uint32_t *ctx_new);

#endif