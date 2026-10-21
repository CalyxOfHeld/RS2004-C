import jagex2.config.*;
import jagex2.graphics.*;
import jagex2.io.Jagfile;
import java.io.*;
import java.lang.reflect.*;
import java.nio.file.*;
import java.security.MessageDigest;
import java.util.*;

// Dumps every object the unchanged revision-225 decoders produce from the
// bundled cache, one "type id field=value" line per instance field, so the
// native decoders can be compared field by field. Run with the original jar
// first on the classpath:
//   java -cp "javaclient/build/libs/rs2client.jar;<classes>" DataOracle engine/data/pack/client out.txt
public class DataOracle {
    static PrintWriter out;
    static PixFont[] fonts;
    static IdentityHashMap<Object, Integer> bases = new IdentityHashMap<>();

    static Jagfile jag(String dir, String name) throws IOException {
        return new Jagfile(Files.readAllBytes(Paths.get(dir, name)), false);
    }

    static String sha(int[] p) throws Exception {
        MessageDigest d = MessageDigest.getInstance("SHA-256");
        for (int v : p) { d.update((byte) (v >> 24)); d.update((byte) (v >> 16)); d.update((byte) (v >> 8)); d.update((byte) v); }
        StringBuilder s = new StringBuilder();
        for (byte b : d.digest()) s.append(String.format("%02x", b));
        return s.substring(0, 16);
    }

    static String sha(byte[] p) throws Exception {
        MessageDigest d = MessageDigest.getInstance("SHA-256");
        d.update(p);
        StringBuilder s = new StringBuilder();
        for (byte b : d.digest()) s.append(String.format("%02x", b));
        return s.substring(0, 16);
    }

    static String str(String s) {
        StringBuilder b = new StringBuilder("\"");
        for (char c : s.toCharArray()) {
            if (c == '"' || c == '\\') b.append('\\').append(c);
            else if (c < 32 || c > 126) b.append(String.format("\\u%04x", (int) c));
            else b.append(c);
        }
        return b.append('"').toString();
    }

    static String value(Object v) throws Exception {
        if (v == null) return "null";
        if (v instanceof String) return str((String) v);
        if (v instanceof Boolean) return ((Boolean) v) ? "1" : "0";
        if (v instanceof Number || v instanceof Character) return String.valueOf(v instanceof Character ? (int) (Character) v : v);
        if (v.getClass().isArray()) {
            int n = Array.getLength(v);
            StringBuilder b = new StringBuilder("[");
            for (int i = 0; i < n; i++) { if (i > 0) b.append(','); b.append(value(Array.get(v, i))); }
            return b.append(']').toString();
        }
        if (v instanceof Pix32) {
            Pix32 p = (Pix32) v;
            return "Pix32(" + p.width + "x" + p.height + " crop=" + p.cropX + "," + p.cropY + " full=" + p.cropW + "x" + p.cropH + " px=" + sha(p.pixels) + ")";
        }
        if (v instanceof Pix8) {
            Pix8 p = (Pix8) v;
            return "Pix8(" + p.width + "x" + p.height + " crop=" + p.cropX + "," + p.cropY + " full=" + p.cropW + "x" + p.cropH + " pal=" + sha(p.palette) + " px=" + sha(p.pixels) + ")";
        }
        if (v instanceof PixFont) {
            for (int i = 0; i < fonts.length; i++) if (fonts[i] == v) return "font" + i;
            return "font?";
        }
        if (bases.containsKey(v)) return "base" + bases.get(v);
        return "<" + v.getClass().getSimpleName() + ">";
    }

    static List<Field> fields(Class<?> c) {
        List<Field> fs = new ArrayList<>();
        for (Field f : c.getDeclaredFields()) {
            if (Modifier.isStatic(f.getModifiers()) || f.getName().matches("field[0-9]+")) continue;
            f.setAccessible(true);
            fs.add(f);
        }
        fs.sort(Comparator.comparing(Field::getName));
        return fs;
    }

    static void dump(String type, int id, Object o) throws Exception {
        if (o == null) { out.println(type + " " + id + " null"); return; }
        for (Field f : fields(o.getClass())) out.println(type + " " + id + " " + f.getName() + "=" + value(f.get(o)));
    }

    static Object staticField(Class<?> c, String name) throws Exception {
        Field f = c.getDeclaredField(name);
        f.setAccessible(true);
        return f.get(null);
    }

    static void instances(String type, Class<?> c) throws Exception {
        Object[] all = (Object[]) staticField(c, "instances");
        out.println(type + " count=" + all.length);
        for (int i = 0; i < all.length; i++) dump(type, i, all[i]);
    }

    public static void main(String[] args) throws Exception {
        // FloType.setColor draws from Math.random(); seed its shared generator so
        // the native dump (math_random_seed(225)) is comparable.
        ((Random) staticField(Class.forName("java.lang.Math$RandomNumberGeneratorHolder"), "randomNumberGenerator")).setSeed(225L);
        String dir = args[0];
        out = new PrintWriter(new BufferedWriter(new OutputStreamWriter(new FileOutputStream(args[1]), "US-ASCII")));
        Jagfile config = jag(dir, "config"), models = jag(dir, "models"), media = jag(dir, "media"),
                title = jag(dir, "title"), iface = jag(dir, "interface");

        // Exactly client.load's order (client.java:6438-6459): SeqType resolves
        // delays from AnimFrame and SpotAnimType links SeqType.
        Model.unpack(3, models);
        AnimBase.unpack(false, models);
        AnimBase[] b = (AnimBase[]) staticField(AnimBase.class, "instances");
        for (int i = 0; i < b.length; i++) if (b[i] != null) bases.put(b[i], i);
        AnimFrame.unpack(false, models);
        SeqType.unpack(config, 473);
        LocType.unpack(config);
        FloType.unpack(config, 473);
        ObjType.unpack(config);
        NpcType.unpack(config);
        IdkType.unpack(config, 473);
        SpotAnimType.unpack(config, 473);
        VarpType.unpack(config, 473);

        instances("flo", FloType.class);
        instances("idk", IdkType.class);
        instances("spotanim", SpotAnimType.class);
        instances("varp", VarpType.class);
        out.println("obj count=" + ObjType.count);
        for (int i = 0; i < ObjType.count; i++) dump("obj", i, ObjType.get(i));
        out.println("loc count=" + LocType.count);
        for (int i = 0; i < LocType.count; i++) dump("loc", i, LocType.get(i));
        out.println("npc count=" + NpcType.count);
        for (int i = 0; i < NpcType.count; i++) dump("npc", i, NpcType.get(i));
        instances("animbase", AnimBase.class);
        instances("animframe", AnimFrame.class);
        instances("seq", SeqType.class);
        Object[] meta = (Object[]) staticField(Model.class, "metadata");
        out.println("model count=" + meta.length);
        for (int i = 0; i < meta.length; i++) {
            if (meta[i] == null) { out.println("model " + i + " null"); continue; }
            dump("model", i, new Model(false, i));
        }
        fonts = new PixFont[] { new PixFont(title, "p11", 530), new PixFont(title, "p12", 530),
                                new PixFont(title, "b12", 530), new PixFont(title, "q8", 530) };
        for (int i = 0; i < fonts.length; i++) dump("font", i, fonts[i]);
        Component.unpack(media, fonts, 30, iface);
        instances("component", Component.class);
        out.close();
        System.out.println("data oracle written: java=" + System.getProperty("java.version"));
        System.exit(0);
    }
}
