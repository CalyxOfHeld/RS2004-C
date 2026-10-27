package jagex2.client;
public class TrackingClock {
 public static long time;public static int calls;public static boolean prefixOnly=true;
 public static long now(){calls++;return time;}
 public static class PrefixDone extends RuntimeException {private static final long serialVersionUID=1L;}
}
