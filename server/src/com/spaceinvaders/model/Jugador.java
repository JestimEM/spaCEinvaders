package com.spaceinvaders.model;

public class Jugador {
    public static final int VIDAS_INICIALES = 3;

    private int vidas;
    private int puntuacion;
    private int posicionCanon;

    public Jugador() {
        reiniciar();
    }

    public void reiniciar() {
        this.vidas         = VIDAS_INICIALES;
        this.puntuacion    = 0;
        this.posicionCanon = 50;
    }

    public void agregarPuntuacion(int puntos) { this.puntuacion += puntos; }
    public void perderVida()                  { if (vidas > 0) vidas--; }
    public void ganarVida()                   { vidas++; }
    public boolean estaVivo()                 { return vidas > 0; }

    public int obtenerVidas()                              { return vidas; }
    public int obtenerPuntuacion()                         { return puntuacion; }
    public int obtenerPosicionCanon()                      { return posicionCanon; }
    public void establecerPosicionCanon(int posicion)      { this.posicionCanon = posicion; }
}
