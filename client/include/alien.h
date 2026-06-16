#ifndef EXTRATERRESTRE_H
#define EXTRATERRESTRE_H

#include "constants.h"

typedef struct {
    int    id;
    int    x;
    int    y;
    int    puntos;
    int    tipo;         /* TIPO_CALAMAR | TIPO_CANGREJO | TIPO_PULPO | TIPO_OVNI */
    int    vivo;
    int    golpe_visual; /* 1 = colision predictiva del cliente, espera confirm del servidor */
    HitBox hitbox;       /* coords de pantalla, actualizado cada frame */
} Extraterrestre;

typedef struct {
    int    activo;
    int    x;
    int    direccion;    /* 0 = IZQ→DER, 1 = DER→IZQ */
    int    puntos;
    HitBox hitbox;
} Ovni;

/* Prototipos */
void extraterrestre_inicializar(Extraterrestre *e, int id, int x, int y, int tipo, int puntos);
void extraterrestre_destruir(Extraterrestre *e);
int  extraterrestre_esta_vivo(const Extraterrestre *e);
void ovni_inicializar(Ovni *ovni);
void ovni_aparecer(Ovni *ovni, int direccion, int puntos);
void ovni_desactivar(Ovni *ovni);

#endif /* EXTRATERRESTRE_H */
