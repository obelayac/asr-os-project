#ifndef __TIME_H__
#define __TIME_H__

#include <inttypes.h>

// Fréquence cible de l'horloge système. 1000 Hz = 1 tick / ms.
#define TIMER_FREQ_HZ 1000

// Programme le PIT à TIMER_FREQ_HZ et installe le traitant sur
// l'IRQ 0.
void init_timer(void);

// Appelé depuis handler_timer.S à chaque IRQ 0.
void tick_handler_C(void);

// Compteur de ticks (= ms) depuis le boot.
uint32_t get_ticks(void);

#endif