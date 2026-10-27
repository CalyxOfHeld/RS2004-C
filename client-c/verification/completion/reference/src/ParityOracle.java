import deob.client;
import sign.signlink;
import jagex2.io.ClientStream;
import jagex2.graphics.PixMap;
import com.sun.net.httpserver.HttpServer;
import java.applet.AppletContext;
import java.applet.AppletStub;
import java.awt.*;
import java.awt.event.InputEvent;
import java.awt.event.KeyEvent;
import java.awt.event.MouseEvent;
import java.awt.image.BufferedImage;
import java.awt.image.ImageObserver;
import java.io.*;
import java.lang.reflect.Field;
import java.net.*;
import java.nio.file.*;
import java.text.AttributedCharacterIterator;
import java.util.*;
import java.util.List;
import java.util.zip.CRC32;
import javax.imageio.ImageIO;

// Differential oracle: drives the unchanged revision-225 client through a
// session recorded by `rs2client --record`, cycle for cycle.
//
//   java -cp "rs2client.jar;classes" ParityOracle <session.rec> <pack dir> <out dir> <random state>
//        [--dump-frame N]...
//
// - Archives come from <pack dir> over a local HTTP server (as engine/src/web.ts serves them).
// - Each frame applies the frame's input (recorded OS events and the input script),
//   then one update() per recorded cycle, then draw(), as GameShell.run does.
// - Inbound bytes are released per cycle exactly as the C client read them.
// - Math.random is set to the C client's state at login (<random state>, printed by
//   the C client as "[PARITY] login ... random_state="), and signlink.uid to the C value 1.
// - The login packet and every frame's outgoing bytes are compared with the recording.
// - The composited screen is captured by intercepting drawImage; every frame's hash of each
//   drawn PixMap rectangle is written to java-regions.txt, rectangles to regions.txt.
//
// RS2_PASSWORD supplies the password. Comparison stops when the client leaves the game
// (logout restarts the unseeded flame thread).
public class ParityOracle {
    static final int W = 789, H = 532;

    // ---------------------------------------------------------------- reflection
    static Field field(Class<?> start, String name) throws Exception {
        for (Class<?> c = start; c != null; c = c.getSuperclass()) {
            try { Field f = c.getDeclaredField(name); f.setAccessible(true); return f; } catch (NoSuchFieldException e) { }
        }
        throw new NoSuchFieldException(name);
    }
    static Object get(Object o, String name) throws Exception { return field(o.getClass(), name).get(o); }
    static void set(Object o, String name, Object v) throws Exception { field(o.getClass(), name).set(o, v); }
    static int getInt(Object o, String name) throws Exception { return field(o.getClass(), name).getInt(o); }
    static void setInt(Object o, String name, int v) throws Exception { field(o.getClass(), name).setInt(o, v); }

    // ---------------------------------------------------------------- session log
    static final class Cycle { ByteArrayOutputStream in = new ByteArrayOutputStream(); }
    static final class Frame {
        int number; long hash;
        List<long[]> events = new ArrayList<>(); List<String> texts = new ArrayList<>();
        Integer held;
        List<Cycle> cycles = new ArrayList<>();
        ByteArrayOutputStream out = new ByteArrayOutputStream();     // NX outside login
        ByteArrayOutputStream lateIn = new ByteArrayOutputStream();  // NR/NB outside cycles and login
        boolean login, close;
        ByteArrayOutputStream loginIn = new ByteArrayOutputStream(), loginOut = new ByteArrayOutputStream();
    }

    static void requireSupportedTrace() {
        if (frames.stream().filter(frame -> frame.login).count() > 1 ||
            frames.stream().anyMatch(frame -> frame.login && frame.loginOut.size() > 0 &&
                (frame.loginOut.toByteArray()[0] & 255) == 18))
            throw new UnsupportedOperationException("UNSUPPORTED RECOVERY TRACE: oracle has only initial-login support; reconnect/I-O/presentation/connection RNG require a recovery-aware adapter. No differential parity result is claimed.");
    }

    static byte[] hex(String s) {
        if (s.equals("-")) return new byte[0];
        byte[] b = new byte[s.length() / 2];
        for (int i = 0; i < b.length; i++) b[i] = (byte) Integer.parseInt(s.substring(2 * i, 2 * i + 2), 16);
        return b;
    }
    static String hex(byte[] b, int len) {
        StringBuilder s = new StringBuilder();
        for (int i = 0; i < len; i++) s.append(String.format("%02x", b[i] & 255));
        return s.toString();
    }

    static List<String> args = new ArrayList<>();
    static String username;
    static long startupSeed;
    static boolean hasStartupSeed;
    static List<Frame> frames = new ArrayList<>();

