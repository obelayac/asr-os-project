.text
# Stub appelé sur IRQ 0 (timer).
    .globl handler_timer
handler_timer:
    pushl %eax
    pushl %edx
    pushl %ecx
    call tick_handler_C
    popl %ecx
    popl %edx
    popl %eax
    iret