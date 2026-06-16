#ifndef BUNKER_VISUAL_H
#define BUNKER_VISUAL_H
#include <stdint.h>
#include "raylib.h"
#include "constants.h"

#define BV_COLS    18
#define BV_FILAS   12
#define BV_PX_SIZE  2

extern const uint8_t BV_FORMA[BV_FILAS][BV_COLS];

typedef struct {
    uint8_t pixeles[BV_FILAS][BV_COLS];
    int     salud_anterior;
    int     inicializado;
} BunkerVisual;

extern BunkerVisual g_bunkers_visual[NUM_BUNKERS];

void bv_inicializar_todos(void);
void bv_sincronizar(int idx, int porcentaje_nuevo);
void bv_dibujar(int idx, int sx, int sy, Color col);

#endif /* BUNKER_VISUAL_H */
