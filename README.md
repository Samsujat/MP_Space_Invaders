# MP Space Invaders

## Présentation

Ce projet est une adaptation du jeu **Space Invaders** développée sur carte **STM32**.  
Le jeu utilise l’écran LCD tactile de la carte, un joystick, des boutons physiques, ainsi que plusieurs tâches FreeRTOS pour gérer les différents éléments du jeu.

L’objectif du projet est de mettre en œuvre un jeu embarqué complet en langage C, avec affichage graphique, interactions utilisateur, gestion des collisions, ennemis, projectiles et score.

## Fonctionnalités

- Écran titre avec bouton **Jouer**
- Déplacement du joueur avec le joystick
- Tir simple avec bouton
- Tir spécial après chargement
- Affichage des vies du joueur
- Vagues d’ennemis
- Déplacement automatique des ennemis
- Projectiles ennemis et alliés
- Détection des collisions
- Écran **Game Over**
- Écran des **5 meilleurs scores**
- Gestion multitâche avec **FreeRTOS**


## Compilation et exécution

Le projet est prévu pour être ouvert avec **STM32CubeIDE**.

Cloner le dépôt :
   ```bash
   git clone https://github.com/Samsujat/MP_Space_Invaders.git
  ```
## Améliorations possibles

Plusieurs améliorations pourraient être ajoutées au projet :

- Sauvegarde permanente des meilleurs scores, par exemple dans la mémoire Flash interne ou sur une carte SD
- Création de nouveaux types d’ennemis avec des comportements différents
- Ajout d’effets sonores pour les tirs, collisions et fins de partie
- Ajout d’un mode multijoueur ou d’un classement plus complet
