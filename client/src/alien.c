#include "../include/alien.h"
#include <string.h>

void extraterrestre_inicializar(Extraterrestre *e, int id, int x, int y, int tipo, int puntos) {
    e->id     = id;
    e->x      = x;
    e->y      = y;
    e->tipo   = tipo;
    e->puntos = puntos;
    e->vivo   = 1;
}

void extraterrestre_destruir(Extraterrestre *e) {
    e->vivo = 0;
}

int extraterrestre_esta_vivo(const Extraterrestre *e) {
    return e->vivo;
}

void ovni_inicializar(Ovni *o) {
    o->activo    = 0;
    o->x         = 0;
    o->direccion = 0;
    o->puntos    = 0;
}

void ovni_aparecer(Ovni *o, int direccion, int puntos) {
    o->activo    = 1;
    o->direccion = direccion;
    o->puntos    = puntos;
    o->x         = (direccion == 0) ? 0 : 100;
}

void ovni_desactivar(Ovni *o) {
    o->activo = 0;
}
