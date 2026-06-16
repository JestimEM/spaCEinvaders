#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include "../include/render.h"
#include "../include/constants.h"
#include "../include/sprites.h"
#include "../include/efectos.h"
#include "../include/bunker_visual.h"

/* ====================================================================
   Detección de click de mouse robusta (bypass GLFW/Win32 focus-click)
   GetAsyncKeyState lee el estado del hardware directamente desde
   user32.dll sin pasar por la cola de mensajes Win32/GLFW.
   En plataformas no-Win32 cae al IsMouseButtonPressed estándar.
   ==================================================================== */
#ifdef _WIN32
/* user32.dll ya está enlazado por Raylib; solo necesitamos la declaración. */
extern __declspec(dllimport) short __stdcall GetAsyncKeyState(int vKey);

static int mouse_click_detectar(void) {
    static int prev = 0;
    int cur  = (GetAsyncKeyState(0x01) & 0x8000) ? 1 : 0; /* 0x01 = VK_LBUTTON */
    int click = cur && !prev;
    prev = cur;
    return click;
}
#else
static int mouse_click_detectar(void) {
    return IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}
#endif

/* ---- Layout de pantalla ---- */
#define HUD_H      55
#define GAME_W_PX  880
#define GAME_H_PX  570

static int gcol(int gx) { return 10 + gx * GAME_W_PX / ANCHO_JUEGO; }
static int grow(int gy) { return HUD_H + gy * GAME_H_PX / ALTO_JUEGO; }

static Rectangle hbox_to_rect(HitBox h) {
    return (Rectangle){ h.x, h.y, h.width, h.height };
}

/* ---- Hitbox de bala (bala del jugador) ---- */
static Rectangle bala_rect(int bx, int by) {
    return (Rectangle){ (float)gcol(bx) - 1.5f, (float)grow(by) - 4.0f, 3.0f, 8.0f };
}

/* ====================================================================
   Inicialización / destrucción
   ==================================================================== */
void render_inicializar(void) {
    InitWindow(SCREEN_W, SCREEN_H, "spaCEinvaders");
    SetTargetFPS(60);
    /* SetWindowFocused() se llama en render_menu() después del primer frame,
       cuando GLFW ya procesó al menos un ciclo de eventos Win32. */
}

void render_cargar_texturas(void) {
    bv_inicializar_todos();
    efectos_limpiar();
}

void render_destruir(void) {
    CloseWindow();
}

/* ====================================================================
   Frame principal — actualiza hitboxes y ejecuta colisiones predictivas
   ==================================================================== */
