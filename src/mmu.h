#pragma once

#include <stdint.h>
#include <stddef.h>

#define FLAG_READ 1
#define FLAG_WRITE 2
#define FLAG_USER 4

#define MMU_PF 1

#define PAGE_SIZE 1024

#define GET_LEVEL_ADDR(x) ((uint32_t*)((uint32_t)x & 0xffffff00))

typedef struct {
    uint32_t *page_map;
    uint32_t flags;
} mmu_t;

void mmu_set_page_map(mmu_t *mmu, uint32_t *page_map);
void mmu_map(mmu_t *mmu, uint32_t virtual_address, uint32_t physical_address, uint8_t flags);
uint32_t mmu_get_page(mmu_t *mmu, uint32_t virtual_address);

uint32_t *mmu_traverse_expect(uint32_t *level, uint32_t entry, uint8_t flags);

void mmu_write(mmu_t *mmu, uint32_t virtual_address, uint8_t data);
uint8_t mmu_read(mmu_t *mmu, uint32_t virtual_address);

void mmu_destroy(mmu_t *mmu);