    static void parse(Path log) throws IOException {
        Frame f = null; Cycle cycle = null; boolean afterDraw = false, inLogin = false;
        for (String line : Files.readAllLines(log)) {
            if (line.isEmpty()) continue;
            String[] p = line.split(" ");
            switch (p[0]) {
                case "S": startupSeed = Long.parseLong(p[1]); hasStartupSeed = true; break;
                case "A": args.add(new String(hex(p[1]), "UTF-8")); break;
                case "L": username = new String(hex(p[1]), "UTF-8"); break;
                case "F": f = new Frame(); f.number = Integer.parseInt(p[1]); frames.add(f); cycle = null; afterDraw = false; inLogin = false; break;
                case "E":
                    f.events.add(new long[] { Long.parseLong(p[1]), Long.parseLong(p[2]), Long.parseLong(p[3]), Long.parseLong(p[4]), Long.parseLong(p[5]), Long.parseLong(p[6]) });
                    f.texts.add(p.length > 7 ? new String(hex(p[7]), "UTF-8") : "");
                    break;
                case "K": f.held = Integer.parseInt(p[1]); break;
                case "U": cycle = new Cycle(); f.cycles.add(cycle); break;
                case "D": f.hash = Long.parseLong(p[1]); cycle = null; afterDraw = true; break;   // hash logged signed
                case "NC":
                    if (f.login) throw new UnsupportedOperationException("UNSUPPORTED RECOVERY TRACE: multiple connection attempts in one frame require recovery-aware replay");
                    f.login = true; inLogin = true; break;
                case "NR": case "NB": {
                    byte[] data = hex(p[p.length - 1]);
                    (inLogin ? f.loginIn : cycle != null ? cycle.in : f.lateIn).write(data, 0, data.length);
                    break;
                }
                case "NX": {
                    byte[] data = hex(p[p.length - 1]);
                    (inLogin ? f.loginOut : f.out).write(data, 0, data.length);
                    break;
                }
                case "NZ": if (f != null) f.close = true; break;
                default: break; // RS2REC S E0 T NA NW Q
            }
        }
    }

    // ---------------------------------------------------------------- input script
    static final class Scripted { int frame; String kind, value; }
    static List<Scripted> script = new ArrayList<>();
    static void loadScript() throws IOException {
        int i = args.indexOf("--input-script");
        if (i < 0) return;
        for (String line : Files.readAllLines(Paths.get(args.get(i + 1)))) {
            if (line.isEmpty() || line.startsWith("#")) continue;
            String[] p = line.split(" ", 3);
            Scripted s = new Scripted(); s.frame = Integer.parseInt(p[0]); s.kind = p[1]; s.value = p.length > 2 ? p[2] : "";
            if (!Arrays.asList("move", "left", "right", "release", "key", "text", "quit").contains(s.kind))
                throw new IllegalStateException("input kind not supported by the oracle: " + s.kind);
            script.add(s);
        }
    }

    // ---------------------------------------------------------------- screen capture
    static final int[] screen = new int[W * H];
    // PixMap -> {x, y}. The title flame thread draws concurrently after logout, so
    // access is synchronized and readers take a snapshot.
    static final Map<PixMap, int[]> drawn = Collections.synchronizedMap(new LinkedHashMap<>());

