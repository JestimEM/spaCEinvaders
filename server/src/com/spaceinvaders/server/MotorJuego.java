package com.spaceinvaders.server;

import com.spaceinvaders.factory.*;
import com.spaceinvaders.model.*;
import com.spaceinvaders.observer.GameStateNotifier;

import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

public class MotorJuego {

    /* ---- Constantes estructurales (no modificar) ---- */
    static final int   ANCHO           = 100;
    static final int   Y_JUGADOR       = 90;
    private static final int   Y_LIMITE_ALIENS = 82;
    private static final int   RADIO_COLISION  = 2;
    private static final int   INTERVALO_MS    = 50;

    /* ================================================================
       CONFIGURACIÓN DE FÍSICA — modificar aquí para ajustar el juego
       ================================================================ */
    private int    velocidadBala       = 2;    // units/tick — bala del jugador sube
    private int    velocidadBalaAlien  = 2;    // units/tick — bala alien baja
    private int    pasoCanon           = 2;    // units por pulsación de tecla
    private int    pasoDescenso        = 1;    // units que bajan los aliens al rebotar
    private volatile double velocidadActual = 0.1;  // escala ×1.2 por ronda; ajustable por admin (>0)
    private long   intervaloOvniMs     = 20_000L; // ms entre spawns de OVNI
    private long   intervaloDisparoMs  = 1_500L;  // ms entre disparos alien
    /* ================================================================ */

    /* ================================================================
       FORMACIÓN INICIAL DE OLEADA (5 filas × 11 columnas = 55 aliens)
       ================================================================ */
    private static final int    FILAS_OLEADA        = 5;
    private static final int    COLS_OLEADA         = 11;
    private static final int    X_INICIO_OLEADA     = 15; // (100 - 10×7) / 2 = 15 → formación centrada
    private static final int    Y_INICIO_OLEADA     = 10;
    private static final int    PASO_X_OLEADA       = 7;  // reducido de 9 → formación más compacta
    private static final int    PASO_Y_OLEADA       = 8;
    /* ================================================================ */

    /* Posiciones X de los bunkers en coordenadas de juego */
    private static final int[]  BUNKER_X   = {12, 37, 62, 87};
    private static final int    BUNKER_Y   = 70;
    private static final int    BUNKER_R   = 6;

    /* ---- Estado ---- */
    private final EstadoJuego estado;
    private final GameStateNotifier notificador;
    private final ScheduledExecutorService executor = Executors.newSingleThreadScheduledExecutor();

    private int  direccion = 1;        // 1=derecha, -1=izquierda
    private int  contadorId = 100;
    private volatile boolean juegoActivo = false;
    private boolean rondaEnTransicion = false;

    /* Balas por jugador */
    private final Map<Integer, BalaJugador> balas = new ConcurrentHashMap<>();

    /* Balas alienígenas */
    private final List<BalaAlien> balasAlien = new ArrayList<>();
    private long   ultimoDisparoAlien = 0;
    private double acumuladorAlien    = 0.0;
    private double acumuladorOvni     = 0.0;

    /* OVNI timer */
    private long ultimoSpawnOvni = System.currentTimeMillis();

    public MotorJuego(EstadoJuego estado, GameStateNotifier notificador) {
        this.estado      = estado;
        this.notificador = notificador;
    }

    /* ================================================================
       Ciclo principal
       ================================================================ */
    public void iniciar() {
        juegoActivo = true;
        crearOleadaInicial();
        executor.scheduleAtFixedRate(this::tick, 0, INTERVALO_MS, TimeUnit.MILLISECONDS);
        System.out.println("[Motor] Game loop iniciado (" + INTERVALO_MS + "ms/tick).");
    }

    public void detener() {
        juegoActivo = false;
        executor.shutdown();
        try { executor.awaitTermination(200, TimeUnit.MILLISECONDS); } catch (InterruptedException ignored) {}
        System.out.println("[Motor] Detenido.");
    }

    private synchronized void tick() {
        if (!juegoActivo) return;
        moverExtraterrestres();
        moverBalasJugadores();
        moverBalasAlien();
        disparoAlienAleatorio();
        verificarSpawnOvni();
        moverOvni();
        verificarColisiones();
        verificarCondicionFin();
        notificador.notificarCambioEstado();
    }

