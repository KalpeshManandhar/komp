$arguments = @{
    "test_entry_point.c" = "i","am","mad","scientist","its","so","cool","sonuvabitch";
    "test_hello_world.c" = "";
    "test_execve.c" = "";
    "test_syscall.c" = "";
}

$expected_values = @{
    "test_entry_point.c" = 9;
    "test_hello_world.c" = 12;
    "test_execve.c" = 0;
    "test_syscall.c" = 0;
} 