package com.spaceinvaders.server;

import com.spaceinvaders.model.EstadoJuego;
import com.spaceinvaders.network.ManejadorCliente;
import com.spaceinvaders.network.ManejadorEspectador;
import com.spaceinvaders.observer.GameStateNotifier;

import javax.swing.SwingUtilities;
import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentSkipListSet;

public class ServidorJuego {
    public static final int PUERTO_JUGADORES    = 8080;
    public static final int PUERTO_ESPECTADORES = 8081;

    /* Sesiones independientes: cada jugador tiene su propio motor y notificador */
    private final Map<Integer, MotorJuego>        motores       = new ConcurrentHashMap<>();
    private final Map<Integer, GameStateNotifier> notificadores = new ConcurrentHashMap<>();

    private final List<ManejadorCliente>    jugadores    = new ArrayList<>();
    private final List<ManejadorEspectador> espectadores = new ArrayList<>();
    private final Set<Integer> idsEnUso = new ConcurrentSkipListSet<>();

    private ConsolaAdmin consolaAdmin = null;

    public void setConsolaAdmin(ConsolaAdmin c) { consolaAdmin = c; }

    public Map<Integer, MotorJuego> getMotores() {
        return Collections.unmodifiableMap(motores);
    }

    /* Asigna el ID más bajo disponible (reutiliza IDs liberados por desconexión) */
    private synchronized int asignarId() {
        int id = 1;
        while (idsEnUso.contains(id)) id++;
        idsEnUso.add(id);
        return id;
    }

    /* Libera un ID para que pueda ser reutilizado por una futura conexión */
    private synchronized void liberarId(int id) {
        idsEnUso.remove(id);
    }

    /* Suscribe al espectador solo a la sesión que eligió durante el handshake */
    public synchronized void suscribirEspectador(ManejadorEspectador esp, int idSesion) {
        GameStateNotifier notificador = notificadores.get(idSesion);
        if (notificador != null) {
            notificador.agregarObservador(esp);
            esp.setMotorEspectado(motores.get(idSesion)); /* velocidad + balas alien */
            log(String.format("[Servidor] Espectador %d observando sesión %d",
                    esp.obtenerIdCliente(), idSesion));
        } else {
            log(String.format("[Servidor] Espectador %d: sesión %d no existe",
                    esp.obtenerIdCliente(), idSesion));
        }
    }

    public void enviarComandoAdmin(String cmd) {
        if (motores.isEmpty()) {
            log("[Admin] No hay partidas activas.");
        } else {
            for (MotorJuego motor : motores.values()) motor.procesarComandoAdmin(cmd);
            log("[Admin] (todas) " + cmd);
        }
    }

    public void enviarComandoAdminASesion(int idSesion, String cmd) {
        MotorJuego motor = motores.get(idSesion);
        if (motor == null) {
            log("[Admin] Sesión " + idSesion + " no existe.");
            return;
        }
        motor.procesarComandoAdmin(cmd);
        log("[Admin] Sesión " + idSesion + " ← " + cmd);
    }

    private void log(String msg) {
        System.out.println(msg);
        if (consolaAdmin != null) consolaAdmin.logEvento(msg);
    }

    public void iniciar() throws IOException {
        new Thread(this::aceptarJugadores,    "hilo-jugadores").start();
        new Thread(this::aceptarEspectadores, "hilo-espectadores").start();
        new Thread(this::leerComandosAdmin,   "hilo-admin").start();
        System.out.println("Servidor iniciado — jugadores:" + PUERTO_JUGADORES
                + " espectadores:" + PUERTO_ESPECTADORES);
        System.out.println("Comandos admin: Crear X Y Pts | OVNI I-D Pts | Velocidad V | Bunkers P");
    }

    /* ----------------------------------------------------------------
       Crea una sesión de juego exclusiva para este jugador.
       Cada conexión obtiene su propio EstadoJuego + MotorJuego nuevos.
       ---------------------------------------------------------------- */
    private synchronized void iniciarSesion(ManejadorCliente manejador) {
        int id = manejador.obtenerIdCliente();

        EstadoJuego       estado      = new EstadoJuego();
        GameStateNotifier notificador = new GameStateNotifier(estado);
        MotorJuego        motor       = new MotorJuego(estado, notificador);

        motores.put(id, motor);
        notificadores.put(id, notificador);

        manejador.establecerMotor(motor);
        manejador.establecerServidor(this);
        notificador.agregarObservador(manejador);
        /* Los espectadores eligen su sesión explícitamente — no auto-suscribir */

        motor.iniciar();
        log(String.format("[Servidor] Sesión iniciada para jugador %d", id));
    }

