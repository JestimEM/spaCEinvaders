#include "bunker_visual.h"
#include <string.h>
#include <math.h>
#include "raylib.h"

/* -----------------------------------------------------------------------
 * Forma clásica Space Invaders: arco superior + abertura inferior central
 * 18 columnas × 12 filas
 * ----------------------------------------------------------------------- */
const uint8_t BV_FORMA[BV_FILAS][BV_COLS] = {
    {0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0},
    {0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0},
    {0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0},
    {0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,0,0,0,0,0,0,1,1,1,1,1,1},
    {1,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,1},
    {1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1},
    {1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1},
};

/* -----------------------------------------------------------------------
 * Array global de bunkers visuales
 * ----------------------------------------------------------------------- */
BunkerVisual g_bunkers_visual[NUM_BUNKERS];

/* -----------------------------------------------------------------------
 * bv_inicializar_todos
 * Copia BV_FORMA en cada bunker y lo marca como intacto (salud = 100).
 * ----------------------------------------------------------------------- */
void bv_inicializar_todos(void) {
    int i, f, c;
    for (i = 0; i < NUM_BUNKERS; i++) {
        BunkerVisual *bv = &g_bunkers_visual[i];
        for (f = 0; f < BV_FILAS; f++)
            for (c = 0; c < BV_COLS; c++)
                bv->pixeles[f][c] = BV_FORMA[f][c];
        bv->salud_anterior = 100;
        bv->inicializado   = 1;
    }
}

/* -----------------------------------------------------------------------
 * bv_destruir_patron_borde  (función estática interna)
 * Destruye `cantidad` píxeles vivos priorizando los más alejados del
 * centro geométrico (erosión de bordes con pequeño ruido aleatorio).
 * Usa bubble sort sobre un array en stack (216 celdas como máximo).
 * ----------------------------------------------------------------------- */
static void bv_destruir_patron_borde(BunkerVisual *bv, int cantidad) {
    typedef struct { int f; int c; float dist; } Celda;
    Celda candidatas[BV_COLS * BV_FILAS];
    int n = 0;
    int i, j;
    float cx = BV_COLS / 2.0f;
    float cy = BV_FILAS / 2.0f;

    for (i = 0; i < BV_FILAS; i++) {
        for (j = 0; j < BV_COLS; j++) {
            if (BV_FORMA[i][j] && bv->pixeles[i][j]) {
                float dx   = (float)j - cx;
                float dy   = (float)i - cy;
                /* Ruido fraccionario para evitar erosión perfectamente circular */
                float ruido = (float)GetRandomValue(-8, 8) / 10.0f;
                candidatas[n].f    = i;
                candidatas[n].c    = j;
                candidatas[n].dist = sqrtf(dx * dx + dy * dy) + ruido;
                n++;
            }
        }
    }

    /* Bubble sort descendente por dist (n <= 216, rendimiento aceptable) */
    for (i = 0; i < n - 1; i++) {
        for (j = i + 1; j < n; j++) {
            if (candidatas[j].dist > candidatas[i].dist) {
                Celda tmp     = candidatas[i];
                candidatas[i] = candidatas[j];
                candidatas[j] = tmp;
            }
        }
    }

    /* Destruir los primeros `cantidad` píxeles candidatos */
    for (i = 0; i < n && cantidad > 0; i++, cantidad--) {
        bv->pixeles[candidatas[i].f][candidatas[i].c] = 0;
    }
}

/* -----------------------------------------------------------------------
 * bv_sincronizar
 * Ajusta el estado visual del bunker `idx` al `porcentaje_nuevo` [0-100]
 * recibido desde el servidor.
 *
 * - Si la salud subió o el bunker no estaba inicializado (nueva ronda),
 *   se restaura la forma completa antes de aplicar el daño.
 * - El daño se erosiona desde los bordes hacia el centro.
 * ----------------------------------------------------------------------- */
void bv_sincronizar(int idx, int porcentaje_nuevo) {
    int f, c;
    BunkerVisual *bv = &g_bunkers_visual[idx];

    /* Restaurar si es nueva ronda (salud subió) o primer uso */
    if (!bv->inicializado || porcentaje_nuevo > bv->salud_anterior) {
        for (f = 0; f < BV_FILAS; f++)
            for (c = 0; c < BV_COLS; c++)
                bv->pixeles[f][c] = BV_FORMA[f][c];
        bv->salud_anterior = 100;
        bv->inicializado   = 1;
    }

    /* Total de píxeles en la forma base */
    int total = 0;
    for (f = 0; f < BV_FILAS; f++)
        for (c = 0; c < BV_COLS; c++)
            total += (int)BV_FORMA[f][c];

    /* Cuántos píxeles deben estar destruidos según el porcentaje recibido */
    int destruir_objetivo = total - (total * porcentaje_nuevo / 100);

    /* Cuántos píxeles de la forma ya fueron destruidos */
    int ya_destruidos = 0;
    for (f = 0; f < BV_FILAS; f++)
        for (c = 0; c < BV_COLS; c++)
            if (BV_FORMA[f][c] && !bv->pixeles[f][c])
                ya_destruidos++;

    int a_destruir = destruir_objetivo - ya_destruidos;
    if (a_destruir > 0) {
        bv_destruir_patron_borde(bv, a_destruir);
    }

    bv->salud_anterior = porcentaje_nuevo;
}

/* -----------------------------------------------------------------------
 * bv_dibujar
 * Dibuja el bunker `idx` en píxeles de pantalla a partir de (sx, sy).
 * Cada celda de la grilla ocupa BV_PX_SIZE × BV_PX_SIZE píxeles.
 * ----------------------------------------------------------------------- */
void bv_dibujar(int idx, int sx, int sy, Color col) {
    int f, c;
    BunkerVisual *bv = &g_bunkers_visual[idx];
    for (f = 0; f < BV_FILAS; f++) {
        for (c = 0; c < BV_COLS; c++) {
            if (!bv->pixeles[f][c]) continue;
            DrawRectangle(
                sx + c * BV_PX_SIZE,
                sy + f * BV_PX_SIZE,
                BV_PX_SIZE, BV_PX_SIZE,
                col
            );
        }
    }
}
