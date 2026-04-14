# Les threads

## Description des threads :

### ```GameMaster```
Ce thread permet de gérer la fin des niveaux, c'est-à-dire que faire lorsque il n'y a plus de mostres ou plus de vie pour le joueur. Le thread est bloqué tant qu'il ne reçoit pas de message par ```queue_F```.

Ce thread est le plus important, car sans ses actions, le jeu ne peut pas avancer, il a donc la plus grande priorité

### ```Joueur_1```
Ce tread gère les déplacements et l'affichage du joueur ainsi que ses tirs.

Ce thread est important à la fluidité du jeu, il a donc la deuxième plus grande priorité

### ```Block_Enemie```
Ce thread gère le block d'ennemis, leurs vies, et leurs tirs. Pour chaque pas de temps, chaque monstre a une probabilité de tirer de $\frac{\text{numéro de la vague}}{15}$. Ainsi, à chaque vague le jeu devient plus dur.

Ce thread a une priorité normale.
### ```Projectile```
Ce thread gère les missiles. Il reçoit les demandes de création de nouveaux missiles par la ```queue_N```, et si il n'y a pas trop de missiles déjà en jeu (moins de $200$), il le rajoute et commence à le simuler. C'est ce thread qui gère toutes les collisions. En cas de collision détectée, il tue le missile incriminé et envoie un messega via la ```queue_J``` quand le choc est avec le joueur, et la ```queue_E``` lorsque le choc implique un monstre.

Ce thread a une priorité normale.

### ```HUD```
Ce thread gère l'affichage de la bar d'informations en haut de l'écran, il ne fait que lire des variables partagées.

Ce thread a une priorité basse.

### ```chargeur```
Ce thread sert a incrémenter régulièrement le compteur représentant la charge du tir secondaire.

Ce thread a une priorité basse.

## Communication entre les threads :

### ```Queue_F```
On envoie un objet ```enum End_Type```, cela permet de débloquer le thread ```GameMaster```.

### ```Queue_N```
On envoie un objet ```struct Missile```, cela permet de demander la création d'un nouveau missile.

### ```Queue_J```
On envoie un ```int8_t``` qui represente les dégats à appliquer au joueur (on peut imaginer des dégats négatifs pour des soins).

### ```Queue_E```
On envoie un objet ```struct collision``` qui comporte le numéro du monstre et les dégats à lui appliquer.