void render_frame(EstadoJuego *ej, int es_observador, int id_jugador_observado) {
    static const int bunker_gx[NUM_BUNKERS] = {12, 37, 62, 87};

    float dt = GetFrameTime();
    static int bala_activa_anterior = 0;

    /* ---- Actualizar hitboxes y colisión predictiva (client-side) ---- */
    Rectangle br = {0};
    int bala_prediccion = ej->bala_activa;
    if (bala_prediccion) {
        br = bala_rect(ej->bala_x, ej->bala_y);
    }

    for (int i = 0; i < ej->cantidad_extraterrestres; i++) {
        Extraterrestre *e = &ej->extraterrestres[i];
        if (!e->vivo) continue;
        int cx = gcol(e->x), cy = grow(e->y);
        e->hitbox = (HitBox){ (float)(cx - 12), (float)(cy - 8), 24.0f, 16.0f };
        if (bala_prediccion && !e->golpe_visual) {
            if (CheckCollisionRecs(br, hbox_to_rect(e->hitbox))) {
                e->golpe_visual = 1; /* visualmente muerto, esperando confirmación */
            }
        }
    }

    /* ---- Hitbox del cañón ---- */
    {
        int cx = gcol(ej->jugador.posicion_canon_x);
        int cy = grow(ALTO_JUEGO);
        ej->jugador.hitbox = (HitBox){ (float)(cx - 14), (float)(cy - 10), 28.0f, 20.0f };
    }

    /* ---- Hitboxes de bunkers ---- */
    for (int i = 0; i < NUM_BUNKERS; i++) {
        int bx = gcol(bunker_gx[i]);
        int by = grow(70);
        ej->bunkers[i].hitbox = (HitBox){ (float)(bx - 18), (float)(by - 8), 36.0f, 16.0f };
    }

    /* Detectar disparo para flash del cañón */
    if (ej->bala_activa && !bala_activa_anterior)
        sprites_canon_flash();
    bala_activa_anterior = ej->bala_activa;

    sprites_canon_tick();
    sprites_alien_tick();
    efectos_actualizar(dt);

    /* ---- Frame-diff: detectar aliens desaparecidos → explosión ---- */
    {
        Extraterrestre aliens_vivos[MAX_EXTRATERR];
        int alien_px[MAX_EXTRATERR];
        int alien_py[MAX_EXTRATERR];
        int n_vivos = 0;
        for (int i = 0; i < ej->cantidad_extraterrestres; i++) {
            Extraterrestre *e = &ej->extraterrestres[i];
            if (!e->vivo) continue;
            alien_px[n_vivos] = gcol(e->x);
            alien_py[n_vivos] = grow(e->y);
            aliens_vivos[n_vivos] = *e;
            n_vivos++;
        }
        efectos_sincronizar_aliens(aliens_vivos, n_vivos, alien_px, alien_py);
    }

    /* ---- Frame-diff: detectar OVNI destruido → explosión ---- */
    if (ej->ovni.activo) {
        efectos_actualizar_ovni(1, gcol(ej->ovni.x), grow(10), RED);
    } else {
        efectos_actualizar_ovni(0, 0, 0, RED);
    }

    /* ==== DIBUJO ==== */
    BeginDrawing();
    ClearBackground(BLACK);

    /* ---- HUD retro estilo arcade 80s ---- */
    DrawRectangle(0, 0, SCREEN_W, HUD_H - 4, BLACK);
    /* Línea verde fosforescente al borde inferior del HUD */
    DrawRectangle(0, HUD_H - 5, SCREEN_W, 2, (Color){0, 255, 70, 200});

    /* SCORE izquierda */
    DrawText("1UP", 20, 6, 16, (Color){0, 255, 70, 255});
    {
        char score_txt[32];
        snprintf(score_txt, sizeof(score_txt), "%d", ej->jugador.puntuacion);
        DrawText(score_txt, 20, 24, 16, WHITE);
    }

    /* HI-SCORE / RONDA centro */
    {
        const char *hi_lbl = "RECORD";
        DrawText(hi_lbl, (SCREEN_W - MeasureText(hi_lbl, 16)) / 2, 6, 16, (Color){0, 255, 70, 255});
        char ronda_txt[32];
        snprintf(ronda_txt, sizeof(ronda_txt), "RONDA %d", ej->ronda);
        DrawText(ronda_txt, (SCREEN_W - MeasureText(ronda_txt, 16)) / 2, 24, 16, WHITE);
    }

    /* LIVES como cañones pequeños a la derecha */
    DrawText("VIDAS", SCREEN_W - 160, 6, 16, (Color){0, 255, 70, 255});
    for (int i = 0; i < ej->jugador.vidas && i < 5; i++) {
        DrawRectangle(SCREEN_W - 130 + i * 22, 22, 6, 12, LIME);
        DrawRectangle(SCREEN_W - 127 + i * 22, 16, 3,  8, LIME);
    }

    /* Borde área de juego */
    DrawRectangleLines(8, HUD_H, SCREEN_W - 16, GAME_H_PX + 2, (Color){0, 255, 70, 60});

    /* OVNI */
    if (ej->ovni.activo) {
        int ox = gcol(ej->ovni.x);
        int oy = grow(10);
        ej->ovni.hitbox = (HitBox){ (float)(ox - 14), (float)(oy - 8), 28.0f, 16.0f };
        sprites_dibujar_ovni(ox, oy, (Color){255, 30, 30, 255});
        char pts[16];
        snprintf(pts, sizeof(pts), "%d", ej->ovni.puntos);
        DrawText(pts, ox - MeasureText(pts, 11) / 2, oy - 22, 11, RED);
    }

    /* Extraterrestres */
    for (int i = 0; i < ej->cantidad_extraterrestres; i++) {
        Extraterrestre *e = &ej->extraterrestres[i];
        if (!e->vivo) continue;  /* safety — dead aliens aren't in the snapshot */
        int cx = gcol(e->x), cy = grow(e->y);

        if (e->golpe_visual) {
            /* Mantener el parpadeo visual existente */
            DrawRectangleRec(hbox_to_rect(e->hitbox), Fade(WHITE, 0.3f));
            continue;
        }

        Color col = (e->tipo == TIPO_PULPO)   ? (Color){255, 220,   0, 255} /* amarillo clásico */
                  : (e->tipo == TIPO_CANGREJO) ? (Color){  0, 200, 255, 255} /* cyan */
                                               : (Color){  0, 255,  70, 255};/* verde fosforescente */
        sprites_dibujar_alien(e->tipo, cx, cy, 1.0f, col);
    }

    /* Bunkers */
    for (int i = 0; i < NUM_BUNKERS; i++) {
        Bunker *b = &ej->bunkers[i];
        if (b->porcentaje_salud == 0) {
            bv_sincronizar(i, 0);
            continue;
        }
        bv_sincronizar(i, b->porcentaje_salud);
        int sx = gcol(bunker_gx[i]) - (BV_COLS * BV_PX_SIZE / 2);
        int sy = grow(70)           - (BV_FILAS * BV_PX_SIZE / 2);
        Color col_b = (b->porcentaje_salud >= 75) ? GREEN :
                      (b->porcentaje_salud >= 50) ? YELLOW : RED;
        bv_dibujar(i, sx, sy, col_b);
    }

    /* Bala del jugador (server-side) */
    if (ej->bala_activa) {
        sprites_dibujar_bala_jugador((int)(br.x + br.width/2), (int)(br.y + br.height/2));
    }

    /* Balas alienígenas */
    for (int i = 0; i < ej->cantidad_balas_alien; i++) {
        if (!ej->balas_alien[i].activa) continue;
        int bx = gcol(ej->balas_alien[i].x);
        int by = grow(ej->balas_alien[i].y);
        sprites_dibujar_bala_alien(bx, by);
    }

    /* Cañón del jugador */
    {
        int cx = gcol(ej->jugador.posicion_canon_x);
        int cy = grow(ALTO_JUEGO);
        sprites_dibujar_canon(cx, cy, LIME);
    }

    /* Línea verde sobre los controles */
    DrawRectangle(0, SCREEN_H - 30, SCREEN_W, 1, (Color){0, 255, 70, 180});
    if (es_observador) {
        char obs_txt[64];
        snprintf(obs_txt, sizeof(obs_txt), "OBSERVADOR — viendo al jugador %d", id_jugador_observado);
        DrawText(obs_txt, (SCREEN_W - MeasureText(obs_txt, 13)) / 2,
                 SCREEN_H - 22, 13, (Color){0, 180, 50, 255});
    } else {
        const char *ctrl = "MOVER: <- ->   DISPARAR: ESPACIO   SALIR: Q";
        DrawText(ctrl, (SCREEN_W - MeasureText(ctrl, 13)) / 2,
                 SCREEN_H - 22, 13, (Color){0, 180, 50, 255});
    }

    /* Overlay ronda completa */
    if (ej->ronda_completa) {
        const char *msg = "RONDA COMPLETADA!  Vida extra!";
        int tw = MeasureText(msg, 22);
        DrawRectangle((SCREEN_W - tw) / 2 - 8, SCREEN_H / 2 - 18, tw + 16, 36, BLACK);
        DrawRectangleLinesEx((Rectangle){(float)((SCREEN_W - tw) / 2 - 8),
                                         (float)(SCREEN_H / 2 - 18),
                                         (float)(tw + 16), 36.0f}, 2,
                             (Color){0, 255, 70, 255});
        DrawText(msg, (SCREEN_W - tw) / 2, SCREEN_H / 2 - 10, 22, GOLD);
    }

    efectos_dibujar();

    /* CRT scanlines overlay */
    for (int sl = 0; sl < SCREEN_H; sl += 3) {
        DrawRectangle(0, sl, SCREEN_W, 1, (Color){0, 0, 0, 40});
    }
    /* Viñeta CRT: bordes laterales oscuros */
    DrawRectangleGradientH(0,            0, 60, SCREEN_H, (Color){0, 0, 0, 80}, (Color){0, 0, 0, 0});
    DrawRectangleGradientH(SCREEN_W - 60, 0, 60, SCREEN_H, (Color){0, 0, 0, 0}, (Color){0, 0, 0, 80});

    EndDrawing();
}

