#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mmu.h"

uint32_t *traverse_level(uint32_t *level, uint32_t entry, uint8_t flags) {
    if (level[entry] & FLAG_READ) return GET_LEVEL_ADDR(level[entry]);
    uint32_t *new_level = (uint32_t*)aligned_alloc(1024, 512);
    level[entry] = (uint32_t)new_level | flags;
    memset(new_level, 0, 512);
    return (uint32_t)new_level;
}

void map(uint32_t *page_map, uint32_t virtual_address, uint32_t physical_address, uint8_t flags) {
    uint32_t page_table_index = (virtual_address >> 25) & 0x7f;
    uint32_t page_directory_index = (virtual_address >> 18) & 0x7f;
    uint32_t page_entry_index = (virtual_address >> 11) & 0x7f;
    
    uint32_t *page_table = traverse_level(page_map, page_table_index, flags);
    uint32_t *page_directory = traverse_level(page_table, page_directory_index, flags);

    page_directory[page_entry_index] = physical_address | flags;
}

uint32_t *page_alloc() {
    return (uint32_t*)aligned_alloc(PAGE_SIZE, PAGE_SIZE);
}

void mem_copy(mmu_t *mmu, uint32_t *dest, const void *source, size_t n) {
    const uint8_t *src = (uint8_t*)source;
    for (size_t i = 0; i < n; i++) {
        mmu_write(mmu, (uint32_t)dest + i, src[i]);
        if (mmu->flags & MMU_PF)
            return;
    }
}

int main() {
    // 3 Page map level

    // 7 bits for PML3 128 entries * 4 (sizeof uint32_t) = 512 bytes
    // 7 bits for PML2 128 entries * 4 (sizeof uint32_t) = 512 bytes
    // 7 bits for PML1 128 entries * 4 (sizeof uint32_t) = 512 bytes
    // 11 bits for offset (1024 bytes page size)
    // Yes, this means we waste 512 bytes if we allocate a page for a page map level

    mmu_t *mmu = (mmu_t*)malloc(sizeof(mmu_t));
    memset(mmu, 0, sizeof(mmu_t));

    uint32_t *page_map = (uint32_t*)aligned_alloc(PAGE_SIZE, 512);
    memset(page_map, 0, 512);
    mmu_set_page_map(mmu, page_map);

    // Allocate a physical page
    uint32_t *page = page_alloc();
    map(mmu->page_map, 0xabcdef12, (uint32_t)page, FLAG_READ | FLAG_WRITE);
    printf("Mapped (virt) 0x%x to (phys) 0x%x\n", 0xabcdef12, (uint32_t)page);
    mem_copy(mmu, (uint32_t*)0xabcdef12, "Hello virtual world!", 21);
    if (mmu->flags & MMU_PF) {
        printf("Page fault while writing.\n");
        goto cleanup;
    }
    char string[21];
    for (int i = 0; i < 21; i++) {
        string[i] = mmu_read(mmu, 0xabcdef12 + i);
        if (mmu->flags & MMU_PF) {
            printf("Page fault while reading.\n");
            goto cleanup;
        }
    }
    printf("0xabcdef12: %s\n", string);

cleanup:
    mmu_destroy(mmu);
    return 0;
}