#ifndef SPRITES_H
#define SPRITES_H

#include "raylib.h"
#include "constants.h"

/* Tipo de funcion de dibujo para un alien — Strategy Pattern */
typedef void (*AlienRenderFn)(int cx, int cy, float escala, Color col);

/* Seleccion de estrategia */
AlienRenderFn sprites_obtener_render_alien(int tipo);
void          sprites_dibujar_alien(int tipo, int cx, int cy, float escala, Color col);

/* Sprites individuales publicos */
void sprites_dibujar_ovni(int cx, int cy, Color col);
void sprites_dibujar_canon(int cx, int cy, Color col);
void sprites_dibujar_bala_jugador(int cx, int cy);
void sprites_dibujar_bala_alien(int cx, int cy);

/* Flash del canon (estado estatico) */
void sprites_canon_flash(void);
void sprites_canon_tick(void);
int  sprites_canon_esta_en_flash(void);

/* Animacion de dos fotogramas para aliens (llamar una vez por render frame) */
void sprites_alien_tick(void);

#endif /* SPRITES_H */