/* ====================================================================
   Helpers de botones
   ==================================================================== */
static void dibujar_boton(Rectangle r, const char *txt, int hover, Color base, Color borde) {
    DrawRectangleRec(r, hover ? Fade(base, 0.9f) : Fade(base, 0.65f));
    DrawRectangleLinesEx(r, 2, borde);
    int fs = 20;
    DrawText(txt,
             (int)(r.x + (r.width  - MeasureText(txt, fs)) / 2),
             (int)(r.y + (r.height - fs) / 2),
             fs, WHITE);
}

/* ====================================================================
   Menú principal: Jugador / Observador
   Devuelve: 0=Jugador  1=Observador  -1=cerrar ventana
   ==================================================================== */
int render_menu(void) {
    Rectangle btn_j = { (float)(SCREEN_W / 2 - 120), (float)(SCREEN_H / 2 + 70), 240, 52 };
    Rectangle btn_o = { (float)(SCREEN_W / 2 - 120), (float)(SCREEN_H / 2 + 140), 240, 52 };

    while (!WindowShouldClose()) {
        /* Teclado: respuesta inmediata, sin problema de focus-click en Windows.
           KEY_ENTER / KEY_KP_ENTER actúan como alias de J (primera opción). */
        if (IsKeyPressed(KEY_J) || IsKeyPressed(KEY_ONE) ||
            IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) return 0;
        if (IsKeyPressed(KEY_O) || IsKeyPressed(KEY_TWO))   return 1;
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_Q)) return -1;

        /* Mouse: click en mitad superior → Jugador, mitad inferior → Observador.
           GetAsyncKeyState bypassa GLFW — funciona desde el primer frame. */
        if (mouse_click_detectar()) {
            Vector2 mouse = GetMousePosition();
            if (mouse.y < (float)(SCREEN_H / 2 + 105)) return 0;  /* encima línea media botones = Jugador */
            else                                        return 1;  /* debajo = Observador */
        }

        BeginDrawing();
        ClearBackground((Color){0, 0, 10, 255});

        /* Título fosforescente */
        {
            const char *titulo = "SPACE  INVADERS";
            DrawText(titulo,
                     (SCREEN_W - MeasureText(titulo, 48)) / 2,
                     SCREEN_H / 2 - 200, 48, (Color){0, 255, 70, 255});
        }

        /* "INSERT COIN" parpadeante */
        {
            static int blink = 0;
            blink = (blink + 1) % 60;
            if (blink < 40) {
                const char *coin = "- INSERTA MONEDA -";
                DrawText(coin,
                         (SCREEN_W - MeasureText(coin, 20)) / 2,
                         SCREEN_H / 2 - 130, 20, (Color){255, 220, 0, 255});
            }
        }

        /* Tabla de puntos retro */
        {
            const char *tbl_hdr = "*TABLA DE PUNTUACION*";
            DrawText(tbl_hdr,
                     (SCREEN_W - MeasureText(tbl_hdr, 16)) / 2,
                     SCREEN_H / 2 - 80, 16, (Color){0, 255, 70, 255});

            /* Fila OVNI */
            DrawText("[?]", (SCREEN_W / 2) - 80, SCREEN_H / 2 - 52, 16, (Color){255, 30, 30, 255});
            DrawText("MISTERIO  ??? PTS", (SCREEN_W / 2) - 36, SCREEN_H / 2 - 52, 16, WHITE);

            /* Fila Pulpo */
            DrawText("[*]", (SCREEN_W / 2) - 80, SCREEN_H / 2 - 28, 16, (Color){255, 220, 0, 255});
            DrawText("PULPO      40 PTS", (SCREEN_W / 2) - 36, SCREEN_H / 2 - 28, 16, WHITE);

            /* Fila Cangrejo */
            DrawText("[W]", (SCREEN_W / 2) - 80, SCREEN_H / 2 - 4, 16, (Color){0, 200, 255, 255});
            DrawText("CANGREJO   20 PTS", (SCREEN_W / 2) - 36, SCREEN_H / 2 - 4, 16, WHITE);

            /* Fila Calamar */
            DrawText("[X]", (SCREEN_W / 2) - 80, SCREEN_H / 2 + 20, 16, (Color){0, 255, 70, 255});
            DrawText("CALAMAR    10 PTS", (SCREEN_W / 2) - 36, SCREEN_H / 2 + 20, 16, WHITE);
        }

        /* Botones de selección */
        Vector2 mouse_draw = GetMousePosition();
        int hj_draw = CheckCollisionPointRec(mouse_draw, btn_j);
        int ho_draw = CheckCollisionPointRec(mouse_draw, btn_o);
        dibujar_boton(btn_j, "[ J ] JUGADOR",    hj_draw, (Color){0, 60, 0, 255},   (Color){0, 255, 70, 255});
        dibujar_boton(btn_o, "[ O ] OBSERVADOR", ho_draw, (Color){0,  0, 60, 255},  (Color){0, 150, 255, 255});

        {
            const char *hint = "[J/Enter] Jugador   [O] Observador   [ESC] Salir";
            DrawText(hint, (SCREEN_W - MeasureText(hint, 14)) / 2,
                     SCREEN_H - 30, 14, (Color){0, 130, 40, 200});
        }

        /* CRT scanlines en menú */
        for (int sl = 0; sl < SCREEN_H; sl += 3) {
            DrawRectangle(0, sl, SCREEN_W, 1, (Color){0, 0, 0, 40});
        }
        DrawRectangleGradientH(0,            0, 60, SCREEN_H, (Color){0, 0, 0, 80}, (Color){0, 0, 0, 0});
        DrawRectangleGradientH(SCREEN_W - 60, 0, 60, SCREEN_H, (Color){0, 0, 0, 0}, (Color){0, 0, 0, 80});

        EndDrawing();
    }
    return -1;
}

