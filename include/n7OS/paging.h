/**
 * @file paging.h
 * @brief Gestion de la pagination dans le noyau
 */
#ifndef _PAGING_H
#define _PAGING_H

#include <inttypes.h>

// Description d'une ligne de table de page (cf. paper, slide 5).
typedef struct {
    uint32_t present    : 1;   // P
    uint32_t rw         : 1;   // W : 1 = lecture/ecriture, 0 = lecture seule
    uint32_t user       : 1;   // U : 1 = utilisateur, 0 = noyau
    uint32_t reserved1  : 2;   // RSVD
    uint32_t accessed   : 1;   // A
    uint32_t dirty      : 1;   // D
    uint32_t reserved2  : 2;   // RSVD
    uint32_t avail      : 3;   // AVAIL
    uint32_t frame_addr : 20;  // adresse physique de la page (20 bits forts)
} page_table_entry_t;

// Une entrée peut être manipulée par champ ou par valeur brute.
typedef union {
    page_table_entry_t page_entry;
    uint32_t value;
} PTE;

// Une table de page = tableau de descripteurs de page.
typedef PTE *PageTable;

// 1024 entrées par table et par répertoire.
#define PAGE_DIRECTORY_ENTRIES 1024
#define PAGE_TABLE_ENTRIES     1024

// Une entrée de répertoire de pages (mêmes champs qu'une PTE,
// le frame_addr pointe sur une PageTable au lieu d'une page de
// données).
typedef PTE PDE;

// Description d'un répertoire de pages.
typedef struct {
    PDE entries[PAGE_DIRECTORY_ENTRIES];
} PageDirectory;

void initialise_paging();
PageTable alloc_page_entry(uint32_t address, int is_writeable, int is_kernel);

#endif