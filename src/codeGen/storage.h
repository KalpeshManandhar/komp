#pragma once

#include <stdint.h>
#include <assert.h>
#include <IR/symbol-table.h>


enum RV64_Register{
    REG_ZERO, // hardwired 0
    REG_RA, // return address
    REG_SP, // stack pointer
    REG_GP, // global pointer
    REG_TP, // thread pointer
    
    // temporary registers
    REG_T0, // alternate link register
    REG_T1,
    REG_T2,
    
    // saved registers
    REG_S0, // frame pointer
    REG_S1,
    
    // function arguments/return values
    REG_A0, 
    REG_A1,
    REG_A2,
    REG_A3,
    REG_A4,
    REG_A5,
    REG_A6,
    REG_A7,
    
    // saved registers
    REG_S2,
    REG_S3,
    REG_S4,
    REG_S5,
    REG_S6,
    REG_S7,
    REG_S8,
    REG_S9,
    REG_S10,
    REG_S11,

    // temporaries
    REG_T3,
    REG_T4,
    REG_T5,
    REG_T6,
    
    // floating point registers
    // temporaries
    REG_FT0,
    REG_FT1,
    REG_FT2,
    REG_FT3,
    REG_FT4,
    REG_FT5,
    REG_FT6,
    REG_FT7,
    
    // saved registers
    REG_FS0,
    REG_FS1,

    // function args/return values
    REG_FA0,
    REG_FA1,

    // function args
    REG_FA2,
    REG_FA3,
    REG_FA4,
    REG_FA5,
    REG_FA6,
    REG_FA7,

    // saved registers
    REG_FS2,
    REG_FS3,
    REG_FS4,
    REG_FS5,
    REG_FS6,
    REG_FS7,
    REG_FS8,
    REG_FS9,
    REG_FS10,
    REG_FS11,

    // temporaries
    REG_FT8,
    REG_FT9,
    REG_FT10,
    REG_FT11,

    REG_COUNT
};

static const char* RV64_RegisterName[RV64_Register::REG_COUNT] = {
    "zero", // hardwired 0
    "ra", // return address
    "sp", // stack pointer
    "gp", // global pointer
    "tp", // thread pointer
    
    // temporary registers
    "t0", // alternate link register
    "t1",
    "t2",
    
    // saved registers
    "s0", // frame pointer
    "s1",
    
    // function arguments/return values
    "a0", 
    "a1",
    "a2",
    "a3",
    "a4",
    "a5",
    "a6",
    "a7",
    
    // saved registers
    "s2",
    "s3",
    "s4",
    "s5",
    "s6",
    "s7",
    "s8",
    "s9",
    "s10",
    "s11",

    // temporaries
    "t3",
    "t4",
    "t5",
    "t6",

        // floating point registers
    // temporaries
    "ft0",
    "ft1",
    "ft2",
    "ft3",
    "ft4",
    "ft5",
    "ft6",
    "ft7",
    
    // saved registers
    "fs0",
    "fs1",

    // function args/return values
    "fa0",
    "fa1",

    // function args
    "fa2",
    "fa3",
    "fa4",
    "fa5",
    "fa6",
    "fa7",

    // saved registers
    "fs2",
    "fs3",
    "fs4",
    "fs5",
    "fs6",
    "fs7",
    "fs8",
    "fs9",
    "fs10",
    "fs11",

    // temporaries
    "ft8",
    "ft9",
    "ft10",
    "ft11",

};




enum RegisterType{
    // These types are processed as either one or the other may be true 
    REG_TEMPORARY       = (0x1 << 0), 
    REG_SAVED           = (0x1 << 1), 
    REG_ARGUMENTS       = (0x1 << 2), 
    REG_CALLER_SAVED    = (0x1 << 3),
    REG_CALLEE_SAVED    = (0x1 << 4),
    REG_RETURN_VALUES   = (0x1 << 5), 

    // These types are processed as one *needs* to be true
    REG_FLOATING_POINT  = (0x1 << 6), 
    REG_DONOT_ALLOCATE  = (0x1 << 7), 

    REG_ANY =  REG_TEMPORARY | REG_SAVED | REG_ARGUMENTS,

};

struct Register{
    uint64_t id;
    RegisterType type;
};

