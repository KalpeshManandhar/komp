$whichLib = @{
    "test_entry_point.c" = "own";
    "test_hello_world.c" = "own";
    "test_test_server.c" = "own";
    "test_execve.c" = "own";
    "test_syscall.c" = "own";
    "test_shell.c" = "own";
    "test_mandelbrot.c" = "other";
    "test_http.c" = "own";
    "test_math_interpreter.c" = "own";
}
$arguments = @{
    "test_entry_point.c" = "i","am","mad","scientist","its","so","cool","sonuvabitch";
    "test_hello_world.c" = "";
    "test_test_server.c" = "";
    "test_execve.c" = "";
    "test_syscall.c" = "";
    "test_shell.c" = "";
    "test_mandelbrot.c" = "";
    "test_http.c" = "";
    "test_math_interpreter.c" = "";
}

$expected_values = @{
    "test_entry_point.c" = 9;
    "test_hello_world.c" = 12;
    "test_test_server.c" = 0;
    "test_execve.c" = 0;
    "test_syscall.c" = 0;
    "test_shell.c" = 0;
    "test_mandelbrot.c" = 0;
    "test_http.c" = 0;
    "test_math_interpreter.c" = 0;
} 