Param(
    [Parameter(Mandatory, HelpMessage = "Usage: <script> <exec>")]
    [string]$exec_path
)

$test_folder = ".\tests\codeGen"


# add the following things to a new path_info.ps1 file
# # the riscv toolchain gcc executable in linux
# $gcc_toolchain = 
# $riscv_gcc = 
# $qemu = 
# $sysroot = 

# # path for current wd from linux
# $cwd_from_linux = 
. "$test_folder/path_info.ps1"


. "$test_folder/expected_info.ps1"
$found_values = $expected_values.Clone()


$files = Get-ChildItem -Path $test_folder"\*" -Include "*.c" 
foreach ($file in $files){  
    Write-Host "Test: " $file.Name -ForegroundColor Cyan  
    
    Write-Host "Generating asm:" -ForegroundColor Yellow  
    & "$exec_path" $file  
    
    Write-Host "Compiling into RV64-ELF.." -ForegroundColor Yellow  
    & "wsl" --distribution Ubuntu $riscv_gcc $cwd_from_linux/codegen_output.s -o $cwd_from_linux/codegen_output
    
    Write-Host "Running on qemu.." -ForegroundColor Yellow
    & "wsl" --distribution Ubuntu $qemu -L $sysroot $cwd_from_linux/codegen_output
      
    $found_values[$file.Name] = $LASTEXITCODE
} 

foreach ($file in $files){    
    if ($found_values[$file.Name] -eq $expected_values[$file.Name]){
        Write-Host -NoNewline "Test passed: " $file.Name " " -ForegroundColor Green    
    }
    else {
        Write-Host -NoNewline "Test failed: " $file.Name " " -ForegroundColor Red    
    }
    Write-Host "Found: "$found_values[$file.Name]"/ Expected: "$expected_values[$file.Name]"."
} 