#include <unistd.h>

// Enveloppe utilisateur de read : 2 args -> syscall2.
syscall2(int, read, char *, s, int, len)