# komp

A work in progress compiler for a subset of the C programming language, indended to target the RISC-V ISA. 


## Current usage
### Compilation
#### Tokenizer
```powershell
clang++ -g --std=c++20 -I./src/ ./src/tokenizer/tokenizer.cpp ./src/tokenizer/tokenizer_main.cpp -o tokenizer.exe
```
#### Parser
```powershell
clang++ -g --std=c++20 -I./src/ ./src/tokenizer/tokenizer.cpp ./src/parser/*.cpp ./src/arena/arena.cpp -o parser.exe
```
#### Code Generator
For the current code, after the middle end refactor.
```powershell
clang++ -g --std=c++20 -I./src/ ./src/tokenizer/tokenizer.cpp ./src/parser/parser.cpp ./src/arena/arena.cpp ./src/IR/middle-end.cpp ./src/codeGen/*.cpp -o codegen.exe
```

#### Standard library
The standard library currently consists of functions wrapping some common syscalls to form a minimal stdlib experience (wow!). 
You can compile the stdlib by running the `compile_stdlib.ps1`.

### Testing
All tests are in **`./tests/`** with respective tests for each stage in the pipeline. 
Due to it being a pain to compare the outputs of the stages, the number of errors found has been used as the test metric for the parser, and the program return value has been used as the metric for the code generator, whose expected values are in the **`expected_info.ps1`** files. Powershell scripts are used to automate the tests, which can be run as

```
run_tests.ps1 <executable to be tested> <specific test name if needed>
```

For the code generator, the tests are run using the [**riscv-gnu-toolchain**](https://github.com/riscv-collab/riscv-gnu-toolchain) using the **riscv64-linux-elf-gcc** to assemble and link the generated assembly to an executable. The binary is the run on **qemu-riscv64** and the return value is checked.

***NOTE:** All scripts that require some form of assembling/linking require path information of the gnu-toolchain, please make a file `./path_info.ps1` in the project directory and add the paths for the given.*
```powershell
# the riscv toolchain gcc executable in linux
$gnu_toolchain = "path/to/gnu-toolchain"
$riscv_cpp = $gnu_toolchain + "/bin/riscv64-unknown-linux-gnu-cpp"
$riscv_gcc = $gnu_toolchain + "/bin/riscv64-unknown-linux-gnu-gcc"
$riscv_ld = $gnu_toolchain + "/bin/riscv64-unknown-linux-gnu-ld"
$qemu = $gnu_toolchain + "/bin/qemu-riscv64"
$sysroot = $gnu_toolchain + "/sysroot"
```