    /* ================================================================
       Movimiento de extraterrestres (formación — rebote por extremos)
       ================================================================ */
    private void moverExtraterrestres() {
        List<Extraterrestre> vivos = obtenerVivos();
        if (vivos.isEmpty()) return;

        acumuladorAlien += velocidadActual;
        if (acumuladorAlien < 1.0) return;
        int movimiento = (int) acumuladorAlien;
        acumuladorAlien -= movimiento;
        int paso = direccion * movimiento;

        int minX = Integer.MAX_VALUE, maxX = Integer.MIN_VALUE;
        for (Extraterrestre e : vivos) {
            minX = Math.min(minX, e.obtenerX());
            maxX = Math.max(maxX, e.obtenerX());
        }

        boolean rebotar = (paso > 0 && maxX + paso >= ANCHO - 3) ||
                          (paso < 0 && minX + paso <= 3);

        if (rebotar) {
            direccion = -direccion;
            acumuladorAlien = 0.0;
            for (Extraterrestre e : vivos) {
                e.establecerY(e.obtenerY() + pasoDescenso);
                e.establecerX(Math.max(4, Math.min(ANCHO - 4, e.obtenerX())));
            }
        } else {
            for (Extraterrestre e : vivos) e.establecerX(e.obtenerX() + paso);
        }
    }

    /* ================================================================
       Balas de jugadores
       ================================================================ */
    private void moverBalasJugadores() {
        for (BalaJugador b : balas.values()) {
            if (!b.isActiva()) continue;
            b.setY(b.getY() - velocidadBala);
            if (b.getY() < 0) b.desactivar();
        }
    }

    /* ================================================================
       Balas alienígenas — disparo y movimiento
       ================================================================ */
    private void disparoAlienAleatorio() {
        long ahora = System.currentTimeMillis();
        if (ahora - ultimoDisparoAlien < intervaloDisparoMs) return;

        List<Extraterrestre> tiradores = obtenerTiradores();
        if (tiradores.isEmpty()) return;

        Extraterrestre tirador = tiradores.get((int)(Math.random() * tiradores.size()));
        balasAlien.add(new BalaAlien(tirador.obtenerX(), tirador.obtenerY()));
        ultimoDisparoAlien = ahora;
    }

    private void moverBalasAlien() {
        balasAlien.removeIf(b -> !b.isActiva());
        for (BalaAlien b : balasAlien) {
            b.setY(b.getY() + velocidadBalaAlien);
            if (b.getY() > Y_JUGADOR + 5) b.desactivar();
        }
    }

    /* Retorna el alien más bajo en cada columna (única línea de fuego) */
    private List<Extraterrestre> obtenerTiradores() {
        Map<Integer, Extraterrestre> porColumna = new TreeMap<>();
        for (Extraterrestre e : obtenerVivos()) {
            int col = e.obtenerX() / 8;
            porColumna.merge(col, e, (a, b) -> a.obtenerY() > b.obtenerY() ? a : b);
        }
        return new ArrayList<>(porColumna.values());
    }

    /* ================================================================
       OVNI — auto-spawn periódico
       ================================================================ */
    private void verificarSpawnOvni() {
        Ovni ovni = estado.obtenerOvni();
        if (ovni.estaActivo()) return;
        long ahora = System.currentTimeMillis();
        if (ahora - ultimoSpawnOvni >= intervaloOvniMs) {
            Ovni.Direccion dir = (Math.random() < 0.5)
                    ? Ovni.Direccion.IZQUIERDA_DERECHA
                    : Ovni.Direccion.DERECHA_IZQUIERDA;
            int pts = 100 + (int)(Math.random() * 201); // 100-300 pts
            ovni.aparecer(dir, pts);
            ultimoSpawnOvni = ahora;
            System.out.println("[Motor] OVNI auto-spawn dir=" + dir + " pts=" + pts);
        }
    }

    private void moverOvni() {
        Ovni ovni = estado.obtenerOvni();
        if (!ovni.estaActivo()) return;
        acumuladorOvni += velocidadActual;
        if (acumuladorOvni < 1.0) return;
        int mov = (int) acumuladorOvni;
        acumuladorOvni -= mov;
        int nx = ovni.obtenerX() +
                 (ovni.obtenerDireccion() == Ovni.Direccion.IZQUIERDA_DERECHA ? mov : -mov);
        if (nx < 0 || nx > ANCHO) { ovni.desactivar(); acumuladorOvni = 0.0; }
        else ovni.establecerX(nx);
    }

