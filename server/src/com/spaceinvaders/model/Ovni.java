package com.spaceinvaders.model;

public class Ovni {
    public enum Direccion { IZQUIERDA_DERECHA, DERECHA_IZQUIERDA }

    private boolean activo;
    private Direccion direccion;
    private int puntos;
    private int x;

    public Ovni() {
        this.activo = false;
    }

    public void aparecer(Direccion direccion, int puntos) {
        this.direccion = direccion;
        this.puntos    = puntos;
        this.activo    = true;
        this.x         = (direccion == Direccion.IZQUIERDA_DERECHA) ? 0 : 100;
    }

    public void desactivar() { this.activo = false; }

    public boolean estaActivo()           { return activo; }
    public Direccion obtenerDireccion()   { return direccion; }
    public int obtenerPuntos()            { return puntos; }
    public int obtenerX()                 { return x; }
    public void establecerX(int x)        { this.x = x; }
}
