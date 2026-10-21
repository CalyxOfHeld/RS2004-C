import deob.client;
import jagex2.io.Jagfile;
import jagex2.graphics.PixFont;
import jagex2.graphics.PixMap;
import java.awt.image.BufferedImage;
import java.io.*;
import java.lang.reflect.Field;
import java.nio.file.*;

// Runs the unchanged revision-225 client.loadTitle, drawTitleScreen and
// drawFlames on the real title archive and records the PixMap pixels.
// The flame thread is not started (flameActive is set first) so the dump
// is not raced; drawFlames is fed fixed buffers because Math.random is
// unseeded in the original.
public class TitleOracle {
    static Field field(String name) throws Exception {
        for (Class<?> c = client.class; c != null; c = c.getSuperclass()) {
            try { Field f = c.getDeclaredField(name); f.setAccessible(true); return f; } catch (NoSuchFieldException e) { }
        }
        throw new NoSuchFieldException(name);
    }
    static Object get(client c, String name) throws Exception { return field(name).get(c); }
    static void set(client c, String name, Object v) throws Exception { field(name).set(c, v); }

    static void image(DataOutputStream out, client c, int index) throws Exception {
        PixMap m = (PixMap) get(c, "imageTitle" + index);
        out.writeInt(index); out.writeInt(m.width); out.writeInt(m.height);
        for (int p : m.pixels) out.writeInt(p);
    }

    public static void main(String[] args) throws Exception {
        Jagfile title = new Jagfile(Files.readAllBytes(Paths.get(args[0])), false);
        client c = new client();
        BufferedImage screen = new BufferedImage(789, 532, BufferedImage.TYPE_INT_RGB);
        set(c, "graphics", screen.getGraphics());
        set(c, "screenWidth", 789);   // initApplication(532, 789)
        set(c, "screenHeight", 532);
        set(c, "archiveTitle", title);
        set(c, "fontBold12", new PixFont(title, "b12", 530));
        set(c, "flameActive", true);
        java.lang.reflect.Method load = client.class.getMethod("loadTitle", byte.class);
        load.invoke(c, (byte) 99);

        DataOutputStream out = new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])));
        out.writeInt(0x54544c31);   // "TTL1"
        for (int i = 0; i < 9; i++) if (i != 4) image(out, c, i);

        // drawTitleScreen: state, field, loopCycle, message0, message1, username, password
        Object[][] states = {
            {0, 0, 0, "", "", "", ""},
            {2, 0, 0, "", "Enter your username & password.", "sollog", "abc"},
            {2, 1, 25, "Your account has been disabled.", "Please check your message-centre for details.", "Zz_9 £$", "pw!?"},
            {2, 1, 5, "", "Connecting to server...", "abcdefghijkl", "abcdefghijklmnopqrst"},
            {3, 0, 0, "", "", "", ""},
        };
        out.writeInt(states.length);
        for (Object[] s : states) {
            set(c, "titleScreenState", s[0]);
            set(c, "titleLoginField", s[1]);
            field("loopCycle").setInt(null, (Integer) s[2]);
            set(c, "loginMessage0", s[3]);
            set(c, "loginMessage1", s[4]);
            set(c, "username", s[5]);
            set(c, "password", s[6]);
            client.class.getMethod("drawTitleScreen", int.class).invoke(c, 4);
            out.writeInt((Integer) s[0]); out.writeInt((Integer) s[1]); out.writeInt((Integer) s[2]);
            for (int k = 3; k < 7; k++) {
                byte[] text = ((String) s[k]).getBytes("ISO-8859-1");
                out.writeInt(text.length); out.write(text);
            }
            image(out, c, 4);
        }

        // drawFlames with fixed flame/offset buffers and each gradient branch.
        int[][] cycles = {{0, 0}, {900, 0}, {500, 0}, {100, 0}, {0, 900}, {0, 500}, {0, 100}};
        int[] buffer = (int[]) get(c, "flameBuffer3");
        int[] offsets = (int[]) get(c, "flameLineOffset");
        for (int i = 0; i < buffer.length; i++) buffer[i] = i % 7 == 0 ? 0 : (i * 37 + (i >> 7) * 11) & 255;
        for (int y = 0; y < 256; y++) offsets[y] = (y * 13) % 61 - 30;
        out.writeInt(cycles.length);
        for (int[] cycle : cycles) {
            set(c, "flameGradientCycle0", cycle[0]);
            set(c, "flameGradientCycle1", cycle[1]);
            client.class.getMethod("drawFlames", boolean.class).invoke(c, false);
            out.writeInt(cycle[0]); out.writeInt(cycle[1]);
            image(out, c, 0);
            image(out, c, 1);
        }
        out.close();
        System.out.println("title oracle written: java=" + System.getProperty("java.version"));
        System.exit(0);
    }
}