    static final class CaptureGraphics extends Graphics {
        final Graphics base;
        CaptureGraphics(Graphics base) { this.base = base; }
        public boolean drawImage(Image img, int x, int y, ImageObserver observer) {
            if (!(observer instanceof PixMap)) return base.drawImage(img, x, y, observer);
            PixMap map = (PixMap) observer;
            drawn.putIfAbsent(map, new int[] { x, y });
            for (int row = 0; row < map.height; row++) {
                int sy = y + row; if (sy < 0 || sy >= H) continue;
                for (int col = 0; col < map.width; col++) {
                    int sx = x + col; if (sx < 0 || sx >= W) continue;
                    screen[sy * W + sx] = map.pixels[row * map.width + col];
                }
            }
            return true;
        }
        public Graphics create() { return this; }
        public void translate(int x, int y) { base.translate(x, y); }
        public Color getColor() { return base.getColor(); }
        public void setColor(Color c) { base.setColor(c); }
        public void setPaintMode() { base.setPaintMode(); }
        public void setXORMode(Color c) { base.setXORMode(c); }
        public Font getFont() { return base.getFont(); }
        public void setFont(Font font) { base.setFont(font); }
        public FontMetrics getFontMetrics(Font f) { return base.getFontMetrics(f); }
        public Rectangle getClipBounds() { return base.getClipBounds(); }
        public void clipRect(int x, int y, int w, int h) { base.clipRect(x, y, w, h); }
        public void setClip(int x, int y, int w, int h) { base.setClip(x, y, w, h); }
        public Shape getClip() { return base.getClip(); }
        public void setClip(Shape clip) { base.setClip(clip); }
        public void copyArea(int x, int y, int w, int h, int dx, int dy) { base.copyArea(x, y, w, h, dx, dy); }
        public void drawLine(int a, int b, int c, int d) { base.drawLine(a, b, c, d); }
        public void fillRect(int x, int y, int w, int h) { base.fillRect(x, y, w, h); }
        public void clearRect(int x, int y, int w, int h) { base.clearRect(x, y, w, h); }
        public void drawRoundRect(int x, int y, int w, int h, int aw, int ah) { base.drawRoundRect(x, y, w, h, aw, ah); }
        public void fillRoundRect(int x, int y, int w, int h, int aw, int ah) { base.fillRoundRect(x, y, w, h, aw, ah); }
        public void drawOval(int x, int y, int w, int h) { base.drawOval(x, y, w, h); }
        public void fillOval(int x, int y, int w, int h) { base.fillOval(x, y, w, h); }
        public void drawArc(int x, int y, int w, int h, int s, int a) { base.drawArc(x, y, w, h, s, a); }
        public void fillArc(int x, int y, int w, int h, int s, int a) { base.fillArc(x, y, w, h, s, a); }
        public void drawPolyline(int[] x, int[] y, int n) { base.drawPolyline(x, y, n); }
        public void drawPolygon(int[] x, int[] y, int n) { base.drawPolygon(x, y, n); }
        public void fillPolygon(int[] x, int[] y, int n) { base.fillPolygon(x, y, n); }
        public void drawString(String s, int x, int y) { base.drawString(s, x, y); }
        public void drawString(AttributedCharacterIterator it, int x, int y) { base.drawString(it, x, y); }
        public boolean drawImage(Image i, int x, int y, int w, int h, ImageObserver o) { return base.drawImage(i, x, y, w, h, o); }
        public boolean drawImage(Image i, int x, int y, Color c, ImageObserver o) { return base.drawImage(i, x, y, c, o); }
        public boolean drawImage(Image i, int x, int y, int w, int h, Color c, ImageObserver o) { return base.drawImage(i, x, y, w, h, c, o); }
        public boolean drawImage(Image i, int a, int b, int c, int d, int e, int f, int g, int h, ImageObserver o) { return base.drawImage(i, a, b, c, d, e, f, g, h, o); }
        public boolean drawImage(Image i, int a, int b, int c, int d, int e, int f, int g, int h, Color col, ImageObserver o) { return base.drawImage(i, a, b, c, d, e, f, g, h, col, o); }
        public void dispose() { }
    }

    // FNV-1a 64 over the low 24 bits of each pixel, little-endian, row by row (as rs2replay).
    static long regionHash(int x0, int y0, int w, int h) {
        long hash = 0xcbf29ce484222325L;
        for (int y = y0; y < y0 + h; y++)
            for (int x = x0; x < x0 + w; x++) {
                int p = screen[y * W + x] & 0xFFFFFF;
                for (int b = 0; b < 4; b++) { hash ^= (p >>> (8 * b)) & 0xFF; hash *= 0x100000001b3L; }
            }
        return hash;
    }

    // ---------------------------------------------------------------- network
    static final class MemoryIn extends InputStream {
        final ByteArrayOutputStream released = new ByteArrayOutputStream();
        byte[] data = new byte[0]; int pos; boolean overread;
        void release(ByteArrayOutputStream more) { byte[] b = more.toByteArray(); released.write(b, 0, b.length); data = released.toByteArray(); }
        public int available() { return data.length - pos; }
        public int read() throws IOException {
            if (pos >= data.length) { overread = true; throw new IOException("parity: read past the bytes the C client had"); }
            return data[pos++] & 255;
        }
        public int read(byte[] b, int off, int len) throws IOException {
            if (len == 0) return 0;
            if (pos >= data.length) { overread = true; throw new IOException("parity: read past the bytes the C client had"); }
            int n = Math.min(len, data.length - pos);
            System.arraycopy(data, pos, b, off, n); pos += n; return n;
        }
    }
    static final ByteArrayOutputStream captured = new ByteArrayOutputStream();
    static final OutputStream captureOut = new OutputStream() {
        public void write(int b) { synchronized (captured) { captured.write(b); } }
        public void write(byte[] b, int off, int len) { synchronized (captured) { captured.write(b, off, len); } }
    };
    static byte[] takeCaptured() { synchronized (captured) { byte[] b = captured.toByteArray(); captured.reset(); return b; } }

    static void drainWriter(client app) throws Exception {
        ClientStream stream = (ClientStream) get(app, "stream");
        if (stream == null) return;
        long deadline = System.currentTimeMillis() + 3000;
        while (System.currentTimeMillis() < deadline) {
            synchronized (stream) { if (stream.bufPos == stream.bufLen) break; }
            Thread.sleep(1);
        }
        Thread.sleep(2); // the writer updates bufLen after out.write returns
    }

