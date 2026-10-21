package sign;

import java.applet.Applet;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.InetAddress;
import java.net.Socket;
import java.net.URL;

public class signlink implements Runnable {

	public static Socket socket = null;

	public static int threadreqpri = 1;

	public static Runnable threadreq = null;

	public static String dnsreq = null;

	public static String dns = null;

	public static String loadreq = null;

	public static byte[] loadbuf = null;

	public static String savereq = null;

	public static byte[] savebuf = null;

	public static String urlreq = null;

	public static DataInputStream urlstream = null;

	public static int looprate = 50;

	public static String midi = null;

	public static String wave = null;

	public static boolean reporterror = true;

	public static String errorname = "";

	public static final int clientversion = 225;

	public static int midifade;

	public static int midipos;

	public static int midivol;

	public static int savelen;

	public static int socketreq;

	public static int threadliveid;

	public static int uid;

	public static int wavepos;

	public static int wavevol;

	public static Applet mainapp;

	public static InetAddress socketip;

	public static boolean active;

	public static boolean midiplay;

	public static boolean sunjava;

	public static boolean waveplay;

	public static final void startpriv(InetAddress arg0) {
		threadliveid = (int) (Math.random() * 9.9999999E7D);
		if (active) {
			try {
				Thread.sleep(500L);
			} catch (Exception var3) {
			}
			active = false;
		}
		socketreq = 0;
		threadreq = null;
		dnsreq = null;
		loadreq = null;
		savereq = null;
		urlreq = null;
		socketip = arg0;
		Thread var1 = new Thread(new signlink());
		var1.setDaemon(true);
		var1.start();
		while (!active) {
			try {
				Thread.sleep(50L);
			} catch (Exception var2) {
			}
		}
	}

	public final void run() {
		active = true;
		String var1 = findcachedir();
		uid = getuid(var1);
		int var2 = threadliveid;
		while (threadliveid == var2) {
			if (socketreq != 0) {
				try {
					socket = new Socket(socketip, socketreq);
				} catch (Exception var6) {
					socket = null;
				}
				socketreq = 0;
			} else if (threadreq != null) {
				Thread var3 = new Thread(threadreq);
				var3.setDaemon(true);
				var3.start();
				var3.setPriority(threadreqpri);
				threadreq = null;
			} else if (dnsreq != null) {
				try {
					dns = InetAddress.getByName(dnsreq).getHostName();
				} catch (Exception var11) {
					dns = "unknown";
				}
				dnsreq = null;
			} else if (loadreq != null) {
				loadbuf = null;
				try {
					File var12 = new File(var1 + loadreq);
					if (var12.exists()) {
						int var4 = (int) var12.length();
						loadbuf = new byte[var4];
						DataInputStream var5 = new DataInputStream(new FileInputStream(var1 + loadreq));
						var5.readFully(loadbuf, 0, var4);
						var5.close();
					}
				} catch (Exception var10) {
				}
				loadreq = null;
			} else if (savereq != null) {
				if (savebuf != null) {
					try {
						FileOutputStream var13 = new FileOutputStream(var1 + savereq);
						var13.write(savebuf, 0, savelen);
						var13.close();
					} catch (Exception var9) {
					}
				}
				if (waveplay) {
					wave = var1 + savereq;
					waveplay = false;
				}
				if (midiplay) {
					midi = var1 + savereq;
					midiplay = false;
				}
				savereq = null;
			} else if (urlreq != null) {
				try {
					urlstream = new DataInputStream((new URL(mainapp.getCodeBase(), urlreq)).openStream());
				} catch (Exception var8) {
					urlstream = null;
				}
				urlreq = null;
			}
			try {
				Thread.sleep((long) looprate);
			} catch (Exception var7) {
			}
		}
	}

    // Verification-only cache isolation. Rendering/protocol client code is unchanged.
    public static final String findcachedir() {
        try {
            String workspace=System.getProperty("rs2.reference.workspace");
            String location=System.getProperty("rs2.reference.cache");
            if(workspace==null || location==null) throw new IllegalStateException("Explicit workspace/cache required");
            File root=new File(workspace).getCanonicalFile(), cache=new File(location).getCanonicalFile();
            if(!cache.getPath().startsWith(root.getPath()+File.separator)) throw new IllegalStateException("Cache outside workspace");
            if(!cache.isDirectory()) throw new IllegalStateException("Cache must already exist inside workspace");
            return cache.getPath()+File.separator;
        } catch(IOException error) { throw new IllegalStateException(error); }
    }