/* ====================================================================
   Lista de partidas activas (solo para Observador)
   Devuelve: id seleccionado, o -1=cerrar/salir
   ==================================================================== */
int render_lista_sesiones(int *ids, int n) {
    while (!WindowShouldClose()) {
        /* Teclado: teclas 1-8 seleccionan la partida en esa posición */
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_Q)) return -1;
        if (n > 0) {
            for (int k = KEY_ONE; k <= KEY_EIGHT; k++) {
                if (IsKeyPressed(k)) {
                    int idx = k - KEY_ONE;
                    if (idx < n) return ids[idx];
                }
            }
        }

        Vector2 mouse = GetMousePosition();
        int sel = -1;
        int click = (n > 0) && mouse_click_detectar();

        BeginDrawing();
        ClearBackground((Color){0, 0, 10, 255});

        {
            const char *titulo = "PARTIDAS ACTIVAS";
            DrawText(titulo, (SCREEN_W - MeasureText(titulo, 34)) / 2,
                     100, 34, (Color){0, 255, 70, 255});
            /* Línea separadora verde */
            DrawRectangle(SCREEN_W / 2 - 160, 142, 320, 2, (Color){0, 255, 70, 120});
        }

        if (n == 0) {
            const char *msg = "NO HAY PARTIDAS ACTIVAS";
            DrawText(msg, (SCREEN_W - MeasureText(msg, 20)) / 2,
                     SCREEN_H / 2, 20, (Color){255, 220, 0, 255});
            const char *hint = "[ESC] Volver";
            DrawText(hint, (SCREEN_W - MeasureText(hint, 16)) / 2,
                     SCREEN_H / 2 + 50, 16, (Color){0, 130, 40, 200});
        } else {
            for (int i = 0; i < n && i < 8; i++) {
                Rectangle btn = { (float)(SCREEN_W / 2 - 130),
                                  (float)(180 + i * 58), 260, 44 };
                int hover = CheckCollisionPointRec(mouse, btn);
                if (click && hover) sel = ids[i];
                char txt[32];
                snprintf(txt, sizeof(txt), "[%d] PARTIDA %d", i + 1, ids[i]);
                dibujar_boton(btn, txt, hover,
                              (Color){30, 0, 60, 255}, (Color){150, 0, 255, 255});
            }
            const char *hint = "[1-8] Seleccionar   [ESC] Volver";
            DrawText(hint, (SCREEN_W - MeasureText(hint, 14)) / 2,
                     SCREEN_H - 30, 14, (Color){0, 130, 40, 200});
        }

        /* CRT scanlines */
        for (int sl = 0; sl < SCREEN_H; sl += 3) {
            DrawRectangle(0, sl, SCREEN_W, 1, (Color){0, 0, 0, 40});
        }
        DrawRectangleGradientH(0,            0, 60, SCREEN_H, (Color){0, 0, 0, 80}, (Color){0, 0, 0, 0});
        DrawRectangleGradientH(SCREEN_W - 60, 0, 60, SCREEN_H, (Color){0, 0, 0, 0}, (Color){0, 0, 0, 80});

        EndDrawing();

        if (sel >= 0) return sel;
    }
    return -1;
}

