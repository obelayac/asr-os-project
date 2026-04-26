#include <n7OS/sys.h>
#include <n7OS/syscall_defs.h>
#include <n7OS/console.h>
#include <n7OS/irq.h>
#include <n7OS/keyboard.h>
#include <unistd.h>

extern void handler_syscall();

void init_syscall() {
    // Enregistrement des fonctions sys_xxx dans la table.
    add_syscall(NR_example, sys_example);
    add_syscall(NR_write,   (fn_ptr) sys_write);
    add_syscall(NR_read,    (fn_ptr) sys_read);

    // Installation du traitant d'IT soft sur le vecteur 0x80.
    init_irq_entry(0x80, (uint32_t) handler_syscall);
}

// Appel système de test : retourne juste 1.
int sys_example() {
    return 1;
}

// Affiche len caractères de s sur la console. Retourne len.
int sys_write(const char *s, int len) {
    console_putbytes(s, len);
    return len;
}

/*
 * Lit jusqu'à len-1 caractères depuis le clavier et les dépose dans
 * s, puis ajoute un '\0'. S'arrête sur Entrée (qui n'est pas mis
 * dans s) ou quand len-1 est atteint. Echo chaque caractère reçu
 * pour que l'utilisateur voie ce qu'il tape, gère le backspace en
 * effaçant visuellement le dernier caractère ("\b \b").
 */
int sys_read(char *s, int len) {
    int count = 0;

    while (count < len - 1) {
        char c = kgetch();

        if (c == '\r' || c == '\n') {
            console_putbytes("\n", 1);
            break;
        }

        if (c == '\b') {
            if (count > 0) {
                count--;
                console_putbytes("\b \b", 3);
            }
            continue;
        }

        s[count++] = c;
        console_putbytes(&c, 1);
    }

    s[count] = '\0';
    return count;
}