    /* ----------------------------------------------------------------
       Desconexión limpia.
       - Jugador: detiene SOLO SU motor; no afecta otras sesiones.
       - Espectador: sale de todos los notificadores activos.
       ---------------------------------------------------------------- */
    public synchronized void desconectarJugador(ManejadorCliente manejador) {
        int id = manejador.obtenerIdCliente();

        if (jugadores.remove(manejador)) {
            GameStateNotifier notificador = notificadores.remove(id);
            if (notificador != null) notificador.eliminarObservador(manejador);

            MotorJuego motor = motores.remove(id);
            if (motor != null) motor.detener();

            liberarId(id);
            log(String.format("[Servidor] Jugador %d desconectado. Su partida terminó.", id));

        } else if (espectadores.remove(manejador)) {
            for (GameStateNotifier notificador : notificadores.values()) {
                notificador.eliminarObservador(manejador);
            }
            liberarId(id);
            log(String.format("[Servidor] Espectador %d desconectado.", id));
        }
    }

    /* ----------------------------------------------------------------
       Consola admin (stdin) — sigue activa como fallback junto a la GUI
       ---------------------------------------------------------------- */
    private void leerComandosAdmin() {
        try (java.io.BufferedReader consola = new java.io.BufferedReader(
                new java.io.InputStreamReader(System.in))) {
            String linea;
            while ((linea = consola.readLine()) != null) {
                enviarComandoAdmin(linea);
            }
        } catch (java.io.IOException e) {
            System.err.println("Error leyendo consola admin: " + e.getMessage());
        }
    }

    /* ----------------------------------------------------------------
       Aceptar conexiones
       ---------------------------------------------------------------- */
    private void aceptarJugadores() {
        try (ServerSocket socketServidor = new ServerSocket(PUERTO_JUGADORES)) {
            System.out.println("[Servidor] Esperando jugadores en puerto " + PUERTO_JUGADORES);
            while (true) {
                Socket socket = socketServidor.accept();
                log(String.format("[Servidor] Jugador conectado desde %s:%d",
                        socket.getInetAddress().getHostAddress(), socket.getPort()));
                ManejadorCliente manejador = new ManejadorCliente(socket, asignarId());
                synchronized (this) { jugadores.add(manejador); }
                iniciarSesion(manejador);
                new Thread(manejador, "jugador-" + manejador.obtenerIdCliente()).start();
            }
        } catch (IOException e) {
            System.err.println("Error en socket de jugadores: " + e.getMessage());
        }
    }

    private void aceptarEspectadores() {
        try (ServerSocket socketServidor = new ServerSocket(PUERTO_ESPECTADORES)) {
            System.out.println("[Servidor] Esperando espectadores en puerto " + PUERTO_ESPECTADORES);
            while (true) {
                Socket socket = socketServidor.accept();
                log(String.format("[Servidor] Espectador conectado desde %s:%d",
                        socket.getInetAddress().getHostAddress(), socket.getPort()));
                ManejadorEspectador manejador = new ManejadorEspectador(socket, asignarId());
                manejador.establecerServidor(this);
                synchronized (this) { espectadores.add(manejador); }
                /* El espectador hace handshake en su propio hilo (elige sesión) */
                new Thread(manejador, "espectador-" + manejador.obtenerIdCliente()).start();
            }
        } catch (IOException e) {
            System.err.println("Error en socket de espectadores: " + e.getMessage());
        }
    }

    public static void main(String[] args) throws IOException {
        ServidorJuego servidor = new ServidorJuego();
        SwingUtilities.invokeLater(() -> {
            ConsolaAdmin consola = new ConsolaAdmin(servidor);
            servidor.setConsolaAdmin(consola);
        });
        servidor.iniciar();
    }
}
