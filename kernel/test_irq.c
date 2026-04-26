#include <n7OS/irq.h>
#include <n7OS/cpu.h>
#include <stdio.h>

// Stub asm défini dans handler_IT.S.
extern void handler_IT();

// Installe le traitant de test sur la ligne 50 de l'IDT.
void init_irq() {
    init_idt_entry(50, (uint32_t) handler_IT);
}

// Appelé depuis handler_IT pour confirmer la reception de l'IT.
void handler_en_C() {
    printf("Interruption recue !\n");
}