#include "raylib.h"
#include "../include/efectos.h"
#include <string.h>
#include <math.h>

/* ====================================================================
   Pool estático de partículas
   ==================================================================== */
static Particula s_pool[MAX_PARTICULAS];

/* ====================================================================
   Registro de IDs de aliens ya procesados (evita doble explosión)
   ==================================================================== */
#define MAX_IDS_MUERTOS 64
static int s_ids[MAX_IDS_MUERTOS];
static int s_ids_n = 0;

/* ====================================================================
   efectos_limpiar
   ==================================================================== */
void efectos_limpiar(void) {
    memset(s_pool, 0, sizeof(s_pool));
    s_ids_n = 0;
}

/* ====================================================================
   efectos_emitir_explosion
   Emite PARTICULAS_X_EXP partículas desde (px, py) en píxeles.
   ==================================================================== */
void efectos_emitir_explosion(float px, float py, Color col_base) {
    for (int i = 0; i < PARTICULAS_X_EXP; i++) {
        /* Buscar primer slot libre */
        Particula *p = NULL;
        for (int k = 0; k < MAX_PARTICULAS; k++) {
            if (s_pool[k].vida <= 0.0f) {
                p = &s_pool[k];
                break;
            }
        }
        if (p == NULL) return; /* pool lleno — ignorar sin crashear */

        float angulo_deg = (float)(i * 360) / (float)PARTICULAS_X_EXP
                           + (float)GetRandomValue(-20, 20);
        float angulo_rad = angulo_deg * 3.14159265f / 180.0f;
        float speed      = (float)GetRandomValue(50, 130);

        p->x    = px;
        p->y    = py;
        p->vx   = cosf(angulo_rad) * speed;
        p->vy   = sinf(angulo_rad) * speed;
        p->vida = VIDA_PARTICULA + (float)GetRandomValue(-10, 10) * 0.01f;
        p->col  = col_base;
    }
}

/* ====================================================================
   efectos_actualizar
   dt en segundos (GetFrameTime()).
   ==================================================================== */
void efectos_actualizar(float dt) {
    for (int i = 0; i < MAX_PARTICULAS; i++) {
        Particula *p = &s_pool[i];
        if (p->vida <= 0.0f) continue;

        p->x    += p->vx * dt;
        p->y    += p->vy * dt;
        p->vy   += 90.0f * dt;  /* gravedad suave */
        p->vida -= dt;

        if (p->vida <= 0.0f) {
            p->vida = 0.0f; /* marcar slot como libre */
        }
    }
}

/* ====================================================================
   efectos_dibujar
   ==================================================================== */
void efectos_dibujar(void) {
    for (int i = 0; i < MAX_PARTICULAS; i++) {
        const Particula *p = &s_pool[i];
        if (p->vida <= 0.0f) continue;

        float alpha = p->vida / VIDA_PARTICULA;
        if (alpha > 1.0f) alpha = 1.0f;
        if (alpha < 0.0f) alpha = 0.0f;

        float radio = 1.5f + alpha * 2.0f;
        DrawCircle((int)p->x, (int)p->y, (int)radio, Fade(p->col, alpha));
    }
}

/* ====================================================================
   efectos_registrar_muerte_alien
   No-op si el id ya fue registrado; emite explosión en caso contrario.
   ==================================================================== */
void efectos_registrar_muerte_alien(int id, float px, float py, Color col) {
    /* Verificar si ya se procesó este alien */
    for (int i = 0; i < s_ids_n; i++) {
        if (s_ids[i] == id) return;
    }

    /* Registrar el ID */
    if (s_ids_n < MAX_IDS_MUERTOS) {
        s_ids[s_ids_n++] = id;
    } else {
        /* Buffer lleno — rotar: descartar el más antiguo */
        memmove(s_ids, s_ids + 1, (MAX_IDS_MUERTOS - 1) * sizeof(int));
        s_ids[MAX_IDS_MUERTOS - 1] = id;
    }

    efectos_emitir_explosion(px, py, col);
}

/* ====================================================================
   Snapshot del frame anterior para detección de desaparición de aliens
   IDs de aliens: rango 100-154 (5 filas × 11 cols), offset = 100
   ==================================================================== */
