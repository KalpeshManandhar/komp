
## General architecture
```
Tokenizer -> Parser -> Type checker/Context Analyzer ----> IR transformations/Optimizations ----> Code generator 
---------------------Frontend----------------------      -------------Middle-end-------------      ---Backend----
```
### Tokenizer 
- **Input:** Source code
- Tokenizes the input source text and labels the tokens.
- Used lazily by the parser, to get the next token whenever needed.
- A mixture of direct coded (for string literals, identifiers) and finite automata (number literals, character literals, punctuators)
    - Not very efficient


### Parser
- **Input:** Tokens from the tokenizer (Lazily called whenever needed)
- Recursive descent 
- Supports a subset of C grammar (can be found in **./docs/grammar.md**)
- Creates a general parse tree (nodes can be found in **./src/IR/node.h**)
- Logs messages for each error/warning found.
- Also builds up the symbol tables used for context analysis.
    - Variable tables store datatype info in order.
    - Function tables store function arguments, return type and other info in order.
    - Struct tables store member declaration info in order.

### Context Analyzer
- **Input:** Symbol tables and parse trees.
- Takes in the parse tree for an expression or any other sort of node.
- Ad hoc implementation within the parser (currently)
- Checks context 
    - Variable declarations
    - Function declarations and definition
    - Struct declarations and definition
    - Redefinition errors
    - Continue/Break/Return context checks
    - A lot of other stuff
- Checks types
    - Implicit type conversions
    - Checks if some type can be converted to another
        - Return value 
        - Function arguments
        - Assignments 
        - And more
    - Resulting type of operations

### IR transformations
```
Parse tree + Symbol tables -------------> LowLevelIR + Storage tables
```


#### Parse tree + Symbol tables
- The output of the frontend
- An IR in itself
- Very language-dependent structure
- Can be used however
- Datatype representations are C-based
- Symbol tables store info as before.

#### LowLevelIR? (I dont know what to call it yet)
- Language independent 
- Only works with the primitives given by the backend
- The actual process of converting language specific details into a language independent implementation.
- The code generator consumes this IR to produce the assembly.
- Datatypes are primitive like u8, u32, u64.
- Symbol tables store this newer datatype info along with space for storage info. 

### Code Generator
- Provides a few primitives nodes for code generation (the LowLevelIR) (can be found in **./src/IR/expanded-node.h**)
- Consumes this IR to generate RV64 assembly.
- Basic treewalk implementation.
- Temporary register allocation for expression generation using virtual registers.

- Assign storage locations to each variable.
- Allocate registers before using, and free after use.
- Each virtual register is resolved into a physical register when needed. 
- It maintains a physical register - vregister mapping.
