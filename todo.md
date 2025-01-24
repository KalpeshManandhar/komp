
# TODO

### Specific refactors:
 - Maybe change the arena to be dynamic in OS memory allocations instead of a fixed size table.
    - Maybe use a list?
 
 - Think about how the code can be restructured to make room for the optimization pass.
 - Also think about where each optimization pass lies in the pipeline, and how it will change the IR.

 
 - implement an arena to reduce chances of memory leaks and easy allocs/frees - done
 - find a better way to call the type checking function. - done



## Tokenizer
 - Character literal tokenization


## Parser:
 - Explicit type casts
 - Unions
 - Typedefs
 - Externs/Statics/Volatiles
 - Array/Struct initialization

 - Change declaration initializers to be an assignment



#### Done 

 - function params into symbols table - done
 - function parsing only in global - done
 - += -= operators in assignment + assignments are binary expressions - done
 - break/continue/return - done
    
 - arrays - indexing works, declarations done
 - type checking - done
 - structs - declaration done
    

    


## IR / Middleend
- Convert the C datatypes to a simpler form (tag + size like u32, i64) for simple conversions.
    - The code generator will be working with these only.
- All defined structs and other complex data types will be changed into the lower level IR. 
    - So there will be no need of the Statement block info while generating code.


## Code generator

- Fully support allocation for all registers
- Call return primitives?
- Proper support for function calls
- Type casts