    /* ================================================================
       Colisiones
       ================================================================ */
    private void verificarColisiones() {
        /* Balas de jugadores */
        for (Map.Entry<Integer, BalaJugador> entry : balas.entrySet()) {
            int idCliente = entry.getKey();
            BalaJugador bala = entry.getValue();
            if (!bala.isActiva()) continue;

            /* vs aliens */
            for (Extraterrestre e : obtenerVivos()) {
                if (distancia(e.obtenerX(), e.obtenerY(), bala.getX(), bala.getY()) <= RADIO_COLISION) {
                    e.destruir();
                    bala.desactivar();
                    estado.obtenerJugador(idCliente).agregarPuntuacion(e.obtenerPuntos());
                    notificador.notificarExtraterrestreEliminado(e.obtenerId(), e.obtenerPuntos());
                    return;
                }
            }

            /* vs OVNI */
            Ovni ovni = estado.obtenerOvni();
            if (ovni.estaActivo() &&
                distancia(ovni.obtenerX(), 10, bala.getX(), bala.getY()) <= RADIO_COLISION) {
                ovni.desactivar();
                bala.desactivar();
                estado.obtenerJugador(idCliente).agregarPuntuacion(ovni.obtenerPuntos());
            }

            /* vs bunkers (bala del jugador destruye bunkers) */
            if (bala.isActiva()) {
                for (Bunker b : estado.obtenerBunkers()) {
                    if (!b.estaDestruido() && puntoEnBunker(b, bala.getX(), bala.getY())) {
                        b.recibirImpacto();
                        bala.desactivar();
                        break;
                    }
                }
            }
        }

        /* Aliens vivos vs bunkers — al contacto el bunker se destruye */
        for (Extraterrestre e : obtenerVivos()) {
            for (Bunker b : estado.obtenerBunkers()) {
                if (!b.estaDestruido() && puntoEnBunker(b, e.obtenerX(), e.obtenerY())) {
                    while (!b.estaDestruido()) b.recibirImpacto();
                    System.out.println("[Motor] Bunker " + b.obtenerId() +
                            " destruido por alien " + e.obtenerId());
                }
            }
        }

        /* Balas alienígenas vs bunkers y jugadores */
        for (BalaAlien ba : balasAlien) {
            if (!ba.isActiva()) continue;

            /* vs bunkers */
            for (Bunker b : estado.obtenerBunkers()) {
                if (!b.estaDestruido() && puntoEnBunker(b, ba.getX(), ba.getY())) {
                    b.recibirImpacto();
                    ba.desactivar();
                    break;
                }
            }

            /* vs jugadores */
            if (ba.isActiva()) {
                for (Map.Entry<Integer, Jugador> entry : estado.obtenerJugadores().entrySet()) {
                    Jugador j = entry.getValue();
                    if (Math.abs(ba.getX() - j.obtenerPosicionCanon()) <= RADIO_COLISION &&
                        Math.abs(ba.getY() - Y_JUGADOR) <= RADIO_COLISION) {
                        j.perderVida();
                        ba.desactivar();
                        System.out.println("[Motor] Jugador " + entry.getKey() +
                                " impactado. Vidas: " + j.obtenerVidas());
                        if (!j.estaVivo()) {
                            juegoActivo = false;
                            notificador.notificarFinJuego(j.obtenerPuntuacion());
                            System.out.println("[Motor] Jugador " + entry.getKey() +
                                    " sin vidas. GAME OVER. Pts=" + j.obtenerPuntuacion());
                        }
                        break;
                    }
                }
            }
        }
    }

