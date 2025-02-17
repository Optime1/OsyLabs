#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <math.h>

#define MIN_BLOCK_SIZE 16

typedef struct Allocator {
    void* memory;
    size_t size;
    void* free_lists[32];
} Allocator;

Allocator* allocator_create(void* memory, size_t size) {
    Allocator* allocator = (Allocator*)mmap(NULL, sizeof(Allocator), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    allocator->memory = memory;
    allocator->size = size;
    for (int i = 0; i < 32; i++) {
        allocator->free_lists[i] = NULL;
    }

    size_t block_size = 1 << (int)log2(size);
    *(void**)memory = NULL;
    allocator->free_lists[(int)log2(block_size)] = memory;

    return allocator;
}

void allocator_destroy(Allocator* allocator) {
    munmap(allocator, sizeof(Allocator));
}

void* allocator_alloc(Allocator* allocator, size_t size) {
    size_t required_size = size + sizeof(size_t);
    int index = (int)ceil(log2(required_size));

    if (index < log2(MIN_BLOCK_SIZE)) {
        index = (int)log2(MIN_BLOCK_SIZE);
    }

    while (index < 32 && !allocator->free_lists[index]) {
        index++;
    }

    if (index >= 32) {
        return NULL;
    }

    void* block = allocator->free_lists[index];
    allocator->free_lists[index] = *(void**)block;

    while (index > log2(required_size)) {
        index--;
        void* buddy = (char*)block + (1 << index);
        *(void**)buddy = allocator->free_lists[index];
        allocator->free_lists[index] = buddy;
    }

    *(size_t*)block = size;
    return (void*)((char*)block + sizeof(size_t));
}

void allocator_free(Allocator* allocator, void* memory) {
    void* block = (char*)memory - sizeof(size_t);
    size_t size = *(size_t*)block;
    int index = (int)log2(size + sizeof(size_t));

    while (1) {
        void* buddy = (void*)((size_t)block ^ (1 << index));
        void* list = allocator->free_lists[index];
        while (list) {
            if (list == buddy) {
                *(void**)block = *(void**)buddy;
                *(void**)buddy = NULL;
                block = (block < buddy) ? block : buddy;
                index++;
                break;
            }
            list = *(void**)list;
        }
        if (!list) {
            *(void**)block = allocator->free_lists[index];
            allocator->free_lists[index] = block;
            break;
        }
    }
}