    // ---------------------------------------------------------------- input
    static void mouse(client app, int id, int x, int y, boolean right) {
        int mods = right ? InputEvent.META_MASK | InputEvent.BUTTON3_MASK : InputEvent.BUTTON1_MASK;
        if (id == MouseEvent.MOUSE_MOVED) mods = 0;
        MouseEvent e = new MouseEvent(app, id, 0L, mods, x, y, 1, false, right ? MouseEvent.BUTTON3 : MouseEvent.BUTTON1);
        if (id == MouseEvent.MOUSE_PRESSED) app.mousePressed(e);
        else if (id == MouseEvent.MOUSE_RELEASED) app.mouseReleased(e);
        else app.mouseMoved(e);
    }
    static void key(client app, boolean press, int code, char ch) {
        KeyEvent e = new KeyEvent(app, press ? KeyEvent.KEY_PRESSED : KeyEvent.KEY_RELEASED, 0L, 0, code, ch);
        if (press) app.keyPressed(e); else app.keyReleased(e);
    }
    // ClientKey order: NONE ESCAPE F1..F12 BACKSPACE DELETE TAB RETURN KP_ENTER
    static void clientKey(client app, int k) {
        if (k == 1) key(app, true, KeyEvent.VK_ESCAPE, (char) 27);
        else if (k >= 2 && k <= 13) key(app, true, KeyEvent.VK_F1 + (k - 2), KeyEvent.CHAR_UNDEFINED);
        else if (k == 14) key(app, true, KeyEvent.VK_BACK_SPACE, '\b');
        else if (k == 15) key(app, true, KeyEvent.VK_DELETE, (char) 127);
        else if (k == 16) key(app, true, KeyEvent.VK_TAB, '\t');
        else if (k == 17 || k == 18) key(app, true, KeyEvent.VK_ENTER, '\n');
    }
    static void text(client app, String s) { for (char c : s.toCharArray()) key(app, true, KeyEvent.VK_UNDEFINED, c); }
    static final String[] SCRIPT_KEYS = { "", "Escape", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "Backspace", "Delete", "Tab", "Return", "Keypad Enter" };

    static int scriptNext;
    static boolean applyScript(client app, int frame) {
        boolean quit = false;
        while (scriptNext < script.size() && script.get(scriptNext).frame < frame) scriptNext++;
        while (scriptNext < script.size() && script.get(scriptNext).frame == frame) {
            Scripted s = script.get(scriptNext++);
            switch (s.kind) {
                case "quit": quit = true; break;
                case "key": clientKey(app, Arrays.asList(SCRIPT_KEYS).indexOf(s.value)); break;
                case "text": text(app, s.value); break;
                default: {
                    String[] xy = s.value.trim().split("\\s+");
                    int x = Integer.parseInt(xy[0]), y = Integer.parseInt(xy[1]);
                    if (s.kind.equals("move")) mouse(app, MouseEvent.MOUSE_MOVED, x, y, false);
                    else if (s.kind.equals("release")) mouse(app, MouseEvent.MOUSE_RELEASED, x, y, false);
                    else mouse(app, MouseEvent.MOUSE_PRESSED, x, y, s.kind.equals("right"));
                }
            }
        }
        return quit;
    }

    static int heldBits;
    static void applyHeld(client app, int bits, PrintWriter report, int frame) {
        int[] codes = { KeyEvent.VK_LEFT, KeyEvent.VK_RIGHT, KeyEvent.VK_UP, KeyEvent.VK_DOWN };
        for (int i = 0; i < 4; i++) if (((bits ^ heldBits) >> i & 1) != 0) key(app, (bits >> i & 1) != 0, codes[i], KeyEvent.CHAR_UNDEFINED);
        if (((bits ^ heldBits) >> 8 & 1) != 0) key(app, (bits >> 8 & 1) != 0, KeyEvent.VK_CONTROL, KeyEvent.CHAR_UNDEFINED);
        if ((bits & 0xF0) != 0 && (heldBits & 0xF0) == 0) report.println("NOTE frame " + frame + ": W/A/S/D held; the original client has no such binding");
        heldBits = bits;
    }

    static List<Map.Entry<PixMap, int[]>> snapshot() {
        synchronized (drawn) { return new ArrayList<>(drawn.entrySet()); }
    }

    // ---------------------------------------------------------------- outgoing decoder
    // For reports only: opcodes of a chunk of the outgoing stream, using the session's
    // ISAAC seeds and the engine's client packet sizes (ClientGameProt.ts).
    static int[] outSeeds;
    static final Map<Integer, Integer> clientSizes = new HashMap<>();
    static final ByteArrayOutputStream matchedOut = new ByteArrayOutputStream();

    static void loadClientSizes(Path engineSrc) throws IOException {
        Path prot = engineSrc.resolve("network/game/client/ClientGameProt.ts");
        if (!Files.exists(prot)) return;
        java.util.regex.Matcher m = java.util.regex.Pattern.compile("new ClientGameProt\\((\\d+),\\s*(-?\\d+)\\)").matcher(new String(Files.readAllBytes(prot), "UTF-8"));
        while (m.find()) clientSizes.put(Integer.parseInt(m.group(1)), Integer.parseInt(m.group(2)));
    }