    /* ================================================================
       Condición fin de ronda / fin de juego
       ================================================================ */
    private void verificarCondicionFin() {
        if (rondaEnTransicion) return;

        List<Extraterrestre> todos = estado.obtenerExtraterrestres();
        List<Extraterrestre> vivos = obtenerVivos();

        if (!todos.isEmpty() && vivos.isEmpty()) {
            rondaEnTransicion = true;
            /* Todos los jugadores ganan una vida al completar la ronda */
            for (Jugador j : estado.obtenerJugadores().values()) j.ganarVida();
            estado.siguienteRonda();
            velocidadActual *= 1.2;
            estado.obtenerExtraterrestres().clear();
            balasAlien.clear();
            crearOleadaInicial();
            notificador.notificarRondaCompleta(estado.obtenerRonda());
            System.out.println("[Motor] Ronda completa. Nueva ronda: " + estado.obtenerRonda() +
                    " vel=" + String.format("%.2f", velocidadActual));
            return;
        }

        /* Algún alien llegó al límite inferior */
        for (Extraterrestre e : vivos) {
            if (e.obtenerY() >= Y_LIMITE_ALIENS) {
                e.destruir();
                for (Map.Entry<Integer, Jugador> entry : estado.obtenerJugadores().entrySet()) {
                    Jugador j = entry.getValue();
                    j.perderVida();
                    System.out.println("[Motor] Alien alcanzó base. Jugador " + entry.getKey() +
                            " vidas=" + j.obtenerVidas());
                    if (!j.estaVivo()) {
                        juegoActivo = false;
                        notificador.notificarFinJuego(j.obtenerPuntuacion());
                        System.out.println("[Motor] GAME OVER. Pts=" + j.obtenerPuntuacion());
                    }
                }
                break;
            }
        }
    }

    /* ================================================================
       Comandos admin
       ================================================================ */
    public synchronized void procesarComandoAdmin(String comando) {
        if (comando == null || comando.isBlank()) return;
        String[] p = comando.trim().split("\\s+");
        String tipo = p[0].toUpperCase();
        try {
            switch (tipo) {
                case "CREAR": {
                    int x         = Integer.parseInt(p[1]);
                    int y         = Integer.parseInt(p[2]);
                    int pts       = (p.length > 3) ? Integer.parseInt(p[3]) : 10;
                    String tipoAlien = (p.length > 4) ? p[4].toUpperCase() : "CALAMAR";
                    crearExtraterrestre(x, y, pts, tipoAlien);
                    System.out.println("[Admin] Alien creado en (" + x + "," + y + ") pts=" + pts + " tipo=" + tipoAlien);
                    break;
                }
                case "OVNI": {
                    Ovni.Direccion dir = p[1].equalsIgnoreCase("I-D")
                            ? Ovni.Direccion.IZQUIERDA_DERECHA
                            : Ovni.Direccion.DERECHA_IZQUIERDA;
                    int pts = Integer.parseInt(p[2]);
                    estado.obtenerOvni().aparecer(dir, pts);
                    ultimoSpawnOvni = System.currentTimeMillis();
                    System.out.println("[Admin] OVNI creado dir=" + p[1] + " pts=" + pts);
                    break;
                }
                case "VELOCIDAD": {
                    double vel = Double.parseDouble(p[1]);
                    if (vel <= 0) {
                        System.err.println("[Motor] Velocidad rechazada: debe ser > 0 (recibido: " + vel + ")");
                        return;
                    }
                    velocidadActual = vel;
                    System.out.println("[Admin] Velocidad=" + vel);
                    break;
                }
                case "BUNKERS": {
                    int pct = Integer.parseInt(p[1]);
                    for (Bunker b : estado.obtenerBunkers()) b.establecerSalud(pct);
                    System.out.println("[Admin] Bunkers al " + pct + "%");
                    break;
                }
                case "CREARENMATRIZ": {
                    int fila     = Integer.parseInt(p[1]);
                    int col      = Integer.parseInt(p[2]);
                    int pts      = (p.length > 3) ? Integer.parseInt(p[3]) : 10;
                    String tipoAlien = (p.length > 4) ? p[4].toUpperCase() : "CALAMAR";
                    if (fila < 0 || fila >= FILAS_OLEADA || col < 0 || col >= COLS_OLEADA) {
                        System.err.println("[Motor] Posición inválida: fila=" + fila + " col=" + col);
                        break;
                    }
                    int matrizId = 100 + fila * COLS_OLEADA + col;
                    int[] origen = calcularOrigenFormacion();
                    int gameX    = origen[0] + col * PASO_X_OLEADA;
                    int gameY    = origen[1] + fila * PASO_Y_OLEADA;
                    estado.obtenerExtraterrestres().removeIf(e -> e.obtenerId() == matrizId);
                    crearExtraterrestreConId(matrizId, gameX, gameY, pts, tipoAlien);
                    System.out.println("[Admin] Alien en matriz [" + fila + "][" + col + "] id=" + matrizId + " game(" + gameX + "," + gameY + ")");
                    break;
                }
                case "AUMENTARVIDA": {
                    int idSesion = Integer.parseInt(p[1]);
                    Jugador j = estado.obtenerJugadores().get(idSesion);
                    if (j == null) {
                        System.err.println("[Motor] Sesión " + idSesion + " no existe.");
                        break;
                    }
                    j.ganarVida();
                    System.out.println("[Admin] Jugador " + idSesion + " vidas=" + j.obtenerVidas());
                    break;
                }
                case "ELIMINARALIEN": {
                    int id = Integer.parseInt(p[1]);
                    boolean encontrado = false;
                    for (Extraterrestre e : estado.obtenerExtraterrestres()) {
                        if (e.obtenerId() == id && e.estaVivo()) {
                            e.destruir();
                            encontrado = true;
                            System.out.println("[Admin] Alien " + id + " eliminado.");
                            break;
                        }
                    }
                    if (!encontrado)
                        System.err.println("[Motor] Alien " + id + " no encontrado o ya muerto.");
                    break;
                }
                case "REDUCIRVIDA": {
                    int idSesion = Integer.parseInt(p[1]);
                    Jugador j = estado.obtenerJugadores().get(idSesion);
                    if (j == null) {
                        System.err.println("[Motor] Sesión " + idSesion + " no existe.");
                        break;
                    }
                    j.perderVida();
                    System.out.println("[Admin] Jugador " + idSesion + " vidas=" + j.obtenerVidas());
                    if (!j.estaVivo()) {
                        juegoActivo = false;
                        notificador.notificarFinJuego(j.obtenerPuntuacion());
                        System.out.println("[Motor] GAME OVER por admin. Pts=" + j.obtenerPuntuacion());
                    }
                    break;
                }
                default:
                    System.err.println("[Motor] Comando desconocido: " + comando);
                    return;
            }
        } catch (NumberFormatException | ArrayIndexOutOfBoundsException e) {
            System.err.println("[Motor] Formato inválido: '" + comando + "' — " + e.getMessage());
            return;
        }
        notificador.notificarCambioEstado();
    }

