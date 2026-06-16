#include "sprites.h"

/* -----------------------------------------------------------------------
 * Flash del canon — unico estado mutable en este modulo
 * --------------------------------------------------------------------- */
static int s_flash = 0;
#define FLASH_FRAMES 4

/* -----------------------------------------------------------------------
 * Animacion de dos fotogramas — alterna cada ANIM_TICKS ticks (≈0.33s a 60fps)
 * --------------------------------------------------------------------- */
static int s_anim_frame = 0;   /* 0 = frame A, 1 = frame B */
static int s_anim_tick  = 0;   /* contador de ticks        */
#define ANIM_TICKS 20

void sprites_alien_tick(void)
{
    s_anim_tick++;
    if (s_anim_tick >= ANIM_TICKS) {
        s_anim_tick  = 0;
        s_anim_frame = 1 - s_anim_frame;   /* alterna 0 ↔ 1 */
    }
}

void sprites_canon_flash(void)        { s_flash = FLASH_FRAMES; }
void sprites_canon_tick(void)         { if (s_flash > 0) s_flash--; }
int  sprites_canon_esta_en_flash(void){ return s_flash > 0; }

/* -----------------------------------------------------------------------
 * Helper: dibuja una grilla de pixels usando strings de '0'/'1'
 * ps   = tamano en px reales de cada "pixel" retro
 * El gap de 1px entre bloques (ps-1) da el look retro clasico
 * --------------------------------------------------------------------- */
static void draw_pixel_grid(int cx, int cy, int ps,
                             const char *rows[], int nrows, int ncols,
                             Color col)
{
    int ox = cx - (ncols * ps) / 2;
    int oy = cy - (nrows * ps) / 2;
    for (int r = 0; r < nrows; r++) {
        for (int c = 0; c < ncols; c++) {
            if (rows[r][c] == '1')
                DrawRectangle(ox + c * ps, oy + r * ps, ps - 1, ps - 1, col);
        }
    }
}

/* -----------------------------------------------------------------------
 * Calamar — TIPO_CALAMAR (0) — 10 pts
 * Grilla 8 cols x 8 filas, pixel size = 3
 * Frame A: antenas en cols 2,5 / tentaculos en cols 1,2,5,6
 * Frame B: antenas en cols 1,6 / tentaculos en cols 0,3,4,7 (alternados)
 * --------------------------------------------------------------------- */
static void sprites_dibujar_calamar(int cx, int cy, float escala, Color col)
{
    int ps = (int)(3.0f * escala + 0.5f);
    if (ps < 1) ps = 1;

    static const char *rows_a[] = {
        "00100100",   /* fila 0: antenas en cols 2,5           */
        "00011000",   /* fila 1                                */
        "01111110",   /* fila 2                                */
        "11011011",   /* fila 3                                */
        "11111111",   /* fila 4: todos                         */
        "01100110",   /* fila 5                                */
        "00111100",   /* fila 6                                */
        "01100110"    /* fila 7: tentaculos posicion A         */
    };

    static const char *rows_b[] = {
        "01000010",   /* fila 0: antenas en cols 1,6 (alternadas) */
        "00011000",   /* fila 1                                */
        "01111110",   /* fila 2                                */
        "11011011",   /* fila 3                                */
        "11111111",   /* fila 4: todos                         */
        "01100110",   /* fila 5                                */
        "00111100",   /* fila 6                                */
        "10011001"    /* fila 7: tentaculos posicion B (alternados) */
    };

    draw_pixel_grid(cx, cy, ps,
                    s_anim_frame == 0 ? rows_a : rows_b,
                    8, 8, col);
}

/* -----------------------------------------------------------------------
 * Cangrejo — TIPO_CANGREJO (1) — 20 pts
 * Grilla 11 cols x 8 filas, pixel size = 3
 * Frame A: pinzas cols 2,8 / patas cols 3,4,6,7
 * Frame B: pinzas cols 1,9 (extendidas) / patas cols 2,5,8 (alternadas)
 * --------------------------------------------------------------------- */
static void sprites_dibujar_cangrejo(int cx, int cy, float escala, Color col)
{
    int ps = (int)(3.0f * escala + 0.5f);
    if (ps < 1) ps = 1;

    static const char *rows_a[] = {
        "00100000100",   /* fila 0: pinzas en cols 2,8           */
        "00010001000",   /* fila 1: cols 3,7                     */
        "00111111100",   /* fila 2: cols 2-8                     */
        "01101110110",   /* fila 3                               */
        "11111111111",   /* fila 4: todos                        */
        "10111111101",   /* fila 5: cols 0,2-8,10                */
        "10100000101",   /* fila 6: cols 0,2,8,10                */
        "00011011000"    /* fila 7: patas posicion A             */
    };

    static const char *rows_b[] = {
        "01000000010",   /* fila 0: pinzas en cols 1,9 (extendidas) */
        "00010001000",   /* fila 1: cols 3,7                     */
        "00111111100",   /* fila 2: cols 2-8                     */
        "01101110110",   /* fila 3                               */
        "11111111111",   /* fila 4: todos                        */
        "10111111101",   /* fila 5: cols 0,2-8,10                */
        "10100000101",   /* fila 6: cols 0,2,8,10                */
        "01100110000"    /* fila 7: patas posicion B (alternadas) */
    };

    draw_pixel_grid(cx, cy, ps,
                    s_anim_frame == 0 ? rows_a : rows_b,
                    8, 11, col);
}

