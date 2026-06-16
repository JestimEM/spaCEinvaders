#ifndef ESTADO_JUEGO_H
#define ESTADO_JUEGO_H

#include "alien.h"
#include "player.h"
#include "bunker.h"

/* Bala alien recibida del servidor */
typedef struct {
    int x, y;
    int activa;
} BalaAlien;

typedef struct {
    /* Extraterrestres */
    Extraterrestre extraterrestres[MAX_EXTRATERR];
    int            cantidad_extraterrestres;

    /* Jugador propio */
    Jugador        jugador;

    /* OVNI */
    Ovni           ovni;

    /* Bunkers */
    Bunker         bunkers[NUM_BUNKERS];

    /* Estado del juego */
    float          velocidad_extraterrestre;
    int            ronda;
    int            fin_juego;
    int            ronda_completa; /* 1 en RONDA_COMPLETA, 0 en siguiente ESTADO_JUEGO */

    /* Bala del jugador (server-side) */
    int            bala_activa;
    int            bala_x;
    int            bala_y;

    /* Balas alienígenas */
    BalaAlien      balas_alien[MAX_BALAS_ALIEN];
    int            cantidad_balas_alien;
} EstadoJuego;

/* Prototipos */
void estado_juego_inicializar(EstadoJuego *ej);
void estado_juego_reiniciar_ronda(EstadoJuego *ej);

#endif /* ESTADO_JUEGO_H */