    /* ================================================================
       Mensajes de clientes
       ================================================================ */
    public synchronized void procesarMensajeCliente(int idCliente, String json) {
        String tipo = extraerCampo(json, "tipo");
        if (tipo == null) return;

        switch (tipo) {
            case "DISPARO": {
                BalaJugador b = getBalaJugador(idCliente);
                if (!b.isActiva()) {
                    b.activar(estado.obtenerJugador(idCliente).obtenerPosicionCanon(), Y_JUGADOR - 5);
                    notificador.notificarCambioEstado(); // feedback inmediato — cliente ve balaActiva:true sin esperar tick
                }
                break;
            }
            /* Compatibilidad MOVER_CANON para versiones antiguas del cliente */
            case "MOVER_CANON": {
                String dir = extraerCampo(json, "direccion");
                Jugador j = estado.obtenerJugador(idCliente);
                if ("IZQUIERDA".equalsIgnoreCase(dir))
                    j.establecerPosicionCanon(Math.max(0, j.obtenerPosicionCanon() - pasoCanon));
                else if ("DERECHA".equalsIgnoreCase(dir))
                    j.establecerPosicionCanon(Math.min(ANCHO, j.obtenerPosicionCanon() + pasoCanon));
                notificador.notificarCambioEstado();
                break;
            }
            default:
                System.err.println("[Motor] Tipo desconocido de cliente " + idCliente + ": " + tipo);
        }
    }

    /* ================================================================
       Gestión de jugadores (conectar/desconectar)
       ================================================================ */
    public synchronized void limpiarJugador(int idCliente) {
        estado.eliminarJugador(idCliente);
        balas.remove(idCliente);
        System.out.println("[Motor] Estado de jugador " + idCliente + " limpiado.");
    }

    /* ================================================================
       Acceso público a balas (para AdaptadorMensaje)
       ================================================================ */
    public BalaJugador getBalaJugador(int idCliente) {
        return balas.computeIfAbsent(idCliente, k -> new BalaJugador());
    }

