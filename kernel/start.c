#include <n7OS/cpu.h>
#include <n7OS/processor_structs.h>
#include <n7OS/console.h>
#include <n7OS/irq.h>
#include <n7OS/sys.h>
#include <n7OS/time.h>
#include <n7OS/process.h>
#include <n7OS/keyboard.h>
#include <stdio.h>

// Définis dans bin/ (linkés via app.o).
extern void processus1(void);
extern void shell(void);

/*
 * Point d'entrée du noyau (appelé depuis crt0.S). Initialise les
 * sous-systèmes, crée les processus de base, puis passe la main
 * à idle. Les tests des différentes étapes sont accessibles depuis
 * le shell via la commande "demo".
 */
void kernel_start(void)
{
    // Console : mémoire vidéo + curseur. En premier pour que les
    // printf ultérieurs marchent.
    init_console();

    // Tables système : GDT, IDT, TSS, PIC. Aucune IT n'est encore
    // active à ce stade.
    setup_base(0);

    // Appels système : enregistrement dans la syscall_table et
    // installation du handler sur IDT[0x80] (DPL=3). A faire avant
    // le premier printf qui passe par write -> int $0x80.
    init_syscall();

    // Handler de test sur IDT[50] (utilisé par demo -> étape 2).
    init_irq();

    // PIT à 1 kHz + handler sur IDT[0x20] + démasquage IRQ 0.
    init_timer();

    // Handler clavier sur IDT[0x21] + démasquage IRQ 1.
    init_keyboard();

    // Activation des IT matérielles. Le timer et le clavier sont
    // maintenant vivants.
    sti();

    printf("n7OS demarre.\n");

    // Création des processus de base.
    init_process();
    create_process("idle",       idle);
    create_process("processus1", processus1);
    create_process("shell",      shell);

    // On se considère comme idle (pid 0) : la pile actuelle (mise
    // en place par crt0.S) devient sa pile.
    process_table[0].state = ELU;

    // A partir d'ici, schedule() peut effectivement basculer.
    start_scheduling();

    // Saut dans idle. Ne retourne jamais.
    idle();

    // Garde-fou : inateignable.
    while (1) {
        hlt();
    }
}