# L'écran

## Organisation de l'écran
Les dix pixels en haut de l'écran sont reservés pour l'affichage des informations.

Les monstres sont rangés sur trois rangs

## Mutex
Pour ne pas avoir de problème d'interruption entre les threads pendant la manipulation de la mémoire de l'écran, on utilise un mutex ainsi qu'une série de fonctions permettant de faire automatiquement l'attente et le relachement du mutex.

## Bitmap
Toutes les textures sont stokées dans ```image.h```. Les textures des coeurs et du joueur sont utilisées directement, alors que pour les 4 textures des monstres, on utilise une liste de pointeurs vers les textures. Ainsi, on peut changer rapidement la texture utilisée par les monstres pour avoir des sortes d'animation.
La génération des bitmaps est abordée dans une [partie dédiée](bitmap.md).
