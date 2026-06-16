#ifndef JUGADOR_H
#define JUGADOR_H

#include "constants.h"

typedef struct {
    int    vidas;
    int    puntuacion;
    int    posicion_canon_x;
    int    posicion_canon_y;
    int    disparando;
    HitBox hitbox;    /* coords de pantalla, actualizado cada frame */
} Jugador;

/* Prototipos */
void jugador_inicializar(Jugador *j);
int  jugador_esta_vivo(const Jugador *j);

#endif /* JUGADOR_H */