/* -----------------------------------------------------------------------
 * Pulpo — TIPO_PULPO (2) — 40 pts
 * Grilla 12 cols x 8 filas, pixel size = 3
 * Frame A: corona cols 4-7 / extremidades cols 0,1,9-11
 * Frame B: corona cols 3-8 (mas ancha) / extremidades cols 2,3,8,9 (alternadas)
 * --------------------------------------------------------------------- */
static void sprites_dibujar_pulpo(int cx, int cy, float escala, Color col)
{
    int ps = (int)(3.0f * escala + 0.5f);
    if (ps < 1) ps = 1;

    static const char *rows_a[] = {
        "000011110000",   /* fila 0: corona cols 4-7               */
        "001111111100",   /* fila 1: cols 2-9                      */
        "011111111110",   /* fila 2: cols 1-10                     */
        "110111111011",   /* fila 3: cols 0,1,3-8,10,11            */
        "111111111111",   /* fila 4: todos                         */
        "011011111010",   /* fila 5: cols 1,3-8,10                 */
        "001011111100",   /* fila 6                                */
        "110000000111"    /* fila 7: extremidades posicion A        */
    };

    static const char *rows_b[] = {
        "001111110000",   /* fila 0: corona cols 2-7 (desplazada izq) */
        "001111111100",   /* fila 1: cols 2-9                      */
        "011111111110",   /* fila 2: cols 1-10                     */
        "110111111011",   /* fila 3: cols 0,1,3-8,10,11            */
        "111111111111",   /* fila 4: todos                         */
        "011011111010",   /* fila 5: cols 1,3-8,10                 */
        "001011111100",   /* fila 6                                */
        "011000000110"    /* fila 7: extremidades posicion B        */
    };

    draw_pixel_grid(cx, cy, ps,
                    s_anim_frame == 0 ? rows_a : rows_b,
                    8, 12, col);
}

/* -----------------------------------------------------------------------
 * Tabla de estrategias — Strategy Pattern
 * --------------------------------------------------------------------- */
static AlienRenderFn s_renders[3] = {
    sprites_dibujar_calamar,   /* TIPO_CALAMAR  = 0 */
    sprites_dibujar_cangrejo,  /* TIPO_CANGREJO = 1 */
    sprites_dibujar_pulpo      /* TIPO_PULPO    = 2 */
};

AlienRenderFn sprites_obtener_render_alien(int tipo)
{
    if (tipo >= 0 && tipo < 3) return s_renders[tipo];
    return sprites_dibujar_calamar; /* fallback */
}

void sprites_dibujar_alien(int tipo, int cx, int cy, float escala, Color col)
{
    sprites_obtener_render_alien(tipo)(cx, cy, escala, col);
}

/* -----------------------------------------------------------------------
 * OVNI — plato volador clasico
 * Grilla 12 cols x 4 filas, pixel size = 4
 * Cupula (fila 0) en blanco, resto en col
 * --------------------------------------------------------------------- */
void sprites_dibujar_ovni(int cx, int cy, Color col)
{
    int ps = 4;

    /* Cupula: fila 0 en blanco */
    static const char *cupula[] = {
        "000011110000"
    };
    draw_pixel_grid(cx, cy - ps * 1, ps, cupula, 1, 12, WHITE);

    /* Cuerpo: filas 1-3 */
    static const char *cuerpo[] = {
        "001111111100",   /* fila 1 */
        "111111111111",   /* fila 2 base */
        "010101010101"    /* fila 3 luces alternas */
    };
    draw_pixel_grid(cx, cy + ps, ps, cuerpo, 3, 12, col);
}

/* -----------------------------------------------------------------------
 * Canon del jugador — pixel art retro
 * Grilla 9 cols x 4 filas, pixel size = 3
 * --------------------------------------------------------------------- */
void sprites_dibujar_canon(int cx, int cy, Color col)
{
    int ps = 3;

    static const char *rows[] = {
        "000010000",   /* fila 0 barrel: col 4 (2px ancho) */
        "000111000",   /* fila 1: cols 3,4,5               */
        "001111100",   /* fila 2: cols 2-6                 */
        "111111111"    /* fila 3 base: todos               */
    };

    draw_pixel_grid(cx, cy, ps, rows, 4, 9, col);

    /* Flash al disparar — circulo sobre el barrel */
    if (s_flash > 0) {
        float alpha = (float)s_flash / FLASH_FRAMES * 0.9f;
        int top = cy - (4 * ps) / 2 - 4;
        DrawCircle(cx, top, 4, Fade(YELLOW, alpha));
    }
}

/* -----------------------------------------------------------------------
 * Balas — ya son pixel-perfect, sin cambios
 * --------------------------------------------------------------------- */
void sprites_dibujar_bala_jugador(int cx, int cy)
{
    DrawRectangle(cx - 1, cy - 5, 3, 10, WHITE);
}

void sprites_dibujar_bala_alien(int cx, int cy)
{
    DrawRectangle(cx - 1, cy - 4, 2, 8, RED);
}
