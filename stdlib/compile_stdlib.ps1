Param(
    [Parameter(Mandatory)]
    [string]$src_folder,
    [string]$lib_name
)


# the riscv toolchain gcc executable in linux
# $gcc_toolchain = "~/software/riscv"
# $riscv_gcc = $gcc_toolchain + "/bin/riscv64-unknown-linux-gnu-gcc"

. "./path_info.ps1"


# Get all .s files in the source folder
$files = Get-ChildItem -Path "$src_folder\*" -Include "*.s"

# If a specific library name is provided, filter the files
if ($PSBoundParameters.ContainsKey('lib_name')) {
    $files = $files | Where-Object { $_.Name -eq $lib_name }
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


New-Item -ItemType Directory -Force -Path $cwd/stdlib/lib

# ---------------- LINUX -------------------
if ($isLinux){
    $cwdLinux = $cwd

    # assemble all to .o files
    Write-Host "Compiling/Assembling into .o files..." -ForegroundColor Yellow  
    foreach ($file in $files) {  
        $fileBaseName = [System.IO.Path]::GetFileNameWithoutExtension($file.Name)
        $fileSrcName = $fileBaseName + ".s"
        $fileObjName = $fileBaseName + ".o"
        
        Write-Host "Processing: " $file.Name -ForegroundColor Cyan  
        
        
        & $riscv_gcc "-c" "$cwdLinux/stdlib/src/$fileSrcName" -o "$cwdLinux/stdlib/lib/$fileObjName"
    }

    # link all .o files except the entry.o file to a shared object
    $objFiles = Get-ChildItem -Path "$cwd/stdlib/lib" -Filter "*.o" `
                | Where-Object {$_.Name -ne "entry.o"} `

    $stdlibName = "stdlib.so"
    Write-Host "Linking .o files into $stdlibName..." -ForegroundColor Yellow  

    & $riscv_gcc -shared -nostdlib @($objFiles) -o "$cwdLinux/stdlib/lib/$stdlibName"

}
# ---------------- WINDOWS -------------------
elseif ($isWindows){
    $cwdLinux = Convert-WindowsPathToLinux($cwd)

    # assemble all to .o files
    Write-Host "Compiling/Assembling into .o files..." -ForegroundColor Yellow  
    foreach ($file in $files) {  
        $fileBaseName = [System.IO.Path]::GetFileNameWithoutExtension($file.Name)
        $fileSrcName = $fileBaseName + ".s"
        $fileObjName = $fileBaseName + ".o"
        
        Write-Host "Processing: " $file.Name -ForegroundColor Cyan  
        
        
        & wsl --distribution Ubuntu `
            $riscv_gcc "-c" `
            "$cwdLinux/stdlib/src/$fileSrcName" `
            -o "$cwdLinux/stdlib/lib/$fileObjName"
    }

    # link all .o files except the entry.o file to a shared object
    $objFiles = Get-ChildItem -Path "$cwd/stdlib/lib" -Filter "*.o" `
                | Where-Object {$_.Name -ne "entry.o"} `
                | ForEach-Object -Process {Convert-WindowsPathToLinux($_.FullName.ToString())}

    $stdlibName = "stdlib.so"
    Write-Host "Linking .o files into $stdlibName..." -ForegroundColor Yellow  

    & wsl --distribution Ubuntu `
        $riscv_gcc -shared -nostdlib `
        @($objFiles) `
        -o "$cwdLinux/stdlib/lib/$stdlibName"

}