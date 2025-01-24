# komp

A work in progress compiler for a subset of the C programming language, indended to target the RISC-V ISA. 


## Current usage
### Compilation
#### Tokenizer
```powershell
clang++ -g --std=c++20 -I.\src\ .\src\tokenizer\tokenizer.cpp .\src\tokenizer\tokenizer_main.cpp -o tokenizer.exe
```
#### Parser
```powershell
clang++ -g --std=c++20 -I.\src\ .\src\tokenizer\tokenizer.cpp .\src\parser\*.cpp .\src\arena\arena.cpp -o parser.exe
```
#### Code Generator
For the current code, after the middle end refactor.
```powershell
clang++ -g --std=c++20 -I.\src\ .\src\tokenizer\tokenizer.cpp .\src\parser\parser.cpp .\src\arena\arena.cpp .\src\IR\middle-end.cpp .\src\codeGen\*.cpp -o codegen.exe
```
For the previous code, before the middle end. WARNING: May or may not work.
```powershell
clang++ -g --std=c++20 -I.\src\ .\src\tokenizer\tokenizer.cpp .\src\parser\parser.cpp .\src\arena\arena.cpp .\src\codeGen\*.cpp -o codegen.exe
```


### Testing
All tests are in **`./tests/`** with respective tests for each stage in the pipeline. 
Due to it being a pain to compare the outputs of the stages, the number of errors found has been used as the test metric for the parser, and the program return value has been used as the metric for the code generator, whose expected values are in the **`expected_info.ps1`** files. Powershell scripts are used to automate the tests, which can be run as

```
run_tests.ps1 <executable to be tested> <specific test name if needed>
```

For the code generator, the tests are run using the [**riscv-gnu-toolchain**](https://github.com/riscv-collab/riscv-gnu-toolchain) using the **riscv64-linux-elf-gcc** to assemble and link the generated assembly to an executable. The binary is the run on **qemu-riscv64** and the return value is checked.

***NOTE:** The script assumes you are running on Windows, and uses wsl with Ubuntu to invoke the riscv64-gcc and qemu. If you are doing so as well, please make a file `./tests/codeGen/path_info.ps1` and add the paths for the given.*
```powershell
# the riscv toolchain gcc executable in linux
$gcc_toolchain = 
$riscv_gcc = 
$qemu = 
$sysroot = 

# path for current wd from linux
$cwd_from_linux = 
```