#define MAX_ID_ALIEN  55
#define ID_OFFSET     100

typedef struct {
    int vivo;   /* 1 = estaba vivo el frame anterior */
    int px;     /* última posición X en píxeles */
    int py;     /* última posición Y en píxeles */
    Color col;  /* color base para la explosión */
} AlienSlot;

static AlienSlot s_aliens_prev[MAX_ID_ALIEN];
static int       s_snapshot_inicializado = 0;

/* ====================================================================
   Snapshot del OVNI
   ==================================================================== */
static int s_ovni_activo_prev = 0;
static int s_ovni_px_prev     = 0;
static int s_ovni_py_prev     = 0;

/* ====================================================================
   efectos_sincronizar_aliens
   Compara el frame actual contra el snapshot previo y emite
   explosiones para los aliens que desaparecieron.
   ==================================================================== */
void efectos_sincronizar_aliens(const Extraterrestre *aliens, int n,
                                const int *px_arr, const int *py_arr)
{
    /* Primera llamada: inicializar snapshot sin explosiones */
    if (!s_snapshot_inicializado) {
        memset(s_aliens_prev, 0, sizeof(s_aliens_prev));
        s_snapshot_inicializado = 1;
        /* Poblar con el frame actual y salir */
        for (int i = 0; i < n; i++) {
            int slot = aliens[i].id - ID_OFFSET;
            if (slot < 0 || slot >= MAX_ID_ALIEN) continue;
            s_aliens_prev[slot].vivo = 1;
            s_aliens_prev[slot].px   = px_arr[i];
            s_aliens_prev[slot].py   = py_arr[i];
            /* Color según tipo — igual que en render.c */
            switch (aliens[i].tipo) {
                case 2:  s_aliens_prev[slot].col = YELLOW;  break; /* TIPO_PULPO */
                case 1:  s_aliens_prev[slot].col = SKYBLUE; break; /* TIPO_CANGREJO */
                default: s_aliens_prev[slot].col = GREEN;   break; /* TIPO_CALAMAR */
            }
        }
        return;
    }

    /* Marcar qué slots aparecen en el frame actual */
    int visto[MAX_ID_ALIEN];
    memset(visto, 0, sizeof(visto));

    for (int i = 0; i < n; i++) {
        int slot = aliens[i].id - ID_OFFSET;
        if (slot < 0 || slot >= MAX_ID_ALIEN) continue;

        visto[slot] = 1;

        /* Actualizar posición y color conocidos */
        s_aliens_prev[slot].px = px_arr[i];
        s_aliens_prev[slot].py = py_arr[i];
        switch (aliens[i].tipo) {
            case 2:  s_aliens_prev[slot].col = YELLOW;  break;
            case 1:  s_aliens_prev[slot].col = SKYBLUE; break;
            default: s_aliens_prev[slot].col = GREEN;   break;
        }
    }

    /* Para cada slot que era vivo y ahora no aparece → explosión */
    for (int slot = 0; slot < MAX_ID_ALIEN; slot++) {
        if (s_aliens_prev[slot].vivo && !visto[slot]) {
            efectos_emitir_explosion(
                (float)s_aliens_prev[slot].px,
                (float)s_aliens_prev[slot].py,
                s_aliens_prev[slot].col
            );
            s_aliens_prev[slot].vivo = 0;
        }
    }

    /* Actualizar flags vivo para el próximo frame */
    for (int slot = 0; slot < MAX_ID_ALIEN; slot++) {
        s_aliens_prev[slot].vivo = visto[slot];
    }
}

/* ====================================================================
   efectos_actualizar_ovni
   Detecta transición activo 1→0 y emite explosión en la última
   posición conocida del OVNI.
   ==================================================================== */
void efectos_actualizar_ovni(int activo, int ovni_px, int ovni_py, Color col)
{
    if (!activo && s_ovni_activo_prev) {
        /* OVNI acaba de desaparecer — explotar en última posición */
        efectos_emitir_explosion(
            (float)s_ovni_px_prev,
            (float)s_ovni_py_prev,
            col
        );
    }

    /* Actualizar snapshot */
    s_ovni_activo_prev = activo;
    if (activo) {
        s_ovni_px_prev = ovni_px;
        s_ovni_py_prev = ovni_py;
    }
}