	public static final int getuid(String arg0) {
		try {
			File var1 = new File(arg0 + "uid.dat");
			if (!var1.exists() || var1.length() < 4L) {
				DataOutputStream var2 = new DataOutputStream(new FileOutputStream(arg0 + "uid.dat"));
				var2.writeInt((int) (Math.random() * 9.9999999E7D));
				var2.close();
			}
		} catch (Exception var4) {
		}
		try {
			DataInputStream var5 = new DataInputStream(new FileInputStream(arg0 + "uid.dat"));
			int var6 = var5.readInt();
			var5.close();
			return var6 + 1;
		} catch (Exception var3) {
			return 0;
		}
	}

	public static final long gethash(String arg0) {
		String var5 = arg0.trim();
		long var1 = 0L;
		for (int var3 = 0; var3 < var5.length() && var3 < 12; var3++) {
			char var4 = var5.charAt(var3);
			var1 *= 37L;
			if (var4 >= 'A' && var4 <= 'Z') {
				var1 += var4 + 1 - 65;
			} else if (var4 >= 'a' && var4 <= 'z') {
				var1 += var4 + 1 - 97;
			} else if (var4 >= '0' && var4 <= '9') {
				var1 += var4 + 27 - 48;
			}
		}
		return var1;
	}

	public static final void looprate(int arg0) {
		looprate = arg0;
	}

	public static final synchronized byte[] cacheload(String arg0) {
		if (!active) {
			return null;
		}
		loadreq = String.valueOf(gethash(arg0));
		while (loadreq != null) {
			try {
				Thread.sleep(1L);
			} catch (Exception var1) {
			}
		}
		return loadbuf;
	}

	public static final synchronized void cachesave(String arg0, byte[] arg1) {
		if (!active || arg1.length > 2000000) {
			return;
		}
		while (savereq != null) {
			try {
				Thread.sleep(1L);
			} catch (Exception var3) {
			}
		}
		savelen = arg1.length;
		savebuf = arg1;
		savereq = String.valueOf(gethash(arg0));
		while (savereq != null) {
			try {
				Thread.sleep(1L);
			} catch (Exception var2) {
			}
		}
	}

	public static final synchronized Socket opensocket(int arg0) throws IOException {
		socketreq = arg0;
		while (socketreq != 0) {
			try {
				Thread.sleep(50L);
			} catch (Exception var1) {
			}
		}
		if (socket == null) {
			throw new IOException("could not open socket");
		}
		return socket;
	}

	public static final synchronized DataInputStream openurl(String arg0) throws IOException {
		urlreq = arg0;
		while (urlreq != null) {
			try {
				Thread.sleep(50L);
			} catch (Exception var1) {
			}
		}
		if (urlstream == null) {
			throw new IOException("could not open: " + arg0);
		}
		return urlstream;
	}

	public static final synchronized void dnslookup(String arg0) {
		dns = arg0;
		dnsreq = arg0;
	}

	public static final synchronized void startthread(Runnable arg0, int arg1) {
		threadreqpri = arg1;
		threadreq = arg0;
	}

	public static final synchronized boolean wavesave(byte[] arg0, int arg1) {
		if (arg1 > 2000000) {
			return false;
		} else if (savereq == null) {
			wavepos = (wavepos + 1) % 5;
			savelen = arg1;
			savebuf = arg0;
			waveplay = true;
			savereq = "sound" + wavepos + ".wav";
			return true;
		} else {
			return false;
		}
	}

	public static final synchronized boolean wavereplay() {
		if (savereq == null) {
			savebuf = null;
			waveplay = true;
			savereq = "sound" + wavepos + ".wav";
			return true;
		} else {
			return false;
		}
	}

	public static final synchronized void midisave(byte[] arg0, int arg1) {
		if (arg1 > 2000000 || savereq != null) {
			return;
		}
		midipos = (midipos + 1) % 5;
		savelen = arg1;
		savebuf = arg0;
		midiplay = true;
		savereq = "jingle" + midipos + ".mid";
	}

	public static final void reporterror(String arg0) {
		if (!reporterror || !active) {
			return;
		}
		System.out.println("Error: " + arg0);
		try {
			String var3 = arg0.replace('@', '_');
			String var4 = var3.replace('&', '_');
			String var5 = var4.replace('#', '_');
			DataInputStream var1 = openurl("reporterror" + 225 + ".cgi?error=" + errorname + " " + var5);
			var1.readLine();
			var1.close();
		} catch (IOException var2) {
		}
	}
}
