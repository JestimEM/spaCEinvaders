#ifndef EFECTOS_H
#define EFECTOS_H

#include "raylib.h"
#include "alien.h"   /* Extraterrestre struct */

#define MAX_PARTICULAS    128
#define PARTICULAS_X_EXP   12
#define VIDA_PARTICULA    0.5f

typedef struct {
    float x, y;
    float vx, vy;
    float vida;   /* segundos restantes; <= 0 = slot libre */
    Color col;
} Particula;

/* Limpia todas las partículas activas y el registro de IDs muertos */
void efectos_limpiar(void);

/* Emite PARTICULAS_X_EXP partículas desde (px, py) en coordenadas de pantalla */
void efectos_emitir_explosion(float px, float py, Color col_base);

/* Avanza la simulación de todas las partículas activas */
void efectos_actualizar(float dt);

/* Dibuja todas las partículas activas */
void efectos_dibujar(void);

/*
 * Registra la muerte de un alien e inicia su explosión.
 * Si el alien ya fue registrado (mismo id), la llamada es un no-op,
 * evitando explosiones duplicadas en ticks consecutivos.
 * px, py son coordenadas de pantalla (píxeles).
 */
void efectos_registrar_muerte_alien(int id, float px, float py, Color col);

/*
 * Frame-diff de aliens: compara el array actual contra el snapshot
 * del frame anterior almacenado internamente.  Para cada alien que
 * estaba en el frame previo y ya no aparece en el actual se emite
 * una explosión en su última posición conocida.
 *
 * Parámetros:
 *   aliens  – array de Extraterrestre del frame actual (solo vivos,
 *             igual que lo que envía el servidor en el snapshot)
 *   n       – número de elementos en aliens[]
 *   px_arr  – coordenadas X de pantalla (píxeles) para cada elemento
 *             de aliens[], misma longitud n
 *   py_arr  – coordenadas Y de pantalla (píxeles), misma longitud n
 *
 * Llamar una vez por frame, después de parsear el snapshot del
 * servidor pero antes de efectos_dibujar().
 */
void efectos_sincronizar_aliens(const Extraterrestre *aliens, int n,
                                const int *px_arr, const int *py_arr);

/*
 * Detección de transición activo 1→0 para el OVNI.
 * Cuando el OVNI pasa de activo a inactivo se emite explosión en su
 * última posición conocida.
 *
 * Parámetros:
 *   activo  – 1 si el OVNI está activo este frame, 0 si no
 *   ovni_px – posición X en píxeles de pantalla (solo usada si activo==1)
 *   ovni_py – posición Y en píxeles de pantalla
 *   col     – color base de la explosión
 */
void efectos_actualizar_ovni(int activo, int ovni_px, int ovni_py, Color col);

#endif /* EFECTOS_H */
