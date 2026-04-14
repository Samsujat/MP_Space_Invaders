# Génerateur de nombre aléatoire

## Utilisation du pérriphérique :
On utilise le code suivant, car il n'y a pas toujours de nouveau nombre aléatoire disponible dans le buffer.
``` C
  uint32_t nombre_aleatoire;
  while (HAL_RNG_GenerateRandomNumber(&hrng, &nombre_aleatoire) != HAL_OK)
    ;
```

## Fonctions utilitaires

On définit deux fonctions pour la suite :

- ```uint8_t proba_bernoulli(uint32_t numerateur, uint32_t denominateur)``` qui revoie ```1``` avec une probabilité de $\frac{\text{numerateur}}{\text{denominateur}}$, ```0``` sinon,

- ```uint8_t proba_tirrage(uint8_t nombre_valeur)``` qui renvoie un nombre entre $1$ et $\text{nombre_valeur}$ avec une probabilité uniforme, utile pour tirer un objet au hasard dans un liste.
