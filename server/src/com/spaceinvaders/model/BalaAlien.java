package com.spaceinvaders.model;

public class BalaAlien {
    private int x;
    private int y;
    private boolean activa;

    public BalaAlien(int x, int y) {
        this.x = x;
        this.y = y;
        this.activa = true;
    }

    public boolean isActiva()     { return activa; }
    public void desactivar()      { activa = false; }
    public int getX()             { return x; }
    public int getY()             { return y; }
    public void setY(int y)       { this.y = y; }
}
