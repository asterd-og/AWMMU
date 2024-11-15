#include "mmu.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

void mmu_set_page_map(mmu_t *mmu, uint32_t *page_map) {
    mmu->page_map = page_map;
}

uint32_t *mmu_traverse_expect(uint32_t *level, uint32_t entry, uint8_t flags) {
    if ((level[entry] & flags) != flags) return NULL;
    return (uint32_t*)(level[entry] & ~(uint8_t)0xff);
}

void mmu_write(mmu_t *mmu, uint32_t virtual_address, uint8_t data) {
    uint32_t page_table_index = (virtual_address >> 25) & 0x7f;
    uint32_t page_directory_index = (virtual_address >> 18) & 0x7f;
    uint32_t page_entry_index = (virtual_address >> 11) & 0x7f;
    uint32_t page_offset = (virtual_address) & 0x7FF;

    uint32_t *page_table = mmu_traverse_expect(mmu->page_map,
        page_table_index, FLAG_READ | FLAG_WRITE);
    if (!page_table) {
        mmu->flags |= MMU_PF;
        return;
    }
    uint32_t *page_directory = mmu_traverse_expect(page_table,
        page_directory_index, FLAG_READ | FLAG_WRITE);
    if (!page_directory) {
        mmu->flags |= MMU_PF;
        return;
    }
    uint32_t *page_entry = mmu_traverse_expect(page_directory,
        page_entry_index, FLAG_READ | FLAG_WRITE);
    if (!page_entry) {
        mmu->flags |= MMU_PF;
        return;
    }
    *((uint32_t*)((uint8_t*)page_entry + page_offset)) = data;
}

uint8_t mmu_read(mmu_t *mmu, uint32_t virtual_address) {
    uint32_t page_table_index = (virtual_address >> 25) & 0x7f;
    uint32_t page_directory_index = (virtual_address >> 18) & 0x7f;
    uint32_t page_entry_index = (virtual_address >> 11) & 0x7f;
    uint32_t page_offset = (virtual_address) & 0x7FF;

    uint32_t *page_table = mmu_traverse_expect(mmu->page_map,
        page_table_index, FLAG_READ | FLAG_WRITE);
    if (!page_table) {
        mmu->flags |= MMU_PF;
        return;
    }
    uint32_t *page_directory = mmu_traverse_expect(page_table,
        page_directory_index, FLAG_READ | FLAG_WRITE);
    if (!page_directory) {
        mmu->flags |= MMU_PF;
        return;
    }
    uint32_t *page_entry = mmu_traverse_expect(page_directory,
        page_entry_index, FLAG_READ | FLAG_WRITE);
    if (!page_entry) {
        mmu->flags |= MMU_PF;
        return;
    }
    return *((uint8_t*)((uint8_t*)page_entry + page_offset));
}

void mmu_destroy(mmu_t *mmu) {
    for (int page_table_idx = 0; page_table_idx < 128; page_table_idx++) {
        uint32_t *page_table = mmu_traverse_expect(mmu->page_map, page_table_idx,
            FLAG_READ);
        if (!page_table) continue;
        for (int page_directory_idx = 0; page_directory_idx < 128; page_directory_idx++) {
            uint32_t *page_directory = mmu_traverse_expect(page_table, page_directory_idx,
                FLAG_READ);
            if (!page_directory) continue;
            for (int page_entry_idx = 0; page_entry_idx < 128; page_entry_idx++) {
                uint32_t *page_entry = mmu_traverse_expect(page_directory, page_entry_idx,
                    FLAG_READ);
                if (!page_entry) continue;
                free(*page_entry);
                free(page_entry);
            }
            free(page_directory);
        }
        free(page_table);
    }
    free(mmu->page_map);
    free(mmu);
}