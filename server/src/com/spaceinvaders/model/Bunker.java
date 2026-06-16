package com.spaceinvaders.model;

public class Bunker {
    private static final int VIDAS_MAXIMAS = 4;

    private int id;
    private int vidas;

    public Bunker(int id) {
        this.id    = id;
        this.vidas = VIDAS_MAXIMAS;
    }

    public void recibirImpacto() {
        if (vidas > 0) vidas--;
    }

    /* Compatibilidad con comando admin "Bunkers P" (porcentaje 0-100) */
    public void establecerSalud(int porcentaje) {
        if      (porcentaje >= 100) vidas = 4;
        else if (porcentaje >= 70)  vidas = 3;
        else if (porcentaje >= 40)  vidas = 2;
        else if (porcentaje > 0)    vidas = 1;
        else                        vidas = 0;
    }

    public boolean estaDestruido()        { return vidas <= 0; }
    public int obtenerId()                { return id; }
    public int obtenerVidas()             { return vidas; }

    /* Deriva porcentaje para el cliente C (4→100, 3→75, 2→50, 1→25, 0→0) */
    public int obtenerPorcentajeSalud()   { return vidas * 25; }
}