struct RegisterInfo{
    bool occupied;
    uint64_t vRegisterMapping;
};

static RegisterInfo x[REG_COUNT];
static RegisterInfo f[REG_COUNT];


struct RV64RegisterInfo {
    uint64_t id;
    const char* name;
    RegisterType type;
};


static RV64RegisterInfo RV64Registers[REG_COUNT] = {
    {.id = REG_ZERO, .name = "zero", .type = RegisterType(REG_DONOT_ALLOCATE)}, // hardwired 0
    {.id = REG_RA, .name = "ra", .type = RegisterType(REG_DONOT_ALLOCATE | REG_CALLER_SAVED)}, // return address
    {.id = REG_SP, .name = "sp", .type = RegisterType(REG_DONOT_ALLOCATE | REG_CALLEE_SAVED)}, // stack pointer
    {.id = REG_GP, .name = "gp", .type = RegisterType(REG_DONOT_ALLOCATE)}, // global pointer
    {.id = REG_TP, .name = "tp", .type = RegisterType(REG_DONOT_ALLOCATE)}, // thread pointer
    
    // temporary registers
    {.id = REG_T0, .name = "t0", .type = RegisterType(REG_TEMPORARY | REG_CALLER_SAVED)}, // alternate link register
    {.id = REG_T1, .name = "t1", .type = RegisterType(REG_TEMPORARY | REG_CALLER_SAVED)},
    {.id = REG_T2, .name = "t2", .type = RegisterType(REG_TEMPORARY | REG_CALLER_SAVED)},
    
    // saved registers
    {.id = REG_S0, .name = "s0", .type = RegisterType(REG_SAVED | REG_CALLEE_SAVED)}, // frame pointer
    {.id = REG_S1, .name = "s1", .type = RegisterType(REG_SAVED | REG_CALLEE_SAVED)},
    
    // function arguments/return values
    {.id = REG_A0, .name = "a0", .type = RegisterType(REG_ARGUMENTS | REG_CALLER_SAVED | REG_RETURN_VALUES)}, 
    {.id = REG_A1, .name = "a1", .type = RegisterType(REG_ARGUMENTS | REG_CALLER_SAVED | REG_RETURN_VALUES)},
    {.id = REG_A2, .name = "a2", .type = RegisterType(REG_ARGUMENTS | REG_CALLER_SAVED)},
    {.id = REG_A3, .name = "a3", .type = RegisterType(REG_ARGUMENTS | REG_CALLER_SAVED)},
    {.id = REG_A4, .name = "a4", .type = RegisterType(REG_ARGUMENTS | REG_CALLER_SAVED)},
    {.id = REG_A5, .name = "a5", .type = RegisterType(REG_ARGUMENTS | REG_CALLER_SAVED)},
    {.id = REG_A6, .name = "a6", .type = RegisterType(REG_ARGUMENTS | REG_CALLER_SAVED)},
    {.id = REG_A7, .name = "a7", .type = RegisterType(REG_ARGUMENTS | REG_CALLER_SAVED)},
    
    // saved registers
    {.id = REG_S2, .name = "s2", .type = RegisterType(REG_SAVED | REG_CALLEE_SAVED)},
    {.id = REG_S3, .name = "s3", .type = RegisterType(REG_SAVED | REG_CALLEE_SAVED)},
    {.id = REG_S4, .name = "s4", .type = RegisterType(REG_SAVED | REG_CALLEE_SAVED)},
    {.id = REG_S5, .name = "s5", .type = RegisterType(REG_SAVED | REG_CALLEE_SAVED)},
    {.id = REG_S6, .name = "s6", .type = RegisterType(REG_SAVED | REG_CALLEE_SAVED)},
    {.id = REG_S7, .name = "s7", .type = RegisterType(REG_SAVED | REG_CALLEE_SAVED)},
    {.id = REG_S8, .name = "s8", .type = RegisterType(REG_SAVED | REG_CALLEE_SAVED)},
    {.id = REG_S9, .name = "s9", .type = RegisterType(REG_SAVED | REG_CALLEE_SAVED)},
    {.id = REG_S10, .name = "s10", .type = RegisterType(REG_SAVED | REG_CALLEE_SAVED)},
    {.id = REG_S11, .name = "s11", .type = RegisterType(REG_SAVED | REG_CALLEE_SAVED)},

    // temporaries
    {.id = REG_T3, .name = "t3", .type = RegisterType(REG_TEMPORARY | REG_CALLER_SAVED)},
    {.id = REG_T4, .name = "t4", .type = RegisterType(REG_TEMPORARY | REG_CALLER_SAVED)},
    {.id = REG_T5, .name = "t5", .type = RegisterType(REG_TEMPORARY | REG_CALLER_SAVED)},
    {.id = REG_T6, .name = "t6", .type = RegisterType(REG_TEMPORARY | REG_CALLER_SAVED)},

        // floating point registers
    // temporaries
    {.id = REG_FT0, .name = "ft0", .type = RegisterType(REG_FLOATING_POINT | REG_TEMPORARY | REG_CALLER_SAVED)},
    {.id = REG_FT1, .name = "ft1", .type = RegisterType(REG_FLOATING_POINT | REG_TEMPORARY | REG_CALLER_SAVED)},
    {.id = REG_FT2, .name = "ft2", .type = RegisterType(REG_FLOATING_POINT | REG_TEMPORARY | REG_CALLER_SAVED)},
    {.id = REG_FT3, .name = "ft3", .type = RegisterType(REG_FLOATING_POINT | REG_TEMPORARY | REG_CALLER_SAVED)},
    {.id = REG_FT4, .name = "ft4", .type = RegisterType(REG_FLOATING_POINT | REG_TEMPORARY | REG_CALLER_SAVED)},
    {.id = REG_FT5, .name = "ft5", .type = RegisterType(REG_FLOATING_POINT | REG_TEMPORARY | REG_CALLER_SAVED)},
    {.id = REG_FT6, .name = "ft6", .type = RegisterType(REG_FLOATING_POINT | REG_TEMPORARY | REG_CALLER_SAVED)},
    {.id = REG_FT7, .name = "ft7", .type = RegisterType(REG_FLOATING_POINT | REG_TEMPORARY | REG_CALLER_SAVED)},
    
    // saved registers
    {.id = REG_FS0, .name = "fs0", .type = RegisterType(REG_FLOATING_POINT | REG_SAVED | REG_CALLEE_SAVED)},
    {.id = REG_FS1, .name = "fs1", .type = RegisterType(REG_FLOATING_POINT | REG_SAVED | REG_CALLEE_SAVED)},

    // function args/return values
    {.id = REG_FA0, .name = "fa0", .type = RegisterType(REG_FLOATING_POINT | REG_ARGUMENTS | REG_RETURN_VALUES | REG_CALLER_SAVED)},
    {.id = REG_FA1, .name = "fa1", .type = RegisterType(REG_FLOATING_POINT | REG_ARGUMENTS | REG_RETURN_VALUES | REG_CALLER_SAVED)},

    // function args
    {.id = REG_FA2, .name = "fa2", .type = RegisterType(REG_FLOATING_POINT | REG_ARGUMENTS | REG_CALLER_SAVED)},
    {.id = REG_FA3, .name = "fa3", .type = RegisterType(REG_FLOATING_POINT | REG_ARGUMENTS | REG_CALLER_SAVED)},
    {.id = REG_FA4, .name = "fa4", .type = RegisterType(REG_FLOATING_POINT | REG_ARGUMENTS | REG_CALLER_SAVED)},
    {.id = REG_FA5, .name = "fa5", .type = RegisterType(REG_FLOATING_POINT | REG_ARGUMENTS | REG_CALLER_SAVED)},
    {.id = REG_FA6, .name = "fa6", .type = RegisterType(REG_FLOATING_POINT | REG_ARGUMENTS | REG_CALLER_SAVED)},
    {.id = REG_FA7, .name = "fa7", .type = RegisterType(REG_FLOATING_POINT | REG_ARGUMENTS | REG_CALLER_SAVED)},

    // saved registers
    {.id = REG_FS2, .name = "fs2", .type = RegisterType(REG_FLOATING_POINT | REG_SAVED | REG_CALLEE_SAVED)},
    {.id = REG_FS3, .name = "fs3", .type = RegisterType(REG_FLOATING_POINT | REG_SAVED | REG_CALLEE_SAVED)},
    {.id = REG_FS4, .name = "fs4", .type = RegisterType(REG_FLOATING_POINT | REG_SAVED | REG_CALLEE_SAVED)},
    {.id = REG_FS5, .name = "fs5", .type = RegisterType(REG_FLOATING_POINT | REG_SAVED | REG_CALLEE_SAVED)},
    {.id = REG_FS6, .name = "fs6", .type = RegisterType(REG_FLOATING_POINT | REG_SAVED | REG_CALLEE_SAVED)},
    {.id = REG_FS7, .name = "fs7", .type = RegisterType(REG_FLOATING_POINT | REG_SAVED | REG_CALLEE_SAVED)},
    {.id = REG_FS8, .name = "fs8", .type = RegisterType(REG_FLOATING_POINT | REG_SAVED | REG_CALLEE_SAVED)},
    {.id = REG_FS9, .name = "fs9", .type = RegisterType(REG_FLOATING_POINT | REG_SAVED | REG_CALLEE_SAVED)},
    {.id = REG_FS10, .name = "fs10", .type = RegisterType(REG_FLOATING_POINT | REG_SAVED | REG_CALLEE_SAVED)},
    {.id = REG_FS11, .name = "fs11", .type = RegisterType(REG_FLOATING_POINT | REG_SAVED | REG_CALLEE_SAVED)},

    // temporaries
    {.id = REG_FT8, .name = "ft8", .type = RegisterType(REG_FLOATING_POINT | REG_TEMPORARY | REG_CALLER_SAVED)},
    {.id = REG_FT9, .name = "ft9", .type = RegisterType(REG_FLOATING_POINT | REG_TEMPORARY | REG_CALLER_SAVED)},
    {.id = REG_FT10, .name = "ft10", .type = RegisterType(REG_FLOATING_POINT | REG_TEMPORARY | REG_CALLER_SAVED)},
    {.id = REG_FT11, .name = "ft11", .type = RegisterType(REG_FLOATING_POINT | REG_TEMPORARY | REG_CALLER_SAVED)},
};



