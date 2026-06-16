#ifndef RENDER_H
#define RENDER_H

#include "game_state.h"

void render_inicializar(void);
void render_cargar_texturas(void); /* llamar después del menú, justo antes del game loop */
void render_destruir(void);

/* render_frame actualiza hitboxes como efecto secundario — no-const.
   es_observador=1 reemplaza la línea de controles por "OBSERVADOR" + id del jugador visto. */
void render_frame(EstadoJuego *ej, int es_observador, int id_jugador_observado);

/* Pantallas de navegación — devuelven la acción del usuario */
int  render_menu(void);                          /* 0=Jugador  1=Observador  -1=salir */
int  render_lista_sesiones(int *ids, int n);     /* id seleccionado, o -1=salir */
int  render_pantalla_fin(int puntuacion, int es_victoria); /* 0=Salir  1=Reiniciar */

void render_mensaje_overlay(const char *msg);

#endif /* RENDER_H */
