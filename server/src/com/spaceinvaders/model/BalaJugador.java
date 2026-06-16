package com.spaceinvaders.model;

public class BalaJugador {
    private boolean activa = false;
    private int x = 0;
    private int y = 0;

    public void activar(int x, int y)  { this.activa = true; this.x = x; this.y = y; }
    public void desactivar()           { this.activa = false; }

    public boolean isActiva()          { return activa; }
    public int getX()                  { return x; }
    public int getY()                  { return y; }
    public void setX(int x)            { this.x = x; }
    public void setY(int y)            { this.y = y; }
}