    static String decode(byte[] prefix, byte[] chunk) {
        if (outSeeds == null || clientSizes.isEmpty()) return "?";
        jagex2.io.Isaac isaac = new jagex2.io.Isaac((byte) 1, outSeeds.clone());
        StringBuilder s = new StringBuilder();
        byte[][] parts = { prefix, chunk };
        for (int part = 0; part < 2; part++) {
            byte[] b = parts[part]; int pos = 0;
            while (pos < b.length) {
                int op = (b[pos++] - isaac.takeNextValue()) & 255;
                Integer size = clientSizes.get(op);
                if (size == null) { if (part == 1) s.append(op).append("(unknown size) "); break; }
                int len = size;
                if (size == -1 && pos < b.length) len = b[pos++] & 255;
                else if (size == -2 && pos + 1 < b.length) { len = ((b[pos] & 255) << 8) | (b[pos + 1] & 255); pos += 2; }
                if (part == 1) s.append(op).append('(').append(len).append(") ");
                pos += Math.max(len, 0);
            }
        }
        return s.toString().trim();
    }

    // ---------------------------------------------------------------- signlink
    // Stands in for the signlink thread (never started: it would write a cache under
    // c:/windows): cacheload serves the map files the C client reads from <pack>/maps,
    // as a warm original cache would; saves are discarded; URL/socket/thread requests
    // only arise with signlink.mainapp set, which stays null, and fail if they do.
    static void startSignlink(Path pack) throws IOException {
        final Map<String, byte[]> cache = new HashMap<>();
        try (DirectoryStream<Path> maps = Files.newDirectoryStream(pack.resolve("maps"))) {
            for (Path p : maps) cache.put(String.valueOf(signlink.gethash(p.getFileName().toString())), Files.readAllBytes(p));
        }
        signlink.active = true;
        Thread t = new Thread(() -> {
            while (true) {
                if (signlink.loadreq != null) { signlink.loadbuf = cache.get(signlink.loadreq); signlink.loadreq = null; }
                else if (signlink.savereq != null) { signlink.waveplay = false; signlink.midiplay = false; signlink.savereq = null; }
                else if (signlink.dnsreq != null) { signlink.dns = "unknown"; signlink.dnsreq = null; }
                else if (signlink.urlreq != null) { signlink.urlstream = null; signlink.urlreq = null; }
                else if (signlink.socketreq != 0) { signlink.socket = null; signlink.socketreq = 0; }
                else if (signlink.threadreq != null) { signlink.threadreq = null; }
                else Thread.yield();
            }
        });
        t.setDaemon(true); t.start();
    }

    // ---------------------------------------------------------------- main
    public static void main(String[] argv) {
        try { System.exit(run(argv)); } catch (Throwable t) { t.printStackTrace(); System.exit(2); }
    }

