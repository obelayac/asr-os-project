#include <n7OS/paging.h>
#include <n7OS/mem.h>
#include <n7OS/kheap.h>
#include <stddef.h>

// Le répertoire de pages du noyau et son adresse physique.
static PageDirectory *kernel_directory;

void initialise_paging() {
    // 1. Allocation du répertoire de pages, aligné sur une page.
    //    kmalloc_a est utilisable car on n'a pas encore activé la
    //    pagination : les adresses retournées sont des adresses
    //    physiques utilisables directement.
    kernel_directory = (PageDirectory *) kmalloc_a(sizeof(PageDirectory));

// 2. Initialisation : toutes les entrées à zéro (P=0).
    for (int i = 0; i < PAGE_DIRECTORY_ENTRIES; i++) {
        kernel_directory->entries[i].value = 0;
    }

    // 3. Identity-mapping des 16 premiers Mo de mémoire physique.
    //    Une entrée de répertoire couvre 4 Mo, on en alloue donc 4.
    //    Chaque table mappe son indice virtuel sur le même indice
    //    physique : virt == phys, ce qui permet au code, aux piles
    //    et aux structures déjà en place de continuer à fonctionner
    //    sans déplacement après l'activation de la pagination.
    for (int dir = 0; dir < 4; dir++) {
        PageTable table = (PageTable) kmalloc_a(
            sizeof(PTE) * PAGE_TABLE_ENTRIES);

        for (int i = 0; i < PAGE_TABLE_ENTRIES; i++) {
            uint32_t phys = (dir * PAGE_TABLE_ENTRIES + i) * PAGE_SIZE;
            table[i].value = 0;
            table[i].page_entry.present    = 1;
            table[i].page_entry.rw         = 1;
            table[i].page_entry.user       = 0;  // mode noyau
            table[i].page_entry.frame_addr = phys >> 12;
        }

        // L'entrée du répertoire pointe sur la table avec les flags
        // P=1, RW=1 (et U=0 pour mode noyau).
        kernel_directory->entries[dir].value = 0;
        kernel_directory->entries[dir].page_entry.present    = 1;
        kernel_directory->entries[dir].page_entry.rw         = 1;
        kernel_directory->entries[dir].page_entry.user       = 0;
        kernel_directory->entries[dir].page_entry.frame_addr = ((uint32_t) table) >> 12;
    }

    // 4. Marquage des pages utilisées par le noyau dans le bitmap.
    //    kmalloc_a a avancé placement_address ; tout ce qui est
    //    avant lui (code noyau + tables qu'on vient d'allouer) est
    //    déjà occupé et ne doit pas être ré-alloué par findfreePage.
    extern uint32_t placement_address;
    for (uint32_t addr = 0; addr < placement_address; addr += PAGE_SIZE) {
        setPage(addr);
    }

    // 5. Chargement de l'adresse du répertoire dans CR3.
    __asm__ __volatile__("mov %0, %%cr3" :: "r" (kernel_directory));

    // 6. Activation de la pagination en mettant à 1 le bit 31 (PG)
    //    de CR0.
    uint32_t cr0;
    __asm__ __volatile__("mov %%cr0, %0" : "=r" (cr0));
    cr0 |= 0x80000000;
    __asm__ __volatile__("mov %0, %%cr0" :: "r" (cr0));
}

PageTable alloc_page_entry(uint32_t address, int is_writeable, int is_kernel) {
    // Découpage de l'adresse virtuelle :
    //   bits 22-31 = indice dans le répertoire (10 bits)
    //   bits 12-21 = indice dans la table de pages (10 bits)
    //   bits 0-11  = offset dans la page (ignoré ici)
    uint32_t dir_idx = (address >> 22) & 0x3FF;
    uint32_t tab_idx = (address >> 12) & 0x3FF;

    // Si la table n'existe pas encore (P=0), on la crée.
    if (kernel_directory->entries[dir_idx].page_entry.present == 0) {
        PageTable new_table =
            (PageTable) kmalloc_a(sizeof(PTE) * PAGE_TABLE_ENTRIES);
        for (int i = 0; i < PAGE_TABLE_ENTRIES; i++) {
            new_table[i].value = 0;
        }
        kernel_directory->entries[dir_idx].value = 0;
        kernel_directory->entries[dir_idx].page_entry.present    = 1;
        kernel_directory->entries[dir_idx].page_entry.rw         = 1;
        kernel_directory->entries[dir_idx].page_entry.user       = 0;
        kernel_directory->entries[dir_idx].page_entry.frame_addr = ((uint32_t) new_table) >> 12;
    }

    PageTable table =
        (PageTable) (kernel_directory->entries[dir_idx].page_entry.frame_addr << 12);

    // Allocation d'une page physique libre via le bitmap.
    uint32_t phys = findfreePage();

    table[tab_idx].value = 0;
    table[tab_idx].page_entry.present    = 1;
    table[tab_idx].page_entry.rw         = is_writeable ? 1 : 0;
    table[tab_idx].page_entry.user       = is_kernel    ? 0 : 1;
    table[tab_idx].page_entry.frame_addr = phys >> 12;

    return table;
}