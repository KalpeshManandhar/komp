
# TODO

### Specific refactors:
 - A minimal stdlib for wrappers around syscalls. 
    - Add memory functions + IO functions + Files + Strings for now
 - Maybe look into better register allocation (Linear scan maybe?)
    - Would probably require that the MIR be converted to a linear IR? Maybe? Dunno

 - Maybe change the arena to be dynamic in OS memory allocations instead of a fixed size table.
    - Maybe use a list?
 
 - Think about how the code can be restructured to make room for the optimization pass.
 - Also think about where each optimization pass lies in the pipeline, and how it will change the IR.

 
 - implement an arena to reduce chances of memory leaks and easy allocs/frees - done
 - find a better way to call the type checking function. - done



## Tokenizer
 - Character literal tokenization - done


## Parser:
 - Unions
 - Typedefs
 - Externs/Statics/Volatiles


 - Array/Struct initialization - done
 - Explicit type casts - done
 - Change declaration initializers to be an assignment - done
 - function params into symbols table - done
 - function parsing only in global - done
 - += -= operators in assignment + assignments are binary expressions - done
 - break/continue/return - done
 - arrays - indexing works, declarations done
 - type checking - done
 - structs - declaration done
    

    


## IR / Middleend

- Convert the C datatypes to a simpler form (tag + size like u32, i64) for simple conversions. - done
    - The code generator will be working with these only.
- All defined structs and other complex data types will be changed into the lower level IR. - done
    - So there will be no need of the Statement block info while generating code.
- Type conversions including arrays. - done
- Initializer lists - done


## Optimization
- Simple register allocation
    - Maybe allocate registers for first N local variables in a scope?
    - Or maybe implement a linear scan register allocator by adding a linear IR?
- Dead code elimination



## Code generator
- Fully support allocation for all registers - somewhat done?
- Proper support for function calls
    - Struct return values - remaining
    - Pass args in memory - done
    - Support for struct arguments - done
    - Callee saved registers - done
    - Caller saved registers - done
    - Can be made more efficient through better register assignment
- Codegen for string literals - done
- Call return primitives - done
- Type casts - done

- Change register assignment maybe


