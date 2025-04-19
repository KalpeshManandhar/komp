Param(
    [Parameter(Mandatory, HelpMessage = "Usage: <script> <exec>")]
    [string]$exec_path,
    [string]$test_name
)

# requires PS 7.0 or higher for the isLinux/isWindows variables
if ($PSVersionTable.PSVersion.Major -lt 7) {
    Write-Host "Error: This script requires PowerShell 7.0 or later." -ForegroundColor Red
    exit 1
}



$test_folder = ".\tests\stdlib"


# add the following things to a new path_info.ps1 file
# the riscv toolchain gcc executable in linux
# $gcc_toolchain = "~/software/riscv"
# $riscv_cpp = $gcc_toolchain + "/bin/riscv64-unknown-linux-gnu-cpp"
# $riscv_gcc = $gcc_toolchain + "/bin/riscv64-unknown-linux-gnu-gcc"
# $riscv_ld = $gcc_toolchain + "/bin/riscv64-unknown-linux-gnu-ld"
# $qemu = $gcc_toolchain + "/bin/qemu-riscv64"
# $sysroot = $gcc_toolchain + "/sysroot"

. "./path_info.ps1"


. "$test_folder/expected_info.ps1"
$found_values = $expected_values.Clone()



$files = Get-ChildItem -Path $test_folder"\*" -Include "*.c" 
if ($PSBoundParameters.ContainsKey('test_name')){
    $files = $files | Where-Object -Property Name -eq $test_name
}



function Convert-WindowsPathToLinux {
    param (
        [Parameter(Mandatory=$true)]
        [string]$winPath
    )

    # Convert drive letter to lowercase and replace colon with '/mnt/'
    $winPath -match "([a-zA-Z]):" | Out-Null
    $driveName = $Matches[1].ToLower()
    $winPath = $winPath -replace "[a-zA-Z]:",  "/mnt/$driveName" 

    # Replace backslashes with forward slashes
    $linuxPath = $winPath -replace '\\', '/'

    return $linuxPath
}


$cwd = Get-Item -Path .

#  ------------ LINUX ---------------
if ($isLinux){
    $cwdLinux = $cwd
    
    
    foreach ($file in $files){  
        $linuxPath = $file.FullName

        Write-Host "Test: " $file.Name -ForegroundColor Cyan


        Write-Host "Invoking preprocessor.." -ForegroundColor Yellow
        & $riscv_cpp -I $cwdLinux/stdlib/include -P $linuxPath -o $cwdLinux/preprocessed.i
        
        Write-Host "Generating asm.." -ForegroundColor Yellow  
        & "$exec_path"  $cwd/preprocessed.i
        
        if ($whichLib[$file.Name] -eq "own"){
            Write-Host "Compiling into RV64 obj.." -ForegroundColor Yellow  
            & $riscv_gcc -nostdlib -c $cwdLinux/codegen_output.s -o $cwdLinux/codegen_output.o
            
            Write-Host "Linking into RV64-ELF.." -ForegroundColor Yellow  
            & $riscv_ld $cwdLinux/codegen_output.o $cwdLinux/stdlib/lib/entry.o $cwdLinux/stdlib/lib/stdlib.so -o $cwdLinux/codegen_output --dynamic-linker /lib/ld-linux-riscv64-lp64d.so.1                   
        }
        else {
            Write-Host "Compiling into RV64 obj.." -ForegroundColor Yellow  
            
            Write-Host "Linking into RV64-ELF.." -ForegroundColor Yellow  
            & $riscv_gcc $cwdLinux/codegen_output.s -o $cwdLinux/codegen_output
            
        }

        Write-Host "Running on qemu.." -ForegroundColor Yellow
        & $qemu -L $sysroot $cwdLinux/codegen_output @($arguments[$file.Name])
        
        $found_values[$file.Name] = $LASTEXITCODE
    } 
}
# ---------------- WINDOWS -------------------
elseif ($isWindows) {
    $cwdLinux = Convert-WindowsPathToLinux($cwd)
    
    foreach ($file in $files){  

        Write-Host "Test: " $file.Name -ForegroundColor Cyan
        
        $linuxPath = Convert-WindowsPathToLinux($file.FullName)
        Write-Host $linuxPath


        Write-Host "Invoking preprocessor.." -ForegroundColor Yellow
        Start-Process "wsl" -ArgumentList "--distribution", "Ubuntu", "$riscv_cpp -I $cwdLinux/stdlib/include -P $linuxPath -o $cwdLinux/preprocessed.i" -NoNewWindow -Wait
        
        Write-Host "Generating asm.." -ForegroundColor Yellow  
        & "$exec_path"  $cwd/preprocessed.i
        
        if ($whichLib[$file.Name] -eq "own"){
            Write-Host "Compiling into RV64 obj.." -ForegroundColor Yellow  
            Start-Process "wsl" -ArgumentList "--distribution", "Ubuntu", "$riscv_gcc -nostdlib -c $cwdLinux/codegen_output.s -o $cwdLinux/codegen_output.o" -NoNewWindow -Wait
            
            Write-Host "Linking into RV64-ELF.." -ForegroundColor Yellow  
            Start-Process "wsl" -ArgumentList "--distribution", "Ubuntu", 
                "$riscv_ld $cwdLinux/codegen_output.o $cwdLinux/stdlib/lib/entry.o $cwdLinux/stdlib/lib/stdlib.so -o $cwdLinux/codegen_output --dynamic-linker /lib/ld-linux-riscv64-lp64d.so.1" `
                -NoNewWindow -Wait
        }
        else {
            Write-Host "Compiling into RV64 obj.." -ForegroundColor Yellow  
            Write-Host "Linking into RV64-ELF.." -ForegroundColor Yellow  
            Start-Process "wsl" -ArgumentList "--distribution", "Ubuntu", "$riscv_gcc $cwdLinux/codegen_output.s -o $cwdLinux/codegen_output" -NoNewWindow -Wait   
        }

        Write-Host "Running on qemu.." -ForegroundColor Yellow
        & "wsl" --distribution Ubuntu $qemu -L $sysroot $cwdLinux/codegen_output @($arguments[$file.Name])
        
        $found_values[$file.Name] = $LASTEXITCODE
    } 
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