    public List<BalaAlien> getBalasAlien() {
        return Collections.unmodifiableList(balasAlien);
    }

    /* ================================================================
       Helpers
       ================================================================ */

    /* Spawna la formación completa: fila 0=Pulpos, 1-2=Cangrejos, 3-4=Calamares */
    private void crearOleadaInicial() {
        contadorId = 100; // IDs fijos por oleada: fila0=100-110, fila1=111-121, etc.
        for (int fila = 0; fila < FILAS_OLEADA; fila++) {
            String tipo;
            int pts;
            if (fila == 0)       { tipo = "PULPO";    pts = 40; }
            else if (fila <= 2)  { tipo = "CANGREJO"; pts = 20; }
            else                 { tipo = "CALAMAR";  pts = 10; }
            for (int col = 0; col < COLS_OLEADA; col++) {
                int x = X_INICIO_OLEADA + col * PASO_X_OLEADA;
                int y = Y_INICIO_OLEADA + fila * PASO_Y_OLEADA;
                crearExtraterrestre(x, y, pts, tipo);
            }
        }
        direccion = 1;
        acumuladorAlien = 0.0;
    }

    private void crearExtraterrestre(int x, int y, int pts, String tipo) {
        crearExtraterrestreConId(contadorId++, x, y, pts, tipo);
    }

    /* Calcula el origen actual de la formación tomando cualquier alien vivo de referencia.
       Resta su offset relativo (col*PASO_X, fila*PASO_Y) para obtener el punto base. */
    private int[] calcularOrigenFormacion() {
        for (Extraterrestre e : estado.obtenerExtraterrestres()) {
            if (!e.estaVivo()) continue;
            int offset = e.obtenerId() - 100;
            if (offset < 0 || offset >= FILAS_OLEADA * COLS_OLEADA) continue;
            int f = offset / COLS_OLEADA;
            int c = offset % COLS_OLEADA;
            return new int[]{ e.obtenerX() - c * PASO_X_OLEADA,
                              e.obtenerY() - f * PASO_Y_OLEADA };
        }
        return new int[]{ X_INICIO_OLEADA, Y_INICIO_OLEADA };
    }

    private void crearExtraterrestreConId(int id, int x, int y, int pts, String tipo) {
        ExtraterrestreFactory factory;
        switch (tipo) {
            case "CANGREJO": factory = new CangrejoFactory(); break;
            case "PULPO":    factory = new PulpoFactory();    break;
            default:         factory = new CalamarFactory();  break;
        }
        estado.agregarExtraterrestre(factory.crearExtraterrestre(id, x, y, pts));
        rondaEnTransicion = false;
    }

    private List<Extraterrestre> obtenerVivos() {
        List<Extraterrestre> vivos = new ArrayList<>();
        for (Extraterrestre e : estado.obtenerExtraterrestres())
            if (e.estaVivo()) vivos.add(e);
        return vivos;
    }

    private boolean puntoEnBunker(Bunker b, int x, int y) {
        int bx = BUNKER_X[b.obtenerId()];
        return Math.abs(x - bx) <= BUNKER_R && Math.abs(y - BUNKER_Y) <= BUNKER_R;
    }

    private static int distancia(int x1, int y1, int x2, int y2) {
        return Math.max(Math.abs(x1 - x2), Math.abs(y1 - y2));
    }

    private static String extraerCampo(String json, String clave) {
        String buscar = "\"" + clave + "\":";
        int idx = json.indexOf(buscar);
        if (idx < 0) return null;
        idx += buscar.length();
        while (idx < json.length() && json.charAt(idx) == ' ') idx++;
        if (idx >= json.length()) return null;
        boolean entreComillas = json.charAt(idx) == '"';
        if (entreComillas) idx++;
        StringBuilder sb = new StringBuilder();
        while (idx < json.length()) {
            char c = json.charAt(idx);
            if (entreComillas && c == '"') break;
            if (!entreComillas && (c == ',' || c == '}')) break;
            sb.append(c);
            idx++;
        }
        return sb.length() > 0 ? sb.toString() : null;
    }

    public boolean isJuegoActivo() { return juegoActivo; }

    public EstadoJuego getEstado()        { return estado; }
    public double      getVelocidadActual() { return velocidadActual; }
}
