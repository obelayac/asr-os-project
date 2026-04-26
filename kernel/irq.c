#include <inttypes.h>
#include <stdio.h>
#include <n7OS/irq.h>
#include <n7OS/segment.h>
#include <n7OS/processor_structs.h>

// Interrupt gate 32 bits, niveau noyau : P=1, DPL=00, S=0, Type=1110
// -> 1000 1110 = 0x8E.
#define IDT_FLAG_INT_GATE_KERNEL 0x8E

/*
 * Configure l'entrée num_line de l'IDT comme interrupt gate 32 bits
 * pointant sur le traitant donné, accessible uniquement depuis le
 * noyau (DPL=0).
 *
 * idt est déclaré en unsigned long long ; on le re-vue comme tableau
 * d'idt_entry_t pour écrire les champs par nom plutôt qu'à coup de
 * shifts.
 */
void init_idt_entry(int num_line, uint32_t handler) {
    idt_entry_t *entry = ((idt_entry_t *) idt) + num_line;

    entry->offset_inf  = (uint16_t)(handler & 0xFFFF);
    entry->sel_segment = KERNEL_CS;
    entry->zero        = 0;
    entry->type_attr   = IDT_FLAG_INT_GATE_KERNEL;
    entry->offset_sup  = (uint16_t)((handler >> 16) & 0xFFFF);
}

// Même chose mais avec DPL=11 (ring 3) pour que du code utilisateur
// puisse emettre "int $0x80" sans declencher de #GP. Octet : 0xEE.
#define IDT_FLAG_INT_GATE_USER 0xEE

/*
 * Variante de init_idt_entry pour les vecteurs accessibles depuis
 * l'utilisateur : appels système (0x80) et IRQ matérielles (timer,
 * clavier) qui peuvent interrompre du code user.
 */
void init_irq_entry(int irq_num, uint32_t addr) {
    idt_entry_t *entry = ((idt_entry_t *) idt) + irq_num;

    entry->offset_inf  = (uint16_t)(addr & 0xFFFF);
    entry->sel_segment = KERNEL_CS;
    entry->zero        = 0;
    entry->type_attr   = IDT_FLAG_INT_GATE_USER;
    entry->offset_sup  = (uint16_t)((addr >> 16) & 0xFFFF);
}