struct RegisterState{
    RegisterInfo x[REG_COUNT];
};


struct RegisterAllocator{
    
    // allocate a specific register
    Register allocRegister(RV64_Register reg){
        assert(!x[reg].occupied);
        x[reg].occupied = true;
        
        Register r;
        r.id = reg;
        return r;
    }
    
    // allocate a virtual register to be resolved later
    Register allocVRegister(RegisterType regType = RegisterType::REG_ANY){
        assert(regType != RegisterType::REG_FLOATING_POINT && regType != REG_DONOT_ALLOCATE && "That is not a valid register type you dumbum.");

        static uint64_t vRegisterCounter = RV64_Register::REG_COUNT;
        
        Register r;
        r.id = vRegisterCounter++;
        r.type = regType;

        return r;
    }
    
    void freeRegister(Register r){
        // if register is physical register
        if (r.id < RV64_Register::REG_COUNT){
            x[r.id].occupied = false;
        }
        
        // if register is virtual, remove mapping
        for (int i=0; i<RV64_Register::REG_COUNT; i++){
            if (x[i].vRegisterMapping == r.id){
                x[i].vRegisterMapping = 0;
                x[i].occupied = false;
            }
        }
        
    }

    RV64_Register resolveRegister(Register r){
        // if register is physical register
        if (r.id < RV64_Register::REG_COUNT){
            return RV64_Register(r.id);
        }

        // if virtual and mapped, return the physical register with the mapping
        for (int i=0; i<RV64_Register::REG_COUNT; i++){
            if (x[i].vRegisterMapping == r.id){
                return RV64_Register(i);
            }
        }
        
        assert(r.type != RegisterType::REG_FLOATING_POINT && r.type != REG_DONOT_ALLOCATE && "That is not a valid register type you dumbum.");
        for (int i=0; i<REG_COUNT; i++){
            RV64RegisterInfo pRegister = RV64Registers[i];
            // if donot allocate, then just dont allocate?
            if (pRegister.type & RegisterType::REG_DONOT_ALLOCATE){
                continue;
            }
            
            // floating point bit must match
            if ((pRegister.type & RegisterType::REG_FLOATING_POINT) != (r.type & RegisterType::REG_FLOATING_POINT)){
                continue;
            }
            
            // if any other bits match, then this register can be used for mapping
            if (pRegister.type & r.type & (~RegisterType::REG_FLOATING_POINT)){
                if (!x[i].occupied){
                    x[i].occupied = true;
                    x[i].vRegisterMapping = r.id;
                    return RV64_Register(i);
                }
            }
        }
        assert(false && "Register resolution went wrong huhu.");
        


        // if virtual and unmapped, then add mapping
        // floating point registers
        if (r.type & RegisterType::REG_FLOATING_POINT){
            if (r.type & RegisterType::REG_TEMPORARY){
                for (int i=RV64_Register::REG_FT0; i<=RV64_Register::REG_FT2; i++){
                    if (!x[i].occupied){
                        x[i].occupied = true;
                        x[i].vRegisterMapping = r.id;
                        return RV64_Register(i);
                    }
                }
                for (int i=RV64_Register::REG_FT3; i<=RV64_Register::REG_FT6; i++){
                    if (!x[i].occupied){
                        x[i].occupied = true;
                        x[i].vRegisterMapping = r.id;
                        return RV64_Register(i);
                    }
                }
            }
        }
        
        // integer registers
        if (r.type & RegisterType::REG_TEMPORARY){
            for (int i=RV64_Register::REG_T0; i<=RV64_Register::REG_T2; i++){
                if (!x[i].occupied){
                    x[i].occupied = true;
                    x[i].vRegisterMapping = r.id;
                    return RV64_Register(i);
                }
            }
            for (int i=RV64_Register::REG_T3; i<=RV64_Register::REG_T6; i++){
                if (!x[i].occupied){
                    x[i].occupied = true;
                    x[i].vRegisterMapping = r.id;
                    return RV64_Register(i);
                }
            }
        }
        
        assert(false && "Out of temporary registers.");
        return RV64_Register::REG_A0;
    }

