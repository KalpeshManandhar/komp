.global socket
socket:
    li a7, 198         # syscall number for socket
    ecall
    ret

.global bind
bind:
    li a7, 200         # syscall number for bind
    ecall
    ret

.global listen
listen:
    li a7, 201         # syscall number for listen
    ecall
    ret

.global connect
connect:
    li a7, 203         # syscall number for listen
    ecall
    ret

.global accept
accept:
    li a7, 202         # syscall number for accept
    ecall
    ret

.global send
send:
    li a7, 206         # syscall number for sendto (send is alias)
    mv a4, zero
    mv a5, zero
    ecall
    ret
.global recv
recv:
    li a7, 207         # syscall number for sendto (send is alias)
    mv a4, zero
    mv a5, zero
    ecall
    ret

