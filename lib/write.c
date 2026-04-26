#include <unistd.h>

// Enveloppe utilisateur de write : 2 args -> syscall2.
syscall2(int, write, const char *, s, int, len)