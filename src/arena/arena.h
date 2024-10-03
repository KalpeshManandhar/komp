#pragma once

#include <stdint.h>

#define PAGE_SIZE 4096


struct Arena{
private:
    const static int MAPPING_INFO_TABLE_SIZE = 64;
    const static int FRAME_TABLE_SIZE = 64;
    const static int ALIGNMENT = 8;

    struct Map{
        void *mem = 0;
        size_t capacity = 0;
        size_t allocated = 0;
    }map[MAPPING_INFO_TABLE_SIZE];

    int currentMap;
    size_t mapAllocSize;

    struct {
        struct Frame{
            int mapNo;
            size_t allocated;
        }frames[FRAME_TABLE_SIZE];
        int currentFrame = -1;
    }frameStack;
    

    bool   allocFromOS(size_t capacity);
    size_t alignUpPowerOf2(size_t address, size_t align);

public:
    void  init(size_t capacity);
    void  destroy();
    
    // create a new stack frame  
    bool  createFrame();

    // deletes the frame invalidating all allocated instances within the frame
    bool  destroyFrame();

    void* alloc(size_t size);

};
