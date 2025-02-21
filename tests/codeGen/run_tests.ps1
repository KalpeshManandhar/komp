Param(
    [Parameter(Mandatory, HelpMessage = "Usage: <script> <exec>")]
    [string]$exec_path,
    [string]$test_name
)

$test_folder = ".\tests\codeGen"


# add the following things to a new path_info.ps1 file
# # the riscv toolchain gcc executable in linux
# $gcc_toolchain = 
# $riscv_gcc = 
# $qemu = 
# $sysroot = 

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

function Check-IsRunningOnLinux {
    param ()
    if ($PSVersionTable.PSEdition -eq 'Core') {
        if ($PSVersionTable.OS -match 'Linux') {
            return $true
        }
    } 
    return $false
}


$cwd = Get-Item -Path .

#  ------------ LINUX ---------------
if ($isLinux){
    $cwdLinux = $cwd
    
    foreach ($file in $files){  
        Write-Host "Test: " $file.Name -ForegroundColor Cyan  
        
        Write-Host "Generating asm:" -ForegroundColor Yellow  
        & "$exec_path" $file  
        
        Write-Host "Compiling into RV64-ELF.." -ForegroundColor Yellow  
        & "$riscv_gcc" $cwdLinux/codegen_output.s -o $cwdLinux/codegen_output
        
        Write-Host "Running on qemu.." -ForegroundColor Yellow
        & "$qemu" -L $sysroot $cwdLinux/codegen_output
        
        $found_values[$file.Name] = $LASTEXITCODE
    } 
}
#  ------------ WINDOWS ---------------
elseif ($isWindows){
    $cwdLinux = Convert-WindowsPathToLinux($cwd)

    foreach ($file in $files){  
        Write-Host "Test: " $file.Name -ForegroundColor Cyan  
        
        Write-Host "Generating asm:" -ForegroundColor Yellow  
        & "$exec_path" $file  
        
        Write-Host "Compiling into RV64-ELF.." -ForegroundColor Yellow  
        & "wsl" --distribution Ubuntu $riscv_gcc $cwdLinux/codegen_output.s -o $cwdLinux/codegen_output
        
        Write-Host "Running on qemu.." -ForegroundColor Yellow
        & "wsl" --distribution Ubuntu $qemu -L $sysroot $cwdLinux/codegen_output
        
        $found_values[$file.Name] = $LASTEXITCODE
    } 
}

Write-Host $cwdLinux



foreach ($file in $files){    
    if ($found_values[$file.Name] -eq $expected_values[$file.Name]){
        Write-Host -NoNewline "Test passed: " $file.Name " " -ForegroundColor Green    
    }
    else {
        Write-Host -NoNewline "Test failed: " $file.Name " " -ForegroundColor Red    
    }
    Write-Host "Found: "$found_values[$file.Name]"/ Expected: "$expected_values[$file.Name]"."
} 