    static int run(String[] argv) throws Exception {
        Path log = Paths.get(argv[0]);
        final Path pack = Paths.get(argv[1]);
        Path out = Paths.get(argv[2]);
        long randomState = Long.parseUnsignedLong(argv[3]);
        Set<Integer> dumpFrames = new HashSet<>();
        for (int i = 4; i + 1 < argv.length; i += 2) if (argv[i].equals("--dump-frame")) dumpFrames.add(Integer.parseInt(argv[i + 1]));
        Files.createDirectories(out);
        String password = System.getenv("RS2_PASSWORD");
        parse(log);
        requireSupportedTrace();
        loadScript();
        loadClientSizes(pack.toAbsolutePath().getParent().getParent().getParent().resolve("src"));
        if (args.contains("--offline")) throw new IllegalStateException("offline sessions have no original-client equivalent");

        // Archives, as engine/src/web.ts serves them.
        final String[] names = { "title", "config", "interface", "media", "models", "textures", "wordenc", "sounds" };
        final ByteArrayOutputStream crcTable = new ByteArrayOutputStream();
        DataOutputStream crcOut = new DataOutputStream(crcTable);
        crcOut.writeInt(0);
        for (String n : names) { CRC32 crc = new CRC32(); crc.update(Files.readAllBytes(pack.resolve(n))); crcOut.writeInt((int) crc.getValue()); }
        HttpServer http = HttpServer.create(new InetSocketAddress(InetAddress.getByName("127.0.0.1"), 0), 0);
        http.createContext("/", exchange -> {
            String path = exchange.getRequestURI().getPath();
            byte[] body = null;
            if (path.startsWith("/crc")) body = crcTable.toByteArray();
            else for (String n : names) if (path.startsWith("/" + n)) body = Files.readAllBytes(pack.resolve(n));
            if (body == null) { exchange.sendResponseHeaders(404, -1); exchange.close(); return; }
            exchange.sendResponseHeaders(200, body.length);
            exchange.getResponseBody().write(body); exchange.close();
        });
        http.start();
        final URL codeBase = new URL("http://127.0.0.1:" + http.getAddress().getPort() + "/");

        // The game socket: serves the recorded login bytes, then the stream is swapped to memory.
        final ServerSocket game = new ServerSocket(0, 1, InetAddress.getByName("127.0.0.1"));
        if (game.getLocalPort() < 43594) throw new IllegalStateException("ephemeral port below 43594");

        // Control the original startup before World class initialization. Login
        // synchronization alone cannot reproduce pre-login palettes/colors.
        if (!hasStartupSeed) throw new IllegalStateException("recording has no startup seed");
        Field mathHolder = field(Class.forName("java.lang.Math$RandomNumberGeneratorHolder"), "randomNumberGenerator");
        Random startupRandom = (Random) mathHolder.get(null);
        startupRandom.setSeed(startupSeed);
        client.nodeId = 10; client.portOffset = game.getLocalPort() - 43594; client.members = true;
        client.setHighMemory(9);       // the C client logs in with low memory 0
        signlink.uid = 1;              // protocol.c writes uid 1
        final client app = new client();
        app.setStub(new AppletStub() {
            public boolean isActive() { return true; }
            public URL getDocumentBase() { return codeBase; }
            public URL getCodeBase() { return codeBase; }
            public String getParameter(String name) { return null; }
            public AppletContext getAppletContext() { return null; }
            public void appletResize(int w, int h) { }
        });
        BufferedImage sink = new BufferedImage(W, H, BufferedImage.TYPE_INT_RGB);
        setInt(app, "screenWidth", W); setInt(app, "screenHeight", H);
        set(app, "graphics", new CaptureGraphics(sink.getGraphics()));
        set(app, "drawArea", new PixMap(app, W, 299, H));
        set(app, "flameActive", true);   // never start the flame thread: it draws Math.random on its own clock
        startSignlink(pack);
        app.load();
        long loadedRandom = ((java.util.concurrent.atomic.AtomicLong)field(Random.class, "seed").get(startupRandom)).get();
        System.out.println("[BOOT225] stage=loaded random_state=" + loadedRandom
            + " hue=" + jagex2.dash3d.World.randomHueOffset + " light=" + jagex2.dash3d.World.randomLightnessOffset);
        int[] floorColors = new int[jagex2.config.FloType.count];
        for (int i = 0; i < floorColors.length; i++) floorColors[i] = jagex2.config.FloType.instances[i].hsl;
        System.out.printf("[BOOT225] palette=%08x floors=%08x%n", Arrays.hashCode(jagex2.graphics.Pix3D.colourTable), Arrays.hashCode(floorColors));
        if ((Boolean) get(app, "errorLoading") || (Boolean) get(app, "errorHost") || (Boolean) get(app, "errorStarted"))
            throw new IllegalStateException("original client failed to load");
        System.out.println("[ORACLE] original client loaded; " + frames.size() + " recorded frames");

        PrintWriter report = new PrintWriter(Files.newBufferedWriter(out.resolve("java-report.txt")));
        PrintWriter hashes = new PrintWriter(Files.newBufferedWriter(out.resolve("java-regions.txt")));
        MemoryIn in = null;
        boolean wasIngame = false, failed = false, consumptionNoted = false;
        long cConsumed = 0;
        int compared = 0;
        String stop = "end of the recording";
        for (Frame f : frames) {
            boolean quit = applyScript(app, f.number);
            for (int i = 0; i < f.events.size(); i++) {
                long[] e = f.events.get(i);
                int type = (int) e[0], x = (int) e[1], y = (int) e[2], button = (int) e[3], k = (int) e[4];
                if (type == 1) quit = true;
                else if (type == 2) mouse(app, MouseEvent.MOUSE_MOVED, x, y, false);
                else if (type == 3) mouse(app, MouseEvent.MOUSE_PRESSED, x, y, button == 3);
                else if (type == 4) mouse(app, MouseEvent.MOUSE_RELEASED, x, y, button == 3);
                else if (type == 5) report.println("NOTE frame " + f.number + ": mouse wheel; the original client has no wheel input");
                else if (type == 6) clientKey(app, k);
                else if (type == 7) text(app, f.texts.get(i));
                else if (type == 8) app.idleCycles = 0; // GameShell.keyReleased; held-key releases are applied separately
            }
            if (f.held != null) applyHeld(app, f.held, report, f.number);
            if (in != null && f.lateIn.size() > 0) report.println("NOTE frame " + f.number + ": C read " + f.lateIn.size() + " bytes outside update cycles");

            for (Cycle c : f.cycles) {
                if (in != null) { in.release(c.in); cConsumed += c.in.size(); }
                app.update(437);
                setInt(app, "mouseClickButton", 0);
                setInt(app, "keyQueueReadPos", getInt(app, "keyQueueWritePos"));
            }
            if (in != null && f.lateIn.size() > 0) { in.release(f.lateIn); cConsumed += f.lateIn.size(); }
            app.draw(false);
            drainWriter(app);
            boolean ingame = (Boolean) get(app, "ingame");

            for (Map.Entry<PixMap, int[]> d : snapshot()) {
                PixMap m = d.getKey(); int x = Math.max(0, d.getValue()[0]), y = Math.max(0, d.getValue()[1]);
                int w = Math.min(m.width, W - x), h = Math.min(m.height, H - y);
                hashes.println(f.number + " " + regionName(app, m) + " " + String.format("%016x", regionHash(x, y, w, h)));
            }
            hashes.println(f.number + " screen " + String.format("%016x", regionHash(0, 0, W, H)));
            if (dumpFrames.contains(f.number)) {
                dump(out.resolve("java-frame-" + String.format("%06d", f.number) + ".png"));
                Object mapImage=get(app,"imageMinimap");
                BufferedImage mapDump=new BufferedImage(512,512,BufferedImage.TYPE_INT_RGB);
                mapDump.setRGB(0,0,512,512,(int[])get(mapImage,"pixels"),0,512);
                ImageIO.write(mapDump,"png",out.resolve("java-minimap-"+String.format("%06d",f.number)+".png").toFile());
            }
            if(dumpFrames.contains(f.number) || System.getenv("RS2_TRACE_ACTORS")!=null) {
                Object tracedLocal=get(app,"localPlayer");
                if(tracedLocal!=null)System.out.println("[LOCAL225 STATE] frame="+f.number+" pos="+getInt(tracedLocal,"x")+","+getInt(tracedLocal,"z")
                    +" yaw="+getInt(tracedLocal,"yaw")+","+getInt(tracedLocal,"dstYaw")
                    +" seq="+getInt(tracedLocal,"secondarySeqId")+","+getInt(tracedLocal,"secondarySeqFrame")+","+getInt(tracedLocal,"secondarySeqCycle")
                    +" primary="+getInt(tracedLocal,"primarySeqId")+","+getInt(tracedLocal,"primarySeqFrame")+","+getInt(tracedLocal,"primarySeqCycle")
                    +" camera="+getInt(app,"cameraX")+","+getInt(app,"cameraY")+","+getInt(app,"cameraZ")+","+getInt(app,"cameraPitch")+","+getInt(app,"cameraYaw"));
                Object[] mapNpcs=(Object[])get(app,"npcs");
                int[] mapNpcIds=(int[])get(app,"npcIds");
                for(int ni=0;ni<getInt(app,"npcCount");ni++) {
                    int slot=mapNpcIds[ni];Object n=mapNpcs[slot];if(n==null)continue;
                    System.out.println("[NPC225 STATE] frame="+f.number+" slot="+slot+" pos="+getInt(n,"x")+","+getInt(n,"z")
                        +" yaw="+getInt(n,"yaw")+","+getInt(n,"dstYaw")+" seq="+getInt(n,"secondarySeqId")+","+getInt(n,"secondarySeqFrame")+","+getInt(n,"secondarySeqCycle")
                        +" primary="+getInt(n,"primarySeqId")+","+getInt(n,"primarySeqFrame")+","+getInt(n,"primarySeqCycle"));
                }
            }
            if(dumpFrames.contains(f.number)) {
                String[] menuLabels=(String[])get(app,"menuOption");
                int[] menuActions=(int[])get(app,"menuAction"),menuTargets=(int[])get(app,"menuParamA");
                for(int mi=0;mi<getInt(app,"menuSize");mi++)
                    System.out.println("[MENU225] frame="+f.number+" row="+mi+" action="+menuActions[mi]+" target="+menuTargets[mi]+" label="+menuLabels[mi]);
                Object local = get(app, "localPlayer");
                System.out.println("[ORACLE STATE] frame=" + f.number + " base=" + getInt(app, "sceneBaseTileX") + "," + getInt(app, "sceneBaseTileZ")
                    + " level=" + getInt(app, "currentLevel") + " local=" + getInt(local, "x") + "," + getInt(local, "z")
                    + " path=" + ((int[])get(local, "pathTileX"))[0] + "," + ((int[])get(local, "pathTileZ"))[0]
                    + " camera=" + getInt(app, "cameraX") + "," + getInt(app, "cameraY") + "," + getInt(app, "cameraZ")
                    + " pitch=" + getInt(app, "cameraPitch") + " yaw=" + getInt(app, "cameraYaw")
                    + " orbit=" + getInt(app, "orbitCameraYaw") + " map=" + getInt(app, "minimapAnticheatAngle") + "," + getInt(app, "minimapZoom")
                    + " menu=" + get(app, "menuVisible") + " hue=" + jagex2.dash3d.World.randomHueOffset + " light=" + jagex2.dash3d.World.randomLightnessOffset);
            }

            if (in != null) {
                byte[] sent = takeCaptured();
                byte[] expected = f.out.toByteArray();
                if (!Arrays.equals(sent, expected) && !failed) {
                    byte[] prefix = matchedOut.toByteArray();
                    report.println("DIVERGE frame " + f.number + " outgoing bytes: java " + hex(sent, sent.length) + " c " + hex(expected, expected.length));
                    report.println("        opcodes: java [" + decode(prefix, sent) + "] c [" + decode(prefix, expected) + "]");
                    failed = true;
                }
                if (!failed) matchedOut.write(sent, 0, sent.length);
                // C reads partial payloads as they arrive; Java waits for the whole payload
                // (client.read checks available()), so counts may differ mid-packet.
                if (in.pos != cConsumed && !consumptionNoted) {
                    report.println("NOTE frame " + f.number + " inbound consumed: java " + in.pos + " c " + cConsumed + " (first occurrence)");
                    consumptionNoted = true;
                }
                if (in.overread && !failed) { report.println("DIVERGE frame " + f.number + " java read past the recorded bytes"); failed = true; }
                compared++;
            }
            if (wasIngame && !ingame) { stop = "the client left the game at frame " + f.number; break; }
            wasIngame = ingame;
            if (quit) { stop = "quit at frame " + f.number; break; }

            if (f.login) {
                // Same Math.random state as the C client: setSeed scrambles with the multiplier.
                Field holder = field(Class.forName("java.lang.Math$RandomNumberGeneratorHolder"), "randomNumberGenerator");
                ((Random) holder.get(null)).setSeed(randomState ^ 0x5DEECE66DL);
                final byte[] serverBytes = f.loginIn.toByteArray();
                Random copy = new Random(randomState ^ 0x5DEECE66DL);
                long serverSeed = 0;
                for (int i = 0; i < 8 && i < serverBytes.length; i++) serverSeed = (serverSeed << 8) | (serverBytes[i] & 255);
                outSeeds = new int[] { (int) (copy.nextDouble() * 9.9999999E7D), (int) (copy.nextDouble() * 9.9999999E7D), (int) (serverSeed >> 32), (int) serverSeed };
                final ByteArrayOutputStream loginSent = new ByteArrayOutputStream();
                Thread server = new Thread(() -> {
                    try {
                        Socket s = game.accept();
                        s.getOutputStream().write(serverBytes); s.getOutputStream().flush();
                        InputStream r = s.getInputStream(); byte[] b = new byte[4096]; int n;
                        while ((n = r.read(b)) > 0) synchronized (loginSent) { loginSent.write(b, 0, n); }
                    } catch (IOException e) { }
                });
                server.setDaemon(true); server.start();
                // Normal title input owns these fields; direct oracle login
                // must populate them too (chat prompt/reconnect read them).
                set(app,"username",username);set(app,"password",password);
                app.login(username, password, false);
                drainWriter(app);
                Thread.sleep(100);
                byte[] sent; synchronized (loginSent) { sent = loginSent.toByteArray(); }
                byte[] expected = f.loginOut.toByteArray();
                if (!Arrays.equals(sent, expected)) {
                    report.println("DIVERGE login packet: java " + hex(sent, sent.length) + " c " + hex(expected, expected.length));
                    failed = true;
                } else report.println("OK login packet identical (" + sent.length + " bytes)");
                if (!(Boolean) get(app, "ingame")) { stop = "login failed: " + get(app, "loginMessage0") + " " + get(app, "loginMessage1"); break; }
                ClientStream stream = (ClientStream) get(app, "stream");
                in = new MemoryIn();
                synchronized (stream) { stream.in = in; stream.out = captureOut; }
                takeCaptured();
                wasIngame = true;
            }
        }
        report.println("STOP " + stop + "; " + compared + " in-game frames compared");
        report.println(failed ? "RESULT DIVERGED" : "RESULT MATCH");
        report.close(); hashes.close();

        PrintWriter rect = new PrintWriter(Files.newBufferedWriter(out.resolve("regions.txt")));
        for (Map.Entry<PixMap, int[]> d : snapshot()) {
            PixMap m = d.getKey(); int x = Math.max(0, d.getValue()[0]), y = Math.max(0, d.getValue()[1]);
            rect.println(regionName(app, m) + " " + x + " " + y + " " + Math.min(m.width, W - x) + " " + Math.min(m.height, H - y));
        }
        rect.println("screen 0 0 " + W + " " + H);
        rect.close();
        System.out.println("[ORACLE] " + stop + (failed ? "; DIVERGED (see java-report.txt)" : "; outgoing bytes and inbound consumption match"));
        http.stop(0);
        return failed ? 1 : 0;
    }

    static final Map<PixMap, String> names = new IdentityHashMap<>();
    static String regionName(client app, PixMap m) throws Exception {
        String n = names.get(m);
        if (n != null) return n;
        for (Class<?> c = client.class; c != null; c = c.getSuperclass())
            for (Field f : c.getDeclaredFields())
                if (f.getType() == PixMap.class && !java.lang.reflect.Modifier.isStatic(f.getModifiers())) {
                    f.setAccessible(true);
                    if (f.get(app) == m) n = f.getName();
                }
        if (n == null) n = "pixmap" + names.size();
        // Distinct PixMaps can share a field over time (title then game); keep names unique.
        while (names.containsValue(n)) n = n + "_";
        names.put(m, n);
        return n;
    }

    static void dump(Path file) throws IOException {
        BufferedImage image = new BufferedImage(W, H, BufferedImage.TYPE_INT_RGB);
        image.setRGB(0, 0, W, H, screen, 0, W);
        ImageIO.write(image, "png", file.toFile());
    }
}
