.text
# Stub appelé sur IRQ 1 (clavier).
    .globl handler_keyboard
handler_keyboard:
    pushl %eax
    pushl %edx
    pushl %ecx
    call keyboard_handler_C
    popl %ecx
    popl %edx
    popl %eax
    iret