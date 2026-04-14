# LED et GPIO

## Ecriture sur les LEDs

Pour faciliter l'utilisation des LEDs, on utilise une liste d'objets ```struct led```, ainsi ```Leds[n] = {{LEDXX_GPIO_Port, LEDXX_Pin}}```.

On peut ainsi utiliser la ligne ```HAL_GPIO_WritePin(Leds[idx].port, Leds[idx].pin, !(charge-1<idx));``` ce qui simplifie l'itération sur toutes les leds.

## Lecture sur les boutons
On lit aussi l'état des boutons ```BP1``` et ```BP2``` via les GPIO.