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
};

enum RegisterType{
    REG_TEMPORARY = (0x1), 
    REG_SAVED = (0x1 << 1), 
    REG_ARGUMENTS = (0x1 << 2), 
    REG_CALLER_SAVED = REG_TEMPORARY | REG_ARGUMENTS,
    REG_ANY =  REG_SAVED | REG_CALLER_SAVED
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
        static uint64_t vRegisterCounter = RV64_Register::REG_COUNT;
        
        Register r;
        r.id = vRegisterCounter++;
        r.type = regType;

        return r;
    }
    
    void freeRegister(Register r){
        if (r.id < RV64_Register::REG_COUNT){
            x[r.id].occupied = false;
        }

        for (int i=0; i<RV64_Register::REG_COUNT; i++){
            if (x[i].vRegisterMapping == r.id){
                x[i].vRegisterMapping = 0;
                x[i].occupied = false;
            }
        }
        
    }

    RV64_Register resolveRegister(Register r){
        if (r.id < RV64_Register::REG_COUNT){
            return RV64_Register(r.id);
        }


        for (int i=0; i<RV64_Register::REG_COUNT; i++){
            if (x[i].vRegisterMapping == r.id){
                return RV64_Register(i);
            }
        }

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
        
        printf("Out of temporary registers.");
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