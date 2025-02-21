#include "arena.h"
#include <stdio.h>
#include <assert.h>

#if defined(_WIN32)
#include <Windows.h>
#elif defined(__linux__)
#include <sys/mman.h>
#endif




size_t Arena::alignUpPowerOf2(size_t address, size_t align) {
    return((address + (align - 1)) & ~(align -1));
}


bool Arena::allocFromOS(size_t capacity){
#if defined(_WIN32)
    map[currentMap].mem = VirtualAlloc(0, capacity, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
#elif defined(__linux__)
    map[currentMap].mem = mmap(0, capacity, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
#endif

    if (!map[currentMap].mem){
        perror("Memory alloc failed");
        return false;
    }

    map[currentMap].capacity  = capacity;
    map[currentMap].allocated = 0;
    
    return true;
}

void  Arena::destroy(){
    // assert(this->mem && this->capacity > 0);
    
    for (int i = 0; i < MAPPING_INFO_TABLE_SIZE; i++){
        if (map[i].capacity == 0){
            break;
        }
#if defined(_WIN32)
        VirtualFree(map[i].mem, map[i].capacity, MEM_RELEASE);
#elif defined(__linux__)
        munmap(map[i].mem, map[i].capacity);
#endif


    }

}




void  Arena::init(size_t capacity){
    mapAllocSize = alignUpPowerOf2(capacity, PAGE_SIZE);
    currentMap = 0;
    allocFromOS(mapAllocSize);
}



bool  Arena::createFrame(){
    if (frameStack.currentFrame == FRAME_TABLE_SIZE - 1){
        fprintf(stderr, "Frame limit exceeded.\n");
        return false;
    }

    frameStack.frames[++frameStack.currentFrame] = {
        .mapNo = currentMap,
        .allocated = map[currentMap].allocated
    };

    return true;
}


bool  Arena::destroyFrame(){
    if (frameStack.currentFrame == -1){
        fprintf(stderr, "No frame created.\n");
        return false;
    }
    
    currentMap = frameStack.frames[frameStack.currentFrame].mapNo;
    map[currentMap].allocated = frameStack.frames[frameStack.currentFrame].allocated;
    frameStack.currentFrame--;

    return true;
}

void* Arena::alloc(size_t size){
    // align size to a multiple of 8 bytes
    size = alignUpPowerOf2(size, ALIGNMENT);
    

    // check if any of the maps in the frame has enough space for allocation
    for (int frameMap = frameStack.frames[frameStack.currentFrame].mapNo; frameMap <= currentMap; frameMap++){
        if (map[frameMap].allocated + size <= map[frameMap].capacity){
            void *mem = (void *)((size_t)map[frameMap].mem + map[frameMap].allocated);
            map[frameMap].allocated += size;
            return mem;
        }
    }

    // if none of the maps has enough space for allocation, then go to next map
    if (map[currentMap].allocated + size > map[currentMap].capacity){
        currentMap++;
        
        if (currentMap == MAPPING_INFO_TABLE_SIZE){
            fprintf(stderr, "Memory limit reached.\n");
            return NULL;
        }

        // if the next map hasn't been allocated from OS, then alloc
        if (map[currentMap].capacity == 0){
            allocFromOS(mapAllocSize);
        }

        map[currentMap].allocated = 0;
    }

    void *mem = (void *)((size_t)map[currentMap].mem + map[currentMap].allocated);
    map[currentMap].allocated += size;
    return mem;
}