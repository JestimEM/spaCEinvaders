package com.spaceinvaders.factory;

import com.spaceinvaders.model.Extraterrestre;

public interface ExtraterrestreFactory {
    Extraterrestre crearExtraterrestre(int id, int x, int y);
    Extraterrestre crearExtraterrestre(int id, int x, int y, int puntos);
}
