package com.spaceinvaders.test;

import java.io.*;
import java.net.*;

/**
 * Prueba de conexion: servidor Java espera un cliente C,
 * recibe un JSON, lo parsea e imprime los campos, y responde.
 */
public class PruebaConexion {

    private static final int PUERTO = 8080;

    public static void main(String[] args) throws Exception {
        System.out.println("[SERVIDOR] Esperando conexion en puerto " + PUERTO + "...");

        try (ServerSocket socketServidor = new ServerSocket(PUERTO)) {
            Socket cliente = socketServidor.accept();
            System.out.println("[SERVIDOR] Cliente conectado: " + cliente.getInetAddress());

            BufferedReader entrada = new BufferedReader(new InputStreamReader(cliente.getInputStream()));
            PrintWriter salida    = new PrintWriter(cliente.getOutputStream(), true);

            String lineaJson = entrada.readLine();
            System.out.println("[SERVIDOR] JSON recibido: " + lineaJson);

            String resultado = parsearJson(lineaJson);
            System.out.println("[SERVIDOR] Resultado del parser:\n" + resultado);

            String respuesta = "{\"tipo\":\"ESTADO_JUEGO\",\"vidas\":3,\"puntuacion\":0,\"ronda\":1,\"velocidad\":1.0}";
            salida.println(respuesta);
            System.out.println("[SERVIDOR] Respuesta enviada: " + respuesta);

            cliente.close();
            System.out.println("[SERVIDOR] Prueba completada exitosamente.");
        }
    }

    /* Parser JSON minimalista sin dependencias externas */
    private static String parsearJson(String json) {
        if (json == null || json.isEmpty()) return "JSON vacio";

        StringBuilder resultado = new StringBuilder();
        json = json.trim().replaceAll("[{}\"]", "");
        String[] pares = json.split(",");

        for (String par : pares) {
            String[] kv = par.split(":", 2);
            if (kv.length == 2) {
                String clave = kv[0].trim();
                String valor = kv[1].trim();
                resultado.append("  ").append(clave).append(" = ").append(valor).append("\n");
            }
        }
        return resultado.toString();
    }
}
