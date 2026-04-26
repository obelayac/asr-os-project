# Projet n7OS

**Nom :** BEL AYACHI Oussama  
**Promotion :** ENSEEIHT 3SN (projet en dette de la 2e année)

Pour voir ce qui a été fait, comment tester, etc. : [voir la section dédiée](#ce-qui-a-été-fait).

---

# Projet développement d'un mini système d'exploitation pour PC x86

## Objectifs

Développer les éléments de base d'un système d'exploitation

Ce que nous verrons :
- Gestion d'entrées/sorties de base : le clavier et l'écran
- Gestion des interruptions
- Gestion des processus
- Gestion de la mémoire virtuelle pour les processus

Ce que nous ne verrons pas :
- Gestion des fichiers
- Partage de ressources et communication entre processus
  
## Organisation

- 14 séances encadrées
- Pas de cours
- Programmation en C, un peu d'assembleur
- Evaluation : code commenté
  
## Au menu

### Entrée
- De l'affichage à la console
- S'il vous plaît ? Je peux vous interrompre ?
- Il y a des manières, monsieur ! Utilisez l'appel système !

> - Mise en oeuvre de la console
> - Appel système write

### Le plat
- Tic Tac Tic Tac, respectez le Timer !
- Des processus ? Comment tu définis ça ?
- Alors toi, tu crées des processus et tu les détruis.
- Il faudrait organiser tout ce beau monde, non ?
  - Et hop ! Tout le monde en file !
  - Laissez un peu la place aux autres ! Revenez dans la file ! Respectez le tourniquet !

> - Interruption Timer
> - Ordonnancement et gestions des processus

### Le dessert
- C'est bien fichu ici : tu peux commander depuis la table avec un clavier.

> - Lecture au clavier et appel système read
> - Interpréteur de commandes simple

### Avec ta fourchette !

Il nous faut des couverts !

- Compilation : GCC 
  
````sudo apt-get install build-essentials```
- Exécution : QEMU
   
```sudo apt-get install qemu```
- Mise au point : GDB
    
    - ```sudo apt-get install gdb```
    - GDB sera connecté à QEMU et permet d'afficher les problèmes potentiels

> [!NOTE]
> Les commandes données sont pour environnement Debian et dérivés (j'utilise Ubuntu). A adapter en fonction de votre distribution Linux.   
> Sous Mac : Utiliser les outils `gcc` pour x86 disponible via les `macports` (paquet `i386-elf-gcc`).

## Les amuses-bouches

### Fichiers fournis 

Le répertoire fourni contient :
- `/boot` 
  - répertoire d'entrée du système ;
  - `crt0.S` initialise le matériel et lance le programme principal du système (`kernel_start`)
- `/kernel`
  - répertoire sources du noyau
  - c'est ici que tout (ou presque) va se passer
- `/lib`
  - quelques outils utiles (par ex.: `printf`)
- `/include`
  - pour les `.h` c'est ici

### Prise en main de l'environnement

- La compilation s'effectue via la commande `make`
  - si tout va bien, résultat : `kernel.bin`
- Exécution : `make run`
  - Une fenêtre QEMU doit apparaître
  - Le système est exécuté
- Mise au point 
  - Lancer : `make dbg`
  - Mettre un point d'arrêt au début du système : `b kernel_start`
  - Lancer l'exécution : `cont` ou `r`
  - Afficher un variable : `display` nom de la variable
  - `n`: Next, `s` : Step

---

## Ce qui a été fait

Toutes les étapes du menu ont été implémentées, plus la pagination. Au démarrage, le noyau initialise console, gestion mémoire, pagination, IT, syscalls, timer et clavier, puis lance trois processus : `idle` (pid 0), `processus1` (muet par défaut, activé via la demo) et `shell` (pid 2).

Une fois `make run` lancé, on tombe sur le prompt `n7OS>` du shell. La commande `demo` ouvre un menu permettant de tester chaque étape individuellement.

### Liste des étapes

- **Console** (`kernel/console.c`)
  - Mémoire vidéo VGA en `0xB8000`, curseur géré via les ports `0x3D4` / `0x3D5`.
  - Caractères de contrôle : `\b`, `\t`, `\n`, `\f`, `\r`.
  - Wrap automatique en bout de ligne et en bout d'écran (retour ligne 0).
  - À chaque saut de ligne, la nouvelle ligne est effacée pour éviter les résidus du tour précédent.

- **Interruption logicielle** (`kernel/test_irq.c`, `kernel/handler_IT.S`, `kernel/irq.c`)
  - Mise en place d'une interrupt gate sur l'entrée 50 de l'IDT (DPL=0).
  - Test : `__asm__("int $50")` doit déclencher `handler_en_C` qui affiche un message.

- **Appel système write** (`kernel/sys.c`, `kernel/handler_syscall.S`, `lib/write.c`)
  - Vecteur 0x80 en interrupt gate DPL=3 pour autoriser l'appel depuis l'utilisateur.
  - `printf` passe désormais par `write` : tout affichage transite par `int $0x80`.

- **Timer** (`kernel/time.c`, `kernel/handler_timer.S`)
  - PIT 8253 programmé à 1 kHz (canal 0, mode 2).
  - Compteur de ticks, affichage `HH:MM:SS` en haut à droite (jaune sur noir), réécrit directement en mémoire vidéo.

- **Processus et ordonnancement** (`kernel/process.c`, `kernel/ctx_sw.S`)
  - Table statique de 16 slots (idle + processus1 + shell + 13 libres).
  - Pile statique de 1024 octets par processus, sauf idle qui réutilise la pile de boot.
  - Ordonnancement round-robin (parcours de la table à partir du courant), préemption toutes les 50 ms via `tick_handler_C`.
  - Petit trampoline `process_starter` qui fait un `sti` avant d'appeler la vraie fonction (sinon les nouveaux processus démarrent avec IF=0).

- **Clavier** (`kernel/keyboard.c`, `kernel/handler_keyboard.S`)
  - IRQ 1, scancode lu sur le port `0x60`.
  - Conversion via les tables AZERTY fournies (Shift géré).
  - Buffer circulaire de 64 caractères, `kgetch()` bloquant et `kbd_has_input()` non bloquant.

- **Appel système read et shell** (`kernel/sys.c`, `lib/read.c`, `bin/shell.c`)
  - `read(buf, len)` lit jusqu'à `len-1` caractères, s'arrête sur Entrée, gère le backspace, écho à l'écran.
  - Shell minimal `n7sh` avec les commandes `help`, `clear`, `time`, `echo X`, `demo`.

- **Pagination** (`kernel/paging.c`, `kernel/mem.c`, `kernel/kheap.c`)
  - Identity-mapping des 16 premiers Mo : chaque adresse virtuelle pointe sur la même adresse physique. Permet d'activer la pagination sans déplacer le code, les piles ou les structures système déjà en place.
  - Répertoire de pages + 4 tables de pages alloués via `kmalloc_a`, chargés dans CR3, bit PG de CR0 mis à 1.
  - Allocateur bitmap pour les pages physiques (`mem.c`) : 1 bit par page, soit 128 mots de 32 bits pour 16 Mo.
  - `alloc_page_entry()` mappe une nouvelle page physique libre à une adresse virtuelle donnée.

### La commande `demo`

Tapée depuis le prompt `n7OS>`, elle ouvre un menu numéroté. Chaque entrée déclenche un test ciblé sur la milestone correspondante :

| Choix | Test | Ce qu'on observe |
|-------|------|------------------|
| 1 | Console | `printf` avec `\t`, `\r`, `\b`, plus le résultat de `fibonacci(5)` (5). |
| 2 | Interruption logicielle | `int $50` déclenche le handler, on lit "Interruption recue !" entre deux messages. |
| 3 | Appel système example | `example()` retourne 1, message "OK" affiché. |
| 4 | Timer | Texte explicatif ; l'horloge en haut à droite est la preuve visible. |
| 5 | Processus et ordonnancement | Affiche la table des processus, active processus1 qui imprime `P1 iter N` en parallèle, on tape `q` pour stopper. |
| 6 | Clavier | Lit 3 caractères et affiche leur code ASCII. |
| 7 | Appel système read | Lit une phrase entière et la réaffiche, en montrant le nombre de caractères lus. |
| 8 | Pagination | Affiche l'état des registres CR0 et CR3, et l'occupation du bitmap mémoire physique. |
| q | Quitter | Retour au prompt `n7OS>`. |
