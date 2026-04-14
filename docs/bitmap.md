# Génération d'```image.h```

Pour utiliser les bitmaps dans le code, il faut convertir les ```.bmp``` en un ```.h```. Pour celà, on utilise un script python dévelopé pour l'occasion : ```BmpToHConvertor.py```. Il permet de composer un header comportant autant de bitmaps que voulus.

## Utilisation

L'usage du script est simple, ainsi pour créer notre ```image.h```, nous avons utilisé la commande suivante :
``` bash
python3 BmpToHConvertor.py --HEADER_NAME Core/Inc/images --INPUT_FILE asset_brut/Coeur_E.bmp asset_brut/Coeur_F.bmp asset_brut/Joueur.bmp asset_brut/Ennemi_1.bmp asset_brut/Ennemi_2.bmp asset_brut/Ennemi_3.bmp asset_brut/Ennemi_4.bmp --VAR_NAMES tex_coeur_E tex_coeur_F tex_joueur tex_ennemi_1 tex_ennemi_2 tex_ennemi_3 tex_ennemi_4
```
Bien qu'elle soit longue, elle ne fait que lister les fichiers d'entrée et le nom des variables de sortie.

## Contrainte sur les images

Pour que les images soit compatibles, il faut que elles soient enregistrées avec un codage ```R5G6B5``` sur 16 bits. De plus, il ne faut pas utiliser de pallette de couleur.

!!! info
    Il faut absolument que l'image ait une largeur multiple de 4, sinon un bug d'affichage déformant l'image se produit.