#include "../include/player.h"

void jugador_inicializar(Jugador *j) {
    j->vidas            = VIDAS_INICIALES;
    j->puntuacion       = 0;
    j->posicion_canon_x = 50;
    j->posicion_canon_y = 90;
    j->disparando       = 0;
}

int jugador_esta_vivo(const Jugador *j) {
    return j->vidas > 0;
}
