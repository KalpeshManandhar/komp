# the riscv toolchain gcc executable in linux
$gcc_toolchain = "~/software/riscv"
$riscv_gcc = $gcc_toolchain + "/bin/riscv64-unknown-linux-gnu-gcc"
$qemu = $gcc_toolchain + "/bin/qemu-riscv64"
$sysroot = $gcc_toolchain + "/sysroot"

# path for current wd from linux
$cwd_from_linux = "/mnt/f/c/compiler/komp"

Write-Host "Toolchain path: " $gcc_toolchain  
Write-Host "RV64 gcc: " $riscv_gcc  
Write-Host "Qemu: " $qemu  
Write-Host "Cwd from linux: " $cwd_from_linux  

