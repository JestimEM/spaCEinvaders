package com.spaceinvaders.factory;

import com.spaceinvaders.model.Extraterrestre;
import com.spaceinvaders.model.Cangrejo;

public class CangrejoFactory implements ExtraterrestreFactory {
    @Override
    public Extraterrestre crearExtraterrestre(int id, int x, int y) {
        return new Cangrejo(id, x, y);
    }

    @Override
    public Extraterrestre crearExtraterrestre(int id, int x, int y, int puntos) {
        return new Cangrejo(id, x, y, puntos);
    }
}