/* ====================================================================
   Pantalla fin de partida con botones Salir / Reiniciar
   Devuelve: 0=Salir  1=Reiniciar
   ==================================================================== */
int render_pantalla_fin(int puntuacion, int es_victoria) {
    const char *titulo = es_victoria ? "*** VICTORIA ***" : "FIN  DEL  JUEGO";
    char score[64];
    snprintf(score, sizeof(score), "PUNTUACION: %d", puntuacion);

    Rectangle btn_reiniciar = { (float)(SCREEN_W / 2 - 120), (float)(SCREEN_H / 2 + 60), 240, 52 };
    Rectangle btn_salir     = { (float)(SCREEN_W / 2 - 120), (float)(SCREEN_H / 2 + 130), 240, 52 };

    /* Flush click residual del gameplay */
    while (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) PollInputEvents();

    while (!WindowShouldClose()) {
        /* Teclado: atajos directos */
        if (IsKeyPressed(KEY_R) || IsKeyPressed(KEY_ENTER) ||
            IsKeyPressed(KEY_KP_ENTER))                          return 1;  /* Reiniciar */
        if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_ESCAPE) ||
            IsKeyPressed(KEY_Q))                                  return 0;  /* Salir */

        Vector2 mouse = GetMousePosition();
        int hs = CheckCollisionPointRec(mouse, btn_salir);
        int hr = CheckCollisionPointRec(mouse, btn_reiniciar);

        if (mouse_click_detectar()) {
            if (hs) return 0;
            if (hr) return 1;
        }

        /* Color título: victoria=dorado, game over=rojo parpadeante */
        static int blink2 = 0;
        blink2 = (blink2 + 1) % 60;
        Color col_titulo = es_victoria
            ? (Color){255, 220, 0, 255}
            : ((blink2 < 40) ? (Color){255, 30, 30, 255} : (Color){80, 0, 0, 255});

        BeginDrawing();
        ClearBackground((Color){0, 0, 10, 255});

        DrawText(titulo,
                 (SCREEN_W - MeasureText(titulo, 52)) / 2,
                 SCREEN_H / 2 - 120, 52, col_titulo);

        /* Línea separadora */
        DrawRectangle(SCREEN_W / 2 - 120, SCREEN_H / 2 - 50, 240, 2,
                      (Color){0, 255, 70, 120});

        DrawText(score,
                 (SCREEN_W - MeasureText(score, 22)) / 2,
                 SCREEN_H / 2 - 30, 22, WHITE);

        dibujar_boton(btn_salir,     "[S] SALIR",          hs, (Color){60, 0,  0, 255},  (Color){255, 30,  30, 255});
        dibujar_boton(btn_reiniciar, "[R] JUGAR DE NUEVO", hr, (Color){ 0, 50, 0, 255},  (Color){  0, 255, 70, 255});

        {
            const char *hint = "[R] Jugar de nuevo   [S] Salir   [ESC] Salir";
            DrawText(hint, (SCREEN_W - MeasureText(hint, 14)) / 2,
                     SCREEN_H - 30, 14, (Color){0, 130, 40, 200});
        }

        /* CRT scanlines */
        for (int sl = 0; sl < SCREEN_H; sl += 3) {
            DrawRectangle(0, sl, SCREEN_W, 1, (Color){0, 0, 0, 40});
        }
        DrawRectangleGradientH(0,            0, 60, SCREEN_H, (Color){0, 0, 0, 80}, (Color){0, 0, 0, 0});
        DrawRectangleGradientH(SCREEN_W - 60, 0, 60, SCREEN_H, (Color){0, 0, 0, 0}, (Color){0, 0, 0, 80});

        EndDrawing();
    }
    return 0;
}

/* ====================================================================
   Overlay (llamar dentro de BeginDrawing/EndDrawing)
   ==================================================================== */
void render_mensaje_overlay(const char *msg) {
    int tw = MeasureText(msg, 20);
    DrawRectangle((SCREEN_W - tw) / 2 - 8, SCREEN_H / 2 - 16, tw + 16, 32, DARKBLUE);
    DrawText(msg, (SCREEN_W - tw) / 2, SCREEN_H / 2 - 10, 20, WHITE);
}
