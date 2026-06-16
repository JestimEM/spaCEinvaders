package com.spaceinvaders.model;

public class Calamar extends Extraterrestre {
    public static final int PUNTOS_BASE = 10;

    public Calamar(int id, int x, int y) {
        super(id, x, y, PUNTOS_BASE);
    }

    public Calamar(int id, int x, int y, int puntos) {
        super(id, x, y, puntos);
    }

    @Override
    public String obtenerTipo() { return "CALAMAR"; }
}
