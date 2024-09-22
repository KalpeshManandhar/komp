#pragma once


/*
Registers in RISC-V

Bits	Int     Reg	Name	    Detail	
00000	x0	    zero            Hard-wired zero	
00001	x1	    ra	            Return address Caller	May be changed by sub
Used for function return with jalr x0, x1, 0
00010	x2	    sp	            Stack pointer Callee	if changed by Sub must be backed up
00011	x3	    gp	            Global pointer	
00100	x4	    tp	            Thread pointer	
00101	x5	    t0	            Temporary/alternate link register Caller	May be changed by sub
00110	x6	    t1	            Temporaries Caller	May be changed by sub
00111	x7	    t2	            Temporaries Caller	May be changed by sub
01000	x8	    s0              / fp	Saved register/frame pointer Callee	if changed by Sub must be backed up
01001	x9	    s1	            Saved register Callee	if changed by Sub must be backed up
01010	x10	    a0	            Function arguments/return values Caller	Use to pass to functions - May be changed by sub
01011	x11	    a1	            Function arguments/return values Caller	Use to pass to functions - May be changed by sub
01100	x12	    a2	            Function arguments Caller	Use to pass to functions - May be changed by sub
01101	x13	    a3	            Function arguments Caller	Use to pass to functions - May be changed by sub
01110	x14	    a4	            Function arguments Caller	Use to pass to functions - May be changed by sub
01111	x15	    a5	            Function arguments Caller	Use to pass to functions - May be changed by sub
10000	x16	    a6	            Function arguments Caller	Use to pass to functions - May be changed by sub
10001	x17	    a7	            Function arguments Caller	Use to pass to functions - May be changed by sub
10010	x18	    s2	            Saved registers Callee	if changed by Sub must be backed up
10011	x19	    s3	            Saved registers Callee	if changed by Sub must be backed up
10100	x20	    s4	            Saved registers Callee	if changed by Sub must be backed up
10101	x21	    s5	            Saved registers Callee	if changed by Sub must be backed up
10110	x22	    s6	            Saved registers Callee	if changed by Sub must be backed up
10111	x23	    s7	            Saved registers Callee	if changed by Sub must be backed up
11000	x24	    s8	            Saved registers Callee	if changed by Sub must be backed up
11001	x25	    s9	            Saved registers Callee	if changed by Sub must be backed up
11010	x26	    s10	            Saved registers Callee	if changed by Sub must be backed up
11011	x27	    s11            Saved registers Callee	if changed by Sub must be backed up
11100	x28	    t3	            Temporaries Caller	May be changed by sub
11101	x29	    t4	            Temporaries Caller	May be changed by sub
11110	x30	    t5	            Temporaries Caller	May be changed by sub
11111	x31	    t6	            Temporaries Caller	May be changed by sub
*/

/*
Addressing modes in RISC-V

Immediate Addressing:
    A fixed number is the parameter for an operation.
    Example:
        LI    a7, 12         // Load immediate value 12 into a7
        ADDI  a0, a0, -7     // Add immediate -7 to a0 and store in a0
        LI    a2, 255        // Load immediate value 255 into a2

Register Addressing:
    A register itself is used as a source or destination of an operation.
    Example:
        OR    a1, a1, a0     // Perform bitwise OR between a1 and a0, store result in a1
        MV    a3, a1         // Move value from a1 to a3

Register Indirect with Offset Addressing:
    The value from the address in a register, offset by a fixed numeric value.
    Example:
        LW    a1, 4(a0)      // Load word from memory at address (a0 + 4) into a1
        LW    a1, 0(a0)      // Load word from memory at address (a0) into a1

Program Counter Relative with Offset Addressing:
    Used by the AUIPC (Add Upper Immediate to Program Counter) command, and for relative jump and branch operations.
    Example:
        BEQ   a0, a1, TestLabel  // Branch to TestLabel if a0 equals a1
        AUIPC a0, 0xFF           // Add upper immediate 0xFF to the program counter, store in a0
*/



