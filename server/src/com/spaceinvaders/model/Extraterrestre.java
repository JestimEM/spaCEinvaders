package com.spaceinvaders.model;

public abstract class Extraterrestre {
    protected int id;
    protected int x;
    protected int y;
    protected int puntos;
    protected boolean vivo;

    public Extraterrestre(int id, int x, int y, int puntos) {
        this.id = id;
        this.x = x;
        this.y = y;
        this.puntos = puntos;
        this.vivo = true;
    }

    public abstract String obtenerTipo();

    public void destruir()          { this.vivo = false; }

    public int obtenerId()          { return id; }
    public int obtenerX()           { return x; }
    public int obtenerY()           { return y; }
    public int obtenerPuntos()      { return puntos; }
    public boolean estaVivo()       { return vivo; }
    public void establecerX(int x)  { this.x = x; }
    public void establecerY(int y)  { this.y = y; }
}
