#include <n7OS/mem.h>
#include <stdio.h>

// Nombre total de pages : 16 Mo / 4 Ko = 4096 pages.
#define NB_PAGES ((LAST_MEMORY_INDEX + 1) / PAGE_SIZE)

// Taille du bitmap en mots de 32 bits : 4096 / 32 = 128.
#define BITMAP_SIZE (NB_PAGES / 32)

// 1 bit par page. Bit à 1 = allouée, 0 = libre.
static uint32_t page_bitmap[BITMAP_SIZE];

void setPage(uint32_t addr) {
    uint32_t page = addr / PAGE_SIZE;
    page_bitmap[page / 32] |= (1u << (page % 32));
}

void clearPage(uint32_t addr) {
    uint32_t page = addr / PAGE_SIZE;
    page_bitmap[page / 32] &= ~(1u << (page % 32));
}

uint32_t findfreePage() {
    for (uint32_t page = 0; page < NB_PAGES; page++) {
        if ((page_bitmap[page / 32] & (1u << (page % 32))) == 0) {
            page_bitmap[page / 32] |= (1u << (page % 32));
            return page * PAGE_SIZE;
        }
    }
    return 0;  // plus de page libre
}

void init_mem() {
    for (int i = 0; i < BITMAP_SIZE; i++) {
        page_bitmap[i] = 0;
    }
}

void print_mem() {
    uint32_t used = 0;
    for (uint32_t p = 0; p < NB_PAGES; p++) {
        if (page_bitmap[p / 32] & (1u << (p % 32))) {
            used++;
        }
    }
    printf("Memoire physique : %d pages totales, %d utilisees, %d libres\n",
           (int) NB_PAGES, (int) used, (int) (NB_PAGES - used));
}