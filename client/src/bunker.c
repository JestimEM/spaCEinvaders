#include "../include/bunker.h"

void bunker_inicializar(Bunker *b, int id, int x, int y) {
    b->id               = id;
    b->x                = x;
    b->y                = y;
    b->porcentaje_salud = 100;
}

void bunker_establecer_salud(Bunker *b, int porcentaje) {
    b->porcentaje_salud = porcentaje;
    if (b->porcentaje_salud < 0)   b->porcentaje_salud = 0;
    if (b->porcentaje_salud > 100) b->porcentaje_salud = 100;
}

void bunker_daniar(Bunker *b, int cantidad) {
    b->porcentaje_salud -= cantidad;
    if (b->porcentaje_salud < 0) b->porcentaje_salud = 0;
}

int bunker_esta_destruido(const Bunker *b) {
    return b->porcentaje_salud <= 0;
}
