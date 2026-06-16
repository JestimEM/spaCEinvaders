#ifndef BUNKER_H
#define BUNKER_H

#include "constants.h"

typedef struct {
    int    id;
    int    porcentaje_salud; /* 100, 75, 50, 25, 0 */
    int    x;
    int    y;
    HitBox hitbox;           /* coords de pantalla, actualizado cada frame */
} Bunker;

/* Prototipos */
void bunker_inicializar(Bunker *b, int id, int x, int y);
void bunker_establecer_salud(Bunker *b, int porcentaje);
void bunker_daniar(Bunker *b, int cantidad);
int  bunker_esta_destruido(const Bunker *b);

#endif /* BUNKER_H */
