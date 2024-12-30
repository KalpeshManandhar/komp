# komp

A work in progress compiler for a subset of the C programming language, indended to target the RISC-V ISA. 


## Current usage
### Compilation
#### Tokenizer
```
clang++ -g --std=c++20 -I.\src\ .\src\tokenizer\tokenizer.cpp .\src\tokenizer\tokenizer_main.cpp -o tokenizer.exe
```
#### Parser
```
clang++ -g --std=c++20 -I.\src\ .\src\tokenizer\tokenizer.cpp .\src\parser\*.cpp .\src\arena\arena.cpp -o parser.exe
```
#### Code Generator
```
clang++ -g --std=c++20 -I.\src\ .\src\tokenizer\tokenizer.cpp .\src\parser\parser.cpp .\src\arena\arena.cpp .\src\codeGen\*.cpp -o codegen.exe
```