    RegisterState getRegisterState(RegisterType type){
        RegisterState state = {0};

        if (type & RegisterType::REG_TEMPORARY){
            for (int i=RV64_Register::REG_T0; i<=RV64_Register::REG_T2; i++){
                state.x[i] = x[i];
            }
            for (int i=RV64_Register::REG_T3; i<=RV64_Register::REG_T6; i++){
                state.x[i] = x[i];
            }
        }
        
        if (type & RegisterType::REG_SAVED){
            for (int i=RV64_Register::REG_S0; i<=RV64_Register::REG_S1; i++){
                state.x[i] = x[i];
            }
            for (int i=RV64_Register::REG_S2; i<=RV64_Register::REG_S11; i++){
                state.x[i] = x[i];
            }
        }
        
        if (type & RegisterType::REG_ARGUMENTS){
            for (int i=RV64_Register::REG_A0; i<=RV64_Register::REG_A7; i++){
                state.x[i] = x[i];
            }
        }
        
        if (type & RegisterType::REG_FLOATING_POINT){
            for (int i=RV64_Register::REG_A0; i<=RV64_Register::REG_A7; i++){
                state.x[i] = x[i];
            }
        }
        
        return state;
    }

    void save(RegisterState state){
        for (int i=0; i<=RV64_Register::REG_COUNT; i++){
            if (state.x[i].occupied){
                x[i].occupied = false;
                x[i].vRegisterMapping = 0;
            }
        }
    }
    
    
    void restore(RegisterState state){
        for (int i=0; i<=RV64_Register::REG_COUNT; i++){
            if (state.x[i].occupied)
                x[i] = state.x[i];
        }
        
    }
};




struct StorageInfo{
    enum StorageTag{
        STORAGE_MEMORY,
        STORAGE_REGISTER,
    }tag;
    size_t size;
    
    union{
        Register reg;
        size_t memAddress;
    };
};


struct StackAllocator{
    size_t sp = 0;
    
    size_t allocate(size_t size){
        size_t spRet = sp;
        
        sp += size;
        return spRet;
    }
    
    void deallocate(size_t size){
        sp -= size;
    }

    size_t getCurrentAddress(){
        return sp;
    }
};



struct ScopeInfo{
    SymbolTable<StorageInfo> storage;
    size_t frameBase;

    ScopeInfo *parent;
};