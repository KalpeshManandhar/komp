# the riscv toolchain gcc executable in linux
$gcc_toolchain = "~/software/riscv"
$riscv_cpp = $gcc_toolchain + "/bin/riscv64-unknown-linux-gnu-cpp"
$riscv_gcc = $gcc_toolchain + "/bin/riscv64-unknown-linux-gnu-gcc"
$riscv_ld = $gcc_toolchain + "/bin/riscv64-unknown-linux-gnu-ld"
$qemu = $gcc_toolchain + "/bin/qemu-riscv64"
$sysroot = $gcc_toolchain + "/sysroot"

