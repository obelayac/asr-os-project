#ifndef __SYSCALL_DEFS_H__
#define __SYSCALL_DEFS_H__

#define NB_SYSCALL 3

int sys_example();

// Affiche len caractères de s sur la console.
int sys_write(const char *s, int len);

// Lit jusqu'à len-1 caractères du clavier dans s. S'arrête sur
// Entrée. Echo chaque caractère lu.
int sys_read(char *s, int len);

typedef int (*fn_ptr)();
extern fn_ptr syscall_table[NB_SYSCALL];

void add_syscall(int num, fn_ptr function);

#endif