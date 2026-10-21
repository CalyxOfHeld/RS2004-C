package deob;

import jagex2.client.GameShell;
import jagex2.client.InputTracking;
import jagex2.config.Component;
import jagex2.config.FloType;
import jagex2.config.IdkType;
import jagex2.config.LocType;
import jagex2.config.NpcType;
import jagex2.config.ObjType;
import jagex2.config.SeqType;
import jagex2.config.SpotAnimType;
import jagex2.config.VarpType;
import jagex2.dash3d.CollisionMap;
import jagex2.dash3d.World;
import jagex2.dash3d.World3D;
import jagex2.dash3d.entity.LocAddEntity;
import jagex2.dash3d.entity.LocEntity;
import jagex2.dash3d.entity.LocMergeEntity;
import jagex2.dash3d.entity.NpcEntity;
import jagex2.dash3d.entity.ObjStackEntity;
import jagex2.dash3d.entity.PathingEntity;
import jagex2.dash3d.entity.PlayerEntity;
import jagex2.dash3d.entity.ProjectileEntity;
import jagex2.dash3d.entity.SpotAnimEntity;
import jagex2.datastruct.JString;
import jagex2.datastruct.LinkList;
import jagex2.graphics.AnimBase;
import jagex2.graphics.AnimFrame;
import jagex2.graphics.Model;
import jagex2.graphics.Pix2D;
import jagex2.graphics.Pix32;
import jagex2.graphics.Pix3D;
import jagex2.graphics.Pix8;
import jagex2.graphics.PixFont;
import jagex2.graphics.PixMap;
import jagex2.io.BZip2;
import jagex2.io.ClientStream;
import jagex2.io.Isaac;
import jagex2.io.Jagfile;
import jagex2.io.Packet;
import jagex2.io.Protocol;
import jagex2.sound.Wave;
import jagex2.wordenc.WordFilter;
import jagex2.wordenc.WordPack;
import java.awt.Color;
import java.awt.Font;
import java.awt.Graphics;
import java.io.DataInputStream;
import java.io.IOException;
import java.math.BigInteger;
import java.net.InetAddress;
import java.net.Socket;
import java.net.URL;
import java.util.zip.CRC32;
import sign.signlink;

@ObfuscatedName("client")
public class client extends GameShell {
    public static volatile Runnable referenceTask; // isolated harness scheduling only

	@ObfuscatedName("client.ab")
	public int hintTileZ;

	@ObfuscatedName("client.bb")
	public int hintHeight;

	@ObfuscatedName("client.cb")
	public int hintOffsetX;

	@ObfuscatedName("client.db")
	public int hintOffsetZ;

	@ObfuscatedName("client.eb")
	public int minimapOffsetCycle;

	@ObfuscatedName("client.fb")
	public boolean redrawBackground = false;

	@ObfuscatedName("client.gb")
	public LinkList locList = new LinkList(0);

	@ObfuscatedName("client.hb")
	public Isaac randomIn;

	@ObfuscatedName("client.ib")
	public boolean[] cameraModifierEnabled = new boolean[5];

	@ObfuscatedName("client.jb")
	public int privateChatSetting;

	@ObfuscatedName("client.kb")
	public int field1121 = 9;

	@ObfuscatedName("client.lb")
	public int selectedTab = 3;

	@ObfuscatedName("client.mb")
	public int[][] bfsCost = new int[104][104];

	@ObfuscatedName("client.nb")
	public int field1124 = 997;

	@ObfuscatedName("client.ob")
	public int socialAction;

	@ObfuscatedName("client.pb")
	public int sceneBaseTileX;

	@ObfuscatedName("client.qb")
	public int sceneBaseTileZ;

	@ObfuscatedName("client.rb")
	public int mapLastBaseX;

	@ObfuscatedName("client.sb")
	public int mapLastBaseZ;

	@ObfuscatedName("client.tb")
	public String socialInput = "";

	@ObfuscatedName("client.ub")
	public byte field1131 = 106;

	@ObfuscatedName("client.vb")
	public LinkList mergedLocations = new LinkList(0);

	@ObfuscatedName("client.wb")
	public long[] ignoreName37 = new long[100];

	@ObfuscatedName("client.xb")
	public int weightCarried;

	@ObfuscatedName("client.yb")
	public byte[][] sceneMapLandData;

	@ObfuscatedName("client.zb")
	public int field1136 = 723;

	@ObfuscatedName("client.ac")
	public Packet out = Packet.alloc(1, -737);

	@ObfuscatedName("client.bc")
	public boolean startMidiThread = false;

	@ObfuscatedName("client.cc")
	public int chatEffects;

	@ObfuscatedName("client.dc")
	public int hintNpc;

	@ObfuscatedName("client.ec")
	public int overrideChat;

	@ObfuscatedName("client.fc")
	public int field1168 = 3;

	@ObfuscatedName("client.gc")
	public static int cyclelogic2;

	@ObfuscatedName("client.hc")
	public int field1170 = -655;

	@ObfuscatedName("client.ic")
	public static int oplogic3;

	@ObfuscatedName("client.jc")
	public int[] skillLevel = new int[50];

	@ObfuscatedName("client.kc")
	public Component chatInterface = new Component();

	@ObfuscatedName("client.lc")
	public int[] waveLoops = new int[50];

	@ObfuscatedName("client.mc")
	public int mouseButtonsOption;

	@ObfuscatedName("client.N")
	public int field1098 = 24676;

	@ObfuscatedName("client.O")
	public int localPid = -1;

	@ObfuscatedName("client.W")
	public int[] designColors = new int[5];

	@ObfuscatedName("client.X")
	public Packet login = Packet.alloc(1, -737);

	@ObfuscatedName("client.Bb")
	public int[] friendWorld = new int[100];

	@ObfuscatedName("client.Cb")
	public int minimapLevel = -1;

	@ObfuscatedName("client.Db")
	public String socialMessage = "";

	@ObfuscatedName("client.Eb")
	public Pix32[] imageHitmarks = new Pix32[20];

	@ObfuscatedName("client.Gb")
	public int field1143 = 332;

	@ObfuscatedName("client.Mb")
	public String chatbackInput = "";

	@ObfuscatedName("client.Ob")
	public int lastWaveId = -1;

	@ObfuscatedName("client.Pb")
	public boolean field1152 = true;

	@ObfuscatedName("client.Qb")
	public boolean updateDesignModel = false;

	@ObfuscatedName("client.Rb")
	public int[] designIdentikits = new int[7];

	@ObfuscatedName("client.Sb")
	public byte field1155 = 8;

	@ObfuscatedName("client.Tb")
	public Pix32[] activeMapFunctions = new Pix32[1000];

	@ObfuscatedName("client.Ub")
	public int chatScrollHeight = 78;

	@ObfuscatedName("client.Xb")
	public Packet in = Packet.alloc(1, -737);

	@ObfuscatedName("client.nc")
	public int[] archiveChecksum = new int[9];

	@ObfuscatedName("client.oc")
	public boolean midiThreadActive = true;

	@ObfuscatedName("client.pc")
	public int field1178 = 4277;

	@ObfuscatedName("client.qc")
	public Pix8[] imageSideicons = new Pix8[13];

	@ObfuscatedName("client.sc")
	public int orbitCameraPitch = 128;

	@ObfuscatedName("client.wc")
	public int MAX_PLAYER_COUNT = 2048;

	@ObfuscatedName("client.xc")
	public int LOCAL_PLAYER_INDEX = 2047;

	@ObfuscatedName("client.yc")
	public PlayerEntity[] players = new PlayerEntity[this.MAX_PLAYER_COUNT];

	@ObfuscatedName("client.Ac")
	public int[] playerIds = new int[this.MAX_PLAYER_COUNT];

	@ObfuscatedName("client.Cc")
	public int[] entityUpdateIds = new int[this.MAX_PLAYER_COUNT];

	@ObfuscatedName("client.Dc")
	public Packet[] playerAppearanceBuffer = new Packet[this.MAX_PLAYER_COUNT];

	@ObfuscatedName("client.Ic")
	public LinkList projectiles = new LinkList(0);

	@ObfuscatedName("client.Kc")
	public String[] menuOption = new String[500];

	@ObfuscatedName("client.Lc")
	public boolean midiActive = true;

	@ObfuscatedName("client.Mc")
	public boolean designGenderMale = true;

	@ObfuscatedName("client.Tc")
	public byte field1208 = 4;

	@ObfuscatedName("client.Zc")
	public boolean field1214 = false;

	@ObfuscatedName("client.ad")
	public int[] flameLineOffset = new int[256];

	@ObfuscatedName("client.bd")
	public byte field1216 = 2;

	@ObfuscatedName("client.gd")
	public int[] compassMaskLineOffsets = new int[33];

	@ObfuscatedName("client.id")
	public int[] waveDelay = new int[50];

	@ObfuscatedName("client.kd")
	public boolean field1225 = false;

	@ObfuscatedName("client.ld")
	public int[] tabInterfaceId = new int[] { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

	@ObfuscatedName("client.pd")
	public boolean errorLoading = false;

	@ObfuscatedName("client.qd")
	public byte field1231 = 99;

	@ObfuscatedName("client.td")
	public boolean showSocialInput = false;

	@ObfuscatedName("client.wd")
	public boolean pressedContinueOption = false;

	@ObfuscatedName("client.Bd")
	public final int field1242 = 100;

	@ObfuscatedName("client.Cd")
	public int[] messageIds = new int[100];

	@ObfuscatedName("client.Dd")
	public boolean menuVisible = false;

	@ObfuscatedName("client.Fd")
	public boolean reportAbuseMuteOption = false;

	@ObfuscatedName("client.Hd")
	public int field1248 = -655;

	@ObfuscatedName("client.Id")
	public LinkList spawnedLocations = new LinkList(0);

	@ObfuscatedName("client.Ud")
	public int[] messageType = new int[100];

	@ObfuscatedName("client.Vd")
	public String[] messageSender = new String[100];

	@ObfuscatedName("client.Wd")
	public String[] messageText = new String[100];

	@ObfuscatedName("client.ae")
	public boolean flameActive = false;

	@ObfuscatedName("client.fe")
	public int reportAbuseInterfaceID = -1;

	@ObfuscatedName("client.ke")
	public boolean field1277 = false;

	@ObfuscatedName("client.qe")
	public int[] activeMapFunctionX = new int[1000];

	@ObfuscatedName("client.re")
	public int[] activeMapFunctionZ = new int[1000];

	@ObfuscatedName("client.se")
	public int[][] tileLastOccupiedCycle = new int[104][104];

	@ObfuscatedName("client.te")
	public boolean redrawPrivacySettings = false;

	@ObfuscatedName("client.ve")
	public boolean errorHost = false;

	@ObfuscatedName("client.ye")
	public int[] skillBaseLevel = new int[50];

	@ObfuscatedName("client.ze")
	public NpcEntity[] npcs = new NpcEntity[8192];

	@ObfuscatedName("client.Be")
	public int[] npcIds = new int[8192];

	@ObfuscatedName("client.De")
	public int minimapZoomModifier = 1;

	@ObfuscatedName("client.He")
	public byte field1300 = -46;

	@ObfuscatedName("client.Ke")
	public int[] varps = new int[2000];

	@ObfuscatedName("client.Me")
	public int[] entityRemovalIds = new int[1000];

	@ObfuscatedName("client.Pe")
	public long[] friendName37 = new long[100];

	@ObfuscatedName("client.Ze")
	public int[] minimapMaskLineLengths = new int[151];

	@ObfuscatedName("client.af")
	public CollisionMap[] levelCollisionMap = new CollisionMap[4];

	@ObfuscatedName("client.cf")
	public Pix32[] imageHeadicons = new Pix32[20];

	@ObfuscatedName("client.ff")
	public int[] cameraModifierJitter = new int[5];

	@ObfuscatedName("client.gf")
	public boolean objGrabThreshold = false;

	@ObfuscatedName("client.lf")
	public boolean redrawSidebar = false;

	@ObfuscatedName("client.mf")
	public boolean redrawChatback = false;

	@ObfuscatedName("client.nf")
	public int[] cameraModifierWobbleScale = new int[5];

	@ObfuscatedName("client.pf")
	public boolean cutscene = false;

	@ObfuscatedName("client.rf")
	public String reportAbuseInput = "";

	@ObfuscatedName("client.sf")
	public int viewportInterfaceId = -1;

	@ObfuscatedName("client.vf")
	public boolean ingame = false;

	@ObfuscatedName("client.wf")
	public boolean flamesThread = false;

	@ObfuscatedName("client.Bf")
	public int SCROLLBAR_GRIP_LOWLIGHT = 3353893;

	@ObfuscatedName("client.Lf")
	public boolean field1355 = false;

	@ObfuscatedName("client.Of")
	public int SCROLLBAR_GRIP_HIGHLIGHT = 7759444;

	@ObfuscatedName("client.Pf")
	public int[] bfsStepX = new int[4000];

	@ObfuscatedName("client.Qf")
	public int[] bfsStepZ = new int[4000];

	@ObfuscatedName("client.Rf")
	public CRC32 crc32 = new CRC32();

	@ObfuscatedName("client.Wf")
	public int chatInterfaceId = -1;

	@ObfuscatedName("client.og")
	public int projectX = -1;

	@ObfuscatedName("client.pg")
	public int projectY = -1;

	@ObfuscatedName("client.qg")
	public int stickyChatInterfaceId = -1;

	@ObfuscatedName("client.sg")
	public boolean rights = false;

	@ObfuscatedName("client.tg")
	public int[] cameraModifierCycle = new int[5];

	@ObfuscatedName("client.Hg")
	public Pix8[] imageMapscene = new Pix8[50];

	@ObfuscatedName("client.Ig")
	public int field1404 = 27808;

	@ObfuscatedName("client.Og")
	public int[] CHAT_COLORS = new int[] { 16776960, 16711680, 65280, 65535, 16711935, 16777215 };

	@ObfuscatedName("client.Tg")
	public int SCROLLBAR_TRACK = 2301979;

	@ObfuscatedName("client.Yg")
	public boolean chatbackInputOpen = false;

	@ObfuscatedName("client.Zg")
	public LinkList spotanims = new LinkList(0);

	@ObfuscatedName("client.ih")
	public int lastWaveLoops = -1;

	@ObfuscatedName("client.jh")
	public String username = "";

	@ObfuscatedName("client.kh")
	public String password = "";

	@ObfuscatedName("client.lh")
	public byte field1433 = 7;

	@ObfuscatedName("client.mh")
	public byte[] textureBuffer = new byte[16384];

	@ObfuscatedName("client.nh")
	public boolean errorStarted = false;

	@ObfuscatedName("client.oh")
	public int field1436 = 242;

	@ObfuscatedName("client.sh")
	public int[] varCache = new int[2000];

	@ObfuscatedName("client.wh")
	public int[] skillExperience = new int[50];

	@ObfuscatedName("client.xh")
	public boolean redrawSideicons = false;

	@ObfuscatedName("client.Ah")
	public String loginMessage0 = "";

	@ObfuscatedName("client.Bh")
	public String loginMessage1 = "";

	@ObfuscatedName("client.Dh")
	public int minimapAngleModifier = 2;

	@ObfuscatedName("client.Jh")
	public int MAX_CHATS = 50;

	@ObfuscatedName("client.Kh")
	public int[] chatX = new int[this.MAX_CHATS];

	@ObfuscatedName("client.Lh")
	public int[] chatY = new int[this.MAX_CHATS];

	@ObfuscatedName("client.Mh")
	public int[] chatHeight = new int[this.MAX_CHATS];

	@ObfuscatedName("client.Nh")
	public int[] chatWidth = new int[this.MAX_CHATS];

	@ObfuscatedName("client.Oh")
	public int[] chatColors = new int[this.MAX_CHATS];

	@ObfuscatedName("client.Ph")
	public int[] chatStyles = new int[this.MAX_CHATS];

	@ObfuscatedName("client.Qh")
	public int[] chatTimers = new int[this.MAX_CHATS];

	@ObfuscatedName("client.Rh")
	public String[] chats = new String[this.MAX_CHATS];

	@ObfuscatedName("client.Wh")
	public int field1470 = -986;

	@ObfuscatedName("client.Xh")
	public int field1471 = -35388;

	@ObfuscatedName("client.Yh")
	public final int[] LOC_SHAPE_TO_LAYER = new int[] { 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3 };

	@ObfuscatedName("client.ii")
	public int[] compassMaskLineLengths = new int[33];

	@ObfuscatedName("client.ji")
	public int[][] bfsDirection = new int[104][104];

	@ObfuscatedName("client.li")
	public Pix32[] imageCrosses = new Pix32[8];

	@ObfuscatedName("client.mi")
	public boolean flameThread = false;

	@ObfuscatedName("client.ni")
	public byte field1487 = 94;

	@ObfuscatedName("client.oi")
	public Object midiSync = new Object();

	@ObfuscatedName("client.pi")
	public int[] waveIds = new int[50];

	@ObfuscatedName("client.ri")
	public int cameraOffsetXModifier = 2;

	@ObfuscatedName("client.si")
	public String[] friendName = new String[100];

	@ObfuscatedName("client.ti")
	public int flashingTab = -1;

	@ObfuscatedName("client.ui")
	public int sidebarInterfaceId = -1;

	@ObfuscatedName("client.wi")
	public int cameraOffsetZModifier = 2;

	@ObfuscatedName("client.xi")
	public int field1497 = 29286;

	@ObfuscatedName("client.yi")
	public int[] minimapMaskLineOffsets = new int[151];

	@ObfuscatedName("client.Ai")
	public int cameraOffsetYawModifier = 1;

	@ObfuscatedName("client.Ci")
	public String chatTyped = "";

	@ObfuscatedName("client.Di")
	public Pix32[] imageMapfunction = new Pix32[50];

	@ObfuscatedName("client.Ei")
	public int[] menuParamB = new int[500];

	@ObfuscatedName("client.Fi")
	public int[] menuParamC = new int[500];

	@ObfuscatedName("client.Gi")
	public int[] menuAction = new int[500];

	@ObfuscatedName("client.Hi")
	public int[] menuParamA = new int[500];

	@ObfuscatedName("client.Ii")
	public int field1508 = 701;

	@ObfuscatedName("client.Ji")
	public boolean scrollGrabbed = false;

	@ObfuscatedName("client.Mi")
	public boolean field1512 = true;

	@ObfuscatedName("client.Si")
	public boolean waveEnabled = true;

	@ObfuscatedName("client.Wi")
	public LinkList[][][] levelObjStacks = new LinkList[4][104][104];

	@ObfuscatedName("client.Xi")
	public int SCROLLBAR_GRIP_FOREGROUND = 5063219;

	@ObfuscatedName("client.Yi")
	public int[] cameraModifierWobbleSpeed = new int[5];

	@ObfuscatedName("client.Zi")
	public int field1525 = -676;

	@ObfuscatedName("client.F")
	public static String CHARSET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!\"Â£$%^&*()-_=+[{]};:'@#~,<.>/?\\| ";

	@ObfuscatedName("client.od")
	public static int[] levelExperience = new int[99];

	@ObfuscatedName("client.ud")
	public static boolean field1235;

	@ObfuscatedName("client.vd")
	public static byte field1236;

	@ObfuscatedName("client.Kd")
	public static int nodeId;

	@ObfuscatedName("client.Md")
	public static boolean members;

	@ObfuscatedName("client.Xd")
	public static int field1264;

	@ObfuscatedName("client.ue")
	public static BigInteger RSA_EXPONENT;

	@ObfuscatedName("client.Oe")
	public static final int[][] field1307;

	@ObfuscatedName("client.ef")
	public static int field1323;

	@ObfuscatedName("client.fh")
	public static BigInteger RSA_MODULUS;

	@ObfuscatedName("client.qh")
	public static final int[] field1438;

	@ObfuscatedName("client.hi")
	public static byte field1481;

	@ObfuscatedName("client.E")
	public static int oplogic5;

	@ObfuscatedName("client.G")
	public int midiSyncLen;

	@ObfuscatedName("client.H")
	public int field1092;

	@ObfuscatedName("client.I")
	public int cutsceneSrcLocalTileX;

	@ObfuscatedName("client.J")
	public int cutsceneSrcLocalTileZ;

	@ObfuscatedName("client.K")
	public int cutsceneSrcHeight;

	@ObfuscatedName("client.L")
	public int cutsceneMoveSpeed;

	@ObfuscatedName("client.M")
	public int cutsceneMoveAcceleration;

	@ObfuscatedName("client.S")
	public int crossX;

	@ObfuscatedName("client.T")
	public int crossY;

	@ObfuscatedName("client.U")
	public int crossCycle;

	@ObfuscatedName("client.V")
	public int crossMode;

	@ObfuscatedName("client.Y")
	public int nextMusicDelay;

	@ObfuscatedName("client.Z")
	public int hintTileX;

	@ObfuscatedName("client.Ab")
	public static int oplogic1;

	@ObfuscatedName("client.Hb")
	public int packetSize;

	@ObfuscatedName("client.Ib")
	public int packetType;

	@ObfuscatedName("client.Jb")
	public int idleNetCycles;

	@ObfuscatedName("client.Kb")
	public int heartbeatTimer;

	@ObfuscatedName("client.Lb")
	public int idleTimeout;

	@ObfuscatedName("client.Nb")
	public int cameraOffsetCycle;

	@ObfuscatedName("client.Vb")
	public int ignoreCount;

	@ObfuscatedName("client.Yb")
	public int field1161;

	@ObfuscatedName("client.Zb")
	public static int oplogic4;

	@ObfuscatedName("client.rc")
	public int lastWaveLength;

	@ObfuscatedName("client.tc")
	public int orbitCameraYaw;

	@ObfuscatedName("client.uc")
	public int orbitCameraYawVelocity;

	@ObfuscatedName("client.vc")
	public int orbitCameraPitchVelocity;

	@ObfuscatedName("client.zc")
	public int playerCount;

	@ObfuscatedName("client.Bc")
	public int entityUpdateCount;

	@ObfuscatedName("client.Ec")
	public int lastPacketType0;

	@ObfuscatedName("client.Fc")
	public int lastPacketType1;

	@ObfuscatedName("client.Gc")
	public int lastPacketType2;

	@ObfuscatedName("client.Jc")
	public int splitPrivateChat;

	@ObfuscatedName("client.Nc")
	public int sceneCycle;

	@ObfuscatedName("client.Oc")
	public int sceneCenterZoneX;

	@ObfuscatedName("client.Pc")
	public int sceneCenterZoneZ;

	@ObfuscatedName("client.Uc")
	public int objDragInterfaceId;

	@ObfuscatedName("client.Vc")
	public int objDragSlot;

	@ObfuscatedName("client.Wc")
	public int objDragArea;

	@ObfuscatedName("client.Xc")
	public int objGrabX;

	@ObfuscatedName("client.Yc")
	public int objGrabY;

	@ObfuscatedName("client.fd")
	public int privateMessageCount;

	@ObfuscatedName("client.hd")
	public static int oplogic6;

	@ObfuscatedName("client.jd")
	public int chatHoveredInterfaceIndex;

	@ObfuscatedName("client.md")
	public int baseX;

	@ObfuscatedName("client.nd")
	public int baseZ;

	@ObfuscatedName("client.rd")
	public static int oplogic2;

	@ObfuscatedName("client.sd")
	public int lastHoveredInterfaceId;

	@ObfuscatedName("client.xd")
	public int daysSinceLastLogin;

	@ObfuscatedName("client.yd")
	public int flameGradientCycle0;

	@ObfuscatedName("client.zd")
	public int flameGradientCycle1;

	@ObfuscatedName("client.Ad")
	public static int oplogic9;

	@ObfuscatedName("client.Ed")
	public int currentLevel;

	@ObfuscatedName("client.Gd")
	public static int cyclelogic1;

	@ObfuscatedName("client.Jd")
	public int tradeChatSetting;

	@ObfuscatedName("client.Ld")
	public static int portOffset;

	@ObfuscatedName("client.Od")
	public static int oplogic8;

	@ObfuscatedName("client.Zd")
	public int daysSinceRecoveriesChanged;

	@ObfuscatedName("client.je")
	public int hintType;

	@ObfuscatedName("client.le")
	public static int cyclelogic6;

	@ObfuscatedName("client.me")
	public int orbitCameraX;

	@ObfuscatedName("client.ne")
	public int orbitCameraZ;

	@ObfuscatedName("client.oe")
	public int cameraMovedWrite;

	@ObfuscatedName("client.pe")
	public int activeMapFunctionCount;

	@ObfuscatedName("client.we")
	public int objDragCycles;

	@ObfuscatedName("client.Ae")
	public int npcCount;

	@ObfuscatedName("client.Ce")
	public int minimapZoom;

	@ObfuscatedName("client.Ee")
	public int cameraPitchClamp;

	@ObfuscatedName("client.Fe")
	public int worldLocationState;

	@ObfuscatedName("client.Ge")
	public int dragCycles;

	@ObfuscatedName("client.Je")
	public static int oplogic7;

	@ObfuscatedName("client.Le")
	public int entityRemovalCount;

	@ObfuscatedName("client.Ne")
	public int sidebarHoveredInterfaceIndex;

	@ObfuscatedName("client.Qe")
	public int selectedCycle;

	@ObfuscatedName("client.Re")
	public int selectedInterface;

	@ObfuscatedName("client.Se")
	public int selectedItem;

	@ObfuscatedName("client.Te")
	public int selectedArea;

	@ObfuscatedName("client.Ue")
	public int cutsceneDstLocalTileX;

	@ObfuscatedName("client.Ve")
	public int cutsceneDstLocalTileZ;

	@ObfuscatedName("client.We")
	public int cutsceneDstHeight;

	@ObfuscatedName("client.Xe")
	public int cutsceneRotateSpeed;

	@ObfuscatedName("client.Ye")
	public int cutsceneRotateAcceleration;

	@ObfuscatedName("client.bf")
	public static int loopCycle;

	@ObfuscatedName("client.df")
	public int systemUpdateTimer;

	@ObfuscatedName("client.kf")
	public int midiSyncCrc;

	@ObfuscatedName("client.qf")
	public int sceneDelta;

	@ObfuscatedName("client.tf")
	public int titleLoginField;

	@ObfuscatedName("client.xf")
	public int publicChatSetting;

	@ObfuscatedName("client.yf")
	public int chatScrollOffset;

	@ObfuscatedName("client.Ff")
	public int inMultizone;

	@ObfuscatedName("client.Kf")
	public int tryMoveNearest;

	@ObfuscatedName("client.Tf")
	public static int cyclelogic3;

	@ObfuscatedName("client.Xf")
	public int objSelected;

	@ObfuscatedName("client.Yf")
	public int objSelectedSlot;

	@ObfuscatedName("client.Zf")
	public int objSelectedInterface;

	@ObfuscatedName("client.ag")
	public int objInterface;

	@ObfuscatedName("client.mg")
	public static int field1382;

	@ObfuscatedName("client.ng")
	public int waveCount;

	@ObfuscatedName("client.rg")
	public int field1387;

	@ObfuscatedName("client.ug")
	public int spellSelected;

	@ObfuscatedName("client.vg")
	public int activeSpellId;

	@ObfuscatedName("client.wg")
	public int activeSpellFlags;

	@ObfuscatedName("client.Ug")
	public int flagSceneTileX;

	@ObfuscatedName("client.Vg")
	public int flagSceneTileZ;

	@ObfuscatedName("client.Xg")
	public int unreadMessages;

	@ObfuscatedName("client.eh")
	public int lastAddress;

	@ObfuscatedName("client.gh")
	public int viewportHoveredInterfaceIndex;

	@ObfuscatedName("client.ph")
	public int energy;

	@ObfuscatedName("client.rh")
	public int menuSize;

	@ObfuscatedName("client.th")
	public int hintPlayer;

	@ObfuscatedName("client.uh")
	public int field1442;

	@ObfuscatedName("client.vh")
	public int sceneState;

	@ObfuscatedName("client.Ch")
	public int minimapAnticheatAngle;

	@ObfuscatedName("client.Eh")
	public int hoveredSlot;

	@ObfuscatedName("client.Fh")
	public int hoveredSlotParentId;

	@ObfuscatedName("client.Gh")
	public int friendCount;

	@ObfuscatedName("client.Hh")
	public static int cyclelogic4;

	@ObfuscatedName("client.Ih")
	public int chatCount;

	@ObfuscatedName("client.Sh")
	public int wildernessLevel;

	@ObfuscatedName("client.Zh")
	public static int cyclelogic5;

	@ObfuscatedName("client.ai")
	public int titleScreenState;

	@ObfuscatedName("client.bi")
	public int midiCrc;

	@ObfuscatedName("client.ci")
	public int cameraX;

	@ObfuscatedName("client.di")
	public int cameraY;

	@ObfuscatedName("client.ei")
	public int cameraZ;

	@ObfuscatedName("client.fi")
	public int cameraPitch;

	@ObfuscatedName("client.gi")
	public int cameraYaw;

	@ObfuscatedName("client.qi")
	public int cameraAnticheatOffsetX;

	@ObfuscatedName("client.vi")
	public int cameraAnticheatOffsetZ;

	@ObfuscatedName("client.zi")
	public int cameraAnticheatAngle;

	@ObfuscatedName("client.Ni")
	public int menuArea;

	@ObfuscatedName("client.Oi")
	public int menuX;

	@ObfuscatedName("client.Pi")
	public int menuY;

	@ObfuscatedName("client.Qi")
	public int menuWidth;

	@ObfuscatedName("client.Ri")
	public int menuHeight;

	@ObfuscatedName("client.Ti")
	public int scrollInputPadding;

	@ObfuscatedName("client.Ui")
	public int midiSize;

	@ObfuscatedName("client.Vi")
	public int flameCycle0;

	@ObfuscatedName("client.Fb")
	public long lastWaveStartTime;

	@ObfuscatedName("client.Yd")
	public long socialName37;

	@ObfuscatedName("client.Li")
	public long serverSeed;

	@ObfuscatedName("client.Hc")
	public World3D scene;

	@ObfuscatedName("client.of")
	public PlayerEntity localPlayer;

	@ObfuscatedName("client.hf")
	public Pix32 genderButtonImage0;

	@ObfuscatedName("client.jf")
	public Pix32 genderButtonImage1;

	@ObfuscatedName("client.zf")
	public Pix32 imageFlamesLeft;

	@ObfuscatedName("client.Af")
	public Pix32 imageFlamesRight;

	@ObfuscatedName("client.Sf")
	public Pix32 imageMapflag;

	@ObfuscatedName("client.Wg")
	public Pix32 imageMinimap;

	@ObfuscatedName("client.ah")
	public Pix32 imageMapdot0;

	@ObfuscatedName("client.bh")
	public Pix32 imageMapdot1;

	@ObfuscatedName("client.ch")
	public Pix32 imageMapdot2;

	@ObfuscatedName("client.dh")
	public Pix32 imageMapdot3;

	@ObfuscatedName("client.Ki")
	public Pix32 imageCompass;

	@ObfuscatedName("client.Pd")
	public Pix8 imageRedstone1;

	@ObfuscatedName("client.Qd")
	public Pix8 imageRedstone2;

	@ObfuscatedName("client.Rd")
	public Pix8 imageRedstone3;

	@ObfuscatedName("client.Sd")
	public Pix8 imageRedstone1h;

	@ObfuscatedName("client.Td")
	public Pix8 imageRedstone2h;

	@ObfuscatedName("client.ge")
	public Pix8 imageBackbase1;

	@ObfuscatedName("client.he")
	public Pix8 imageBackbase2;

	@ObfuscatedName("client.ie")
	public Pix8 imageBackhmid1;

	@ObfuscatedName("client.Cf")
	public Pix8 imageInvback;

	@ObfuscatedName("client.Df")
	public Pix8 imageMapback;

	@ObfuscatedName("client.Ef")
	public Pix8 imageChatback;

	@ObfuscatedName("client.Jg")
	public Pix8 imageRedstone1v;

	@ObfuscatedName("client.Kg")
	public Pix8 imageRedstone2v;

	@ObfuscatedName("client.Lg")
	public Pix8 imageRedstone3v;

	@ObfuscatedName("client.Mg")
	public Pix8 imageRedstone1hv;

	@ObfuscatedName("client.Ng")
	public Pix8 imageRedstone2hv;

	@ObfuscatedName("client.yh")
	public Pix8 imageScrollbar0;

	@ObfuscatedName("client.zh")
	public Pix8 imageScrollbar1;

	@ObfuscatedName("client.Uh")
	public Pix8 imageTitlebox;

	@ObfuscatedName("client.Vh")
	public Pix8 imageTitlebutton;

	@ObfuscatedName("client.Gf")
	public PixFont fontPlain11;

	@ObfuscatedName("client.Hf")
	public PixFont fontPlain12;

	@ObfuscatedName("client.If")
	public PixFont fontBold12;

	@ObfuscatedName("client.Jf")
	public PixFont fontQuill8;

	@ObfuscatedName("client.cd")
	public PixMap areaBackbase1;

	@ObfuscatedName("client.dd")
	public PixMap areaBackbase2;

	@ObfuscatedName("client.ed")
	public PixMap areaBackhmid1;

	@ObfuscatedName("client.cg")
	public PixMap areaBackleft1;

	@ObfuscatedName("client.dg")
	public PixMap areaBackleft2;

	@ObfuscatedName("client.eg")
	public PixMap areaBackright1;

	@ObfuscatedName("client.fg")
	public PixMap areaBackright2;

	@ObfuscatedName("client.gg")
	public PixMap areaBacktop1;

	@ObfuscatedName("client.hg")
	public PixMap areaBacktop2;

	@ObfuscatedName("client.ig")
	public PixMap areaBackvmid1;

	@ObfuscatedName("client.jg")
	public PixMap areaBackvmid2;

	@ObfuscatedName("client.kg")
	public PixMap areaBackvmid3;

	@ObfuscatedName("client.lg")
	public PixMap areaBackhmid2;

	@ObfuscatedName("client.yg")
	public PixMap imageTitle2;

	@ObfuscatedName("client.zg")
	public PixMap imageTitle3;

	@ObfuscatedName("client.Ag")
	public PixMap imageTitle4;

	@ObfuscatedName("client.Bg")
	public PixMap imageTitle0;

	@ObfuscatedName("client.Cg")
	public PixMap imageTitle1;

	@ObfuscatedName("client.Dg")
	public PixMap imageTitle5;

	@ObfuscatedName("client.Eg")
	public PixMap imageTitle6;

	@ObfuscatedName("client.Fg")
	public PixMap imageTitle7;

	@ObfuscatedName("client.Gg")
	public PixMap imageTitle8;

	@ObfuscatedName("client.Pg")
	public PixMap areaSidebar;

	@ObfuscatedName("client.Qg")
	public PixMap areaMapback;

	@ObfuscatedName("client.Rg")
	public PixMap areaViewport;

	@ObfuscatedName("client.Sg")
	public PixMap areaChatback;

	@ObfuscatedName("client.Bi")
	public Jagfile archiveTitle;

	@ObfuscatedName("client.Uf")
	public ClientStream stream;

	@ObfuscatedName("client.Ie")
	public String modalMessage;

	@ObfuscatedName("client.bg")
	public String objSelectedName;

	@ObfuscatedName("client.xg")
	public String spellCaption;

	@ObfuscatedName("client.hh")
	public String midiSyncName;

	@ObfuscatedName("client.ki")
	public String currentMidi;

	@ObfuscatedName("client.Nd")
	public static boolean lowMemory;

	@ObfuscatedName("client.Th")
	public static boolean started;

	@ObfuscatedName("client.P")
	public int[] areaChatbackOffsets;

	@ObfuscatedName("client.Q")
	public int[] areaSidebarOffsets;

	@ObfuscatedName("client.R")
	public int[] areaViewportOffsets;

	@ObfuscatedName("client.Rc")
	public int[] flameBuffer0;

	@ObfuscatedName("client.Sc")
	public int[] flameBuffer1;

	@ObfuscatedName("client.be")
	public int[] flameGradient;

	@ObfuscatedName("client.ce")
	public int[] flameGradient0;

	@ObfuscatedName("client.de")
	public int[] flameGradient1;

	@ObfuscatedName("client.ee")
	public int[] flameGradient2;

	@ObfuscatedName("client.xe")
	public int[] sceneMapIndex;

	@ObfuscatedName("client.Mf")
	public int[] flameBuffer3;

	@ObfuscatedName("client.Nf")
	public int[] flameBuffer2;

	@ObfuscatedName("client.uf")
	public Pix8[] imageRunes;

	@ObfuscatedName("client.Vf")
	public byte[][] sceneMapLocData;

	@ObfuscatedName("client.Qc")
	public byte[][][] levelTileFlags;

	@ObfuscatedName("client.Wb")
	public int[][][] levelHeightmap;

	@ObfuscatedName("client.a(ZILjava/lang/String;I)V")
	public final void setMidi(boolean arg0, int arg1, String arg2, int arg3) {
		if (arg2 == null) {
			return;
		}
		Object var5 = this.midiSync;
		synchronized (this.midiSync) {
			this.midiSyncName = arg2;
			this.midiSyncCrc = arg1;
			this.midiSyncLen = arg3;
		}
		if (arg0) {
			this.packetType = -1;
		}
	}

	@ObfuscatedName("client.d(I)V")
	public final void draw2DEntityElements(int arg0) {
		this.chatCount = 0;
		while (arg0 >= 0) {
			this.field1248 = this.randomIn.takeNextValue();
		}
		int var4;
		int var6;
		for (int var2 = -1; var2 < this.playerCount + this.npcCount; var2++) {
			PathingEntity var3;
			if (var2 == -1) {
				var3 = this.localPlayer;
			} else if (var2 < this.playerCount) {
				var3 = this.players[this.playerIds[var2]];
			} else {
				var3 = this.npcs[this.npcIds[var2 - this.playerCount]];
			}
			if (var3 != null && var3.isVisible(false)) {
				if (var2 < this.playerCount) {
					var4 = 30;
					PlayerEntity var5 = (PlayerEntity) var3;
					if (var5.headicons != 0) {
						this.projectFromGround(var3.height + 15, this.field1225, var3);
						if (this.projectX > -1) {
							for (var6 = 0; var6 < 8; var6++) {
								if ((var5.headicons & 0x1 << var6) != 0) {
									this.imageHeadicons[var6].draw(this.projectY - var4, this.projectX - 12, false);
									var4 -= 25;
								}
							}
						}
					}
					if (var2 >= 0 && this.hintType == 10 && this.hintPlayer == this.playerIds[var2]) {
						this.projectFromGround(var3.height + 15, this.field1225, var3);
						if (this.projectX > -1) {
							this.imageHeadicons[7].draw(this.projectY - var4, this.projectX - 12, false);
						}
					}
				} else if (this.hintType == 1 && this.hintNpc == this.npcIds[var2 - this.playerCount] && loopCycle % 20 < 10) {
					this.projectFromGround(var3.height + 15, this.field1225, var3);
					if (this.projectX > -1) {
						this.imageHeadicons[2].draw(this.projectY - 28, this.projectX - 12, false);
					}
				}
				if (var3.chat != null && (var2 >= this.playerCount || this.publicChatSetting == 0 || this.publicChatSetting == 3 || this.publicChatSetting == 1 && this.isFriend(-20, ((PlayerEntity) var3).name))) {
					this.projectFromGround(var3.height, this.field1225, var3);
					if (this.projectX > -1 && this.chatCount < this.MAX_CHATS) {
						this.chatWidth[this.chatCount] = this.fontBold12.stringWidth(false, var3.chat) / 2;
						this.chatHeight[this.chatCount] = this.fontBold12.height;
						this.chatX[this.chatCount] = this.projectX;
						this.chatY[this.chatCount] = this.projectY;
						this.chatColors[this.chatCount] = var3.chatColor;
						this.chatStyles[this.chatCount] = var3.chatStyle;
						this.chatTimers[this.chatCount] = var3.chatTimer;
						this.chats[this.chatCount++] = var3.chat;
						if (this.chatEffects == 0 && var3.chatStyle == 1) {
							this.chatHeight[this.chatCount] += 10;
							this.chatY[this.chatCount] += 5;
						}
						if (this.chatEffects == 0 && var3.chatStyle == 2) {
							this.chatWidth[this.chatCount] = 60;
						}
					}
				}
				if (var3.combatCycle > loopCycle + 100) {
					this.projectFromGround(var3.height + 15, this.field1225, var3);
					if (this.projectX > -1) {
						var4 = var3.health * 30 / var3.totalHealth;
						if (var4 > 30) {
							var4 = 30;
						}
						Pix2D.fillRect(this.projectY - 3, this.projectX - 15, 65280, (byte) 93, var4, 5);
						Pix2D.fillRect(this.projectY - 3, this.projectX - 15 + var4, 16711680, (byte) 93, 30 - var4, 5);
					}
				}
				if (var3.combatCycle > loopCycle + 330) {
					this.projectFromGround(var3.height / 2, this.field1225, var3);
					if (this.projectX > -1) {
						this.imageHitmarks[var3.damageType].draw(this.projectY - 12, this.projectX - 12, false);
						this.fontPlain11.drawStringCenter(this.projectY + 4, (byte) 6, 0, String.valueOf(var3.damage), this.projectX);
						this.fontPlain11.drawStringCenter(this.projectY + 3, (byte) 6, 16777215, String.valueOf(var3.damage), this.projectX - 1);
					}
				}
			}
		}
		for (int var13 = 0; var13 < this.chatCount; var13++) {
			var4 = this.chatX[var13];
			int var14 = this.chatY[var13];
			var6 = this.chatWidth[var13];
			int var7 = this.chatHeight[var13];
			boolean var8 = true;
			while (var8) {
				var8 = false;
				for (int var9 = 0; var9 < var13; var9++) {
					if (var14 + 2 > this.chatY[var9] - this.chatHeight[var9] && var14 - var7 < this.chatY[var9] + 2 && var4 - var6 < this.chatX[var9] + this.chatWidth[var9] && var4 + var6 > this.chatX[var9] - this.chatWidth[var9] && this.chatY[var9] - this.chatHeight[var9] < var14) {
						var14 = this.chatY[var9] - this.chatHeight[var9];
						var8 = true;
					}
				}
			}
			this.projectX = this.chatX[var13];
			this.projectY = this.chatY[var13] = var14;
			String var15 = this.chats[var13];
			if (this.chatEffects == 0) {
				int var10 = 16776960;
				if (this.chatColors[var13] < 6) {
					var10 = this.CHAT_COLORS[this.chatColors[var13]];
				}
				if (this.chatColors[var13] == 6) {
					var10 = this.sceneCycle % 20 < 10 ? 16711680 : 16776960;
				}
				if (this.chatColors[var13] == 7) {
					var10 = this.sceneCycle % 20 < 10 ? 255 : 65535;
				}
				if (this.chatColors[var13] == 8) {
					var10 = this.sceneCycle % 20 < 10 ? 45056 : 8454016;
				}
				int var11;
				if (this.chatColors[var13] == 9) {
					var11 = 150 - this.chatTimers[var13];
					if (var11 < 50) {
						var10 = var11 * 1280 + 16711680;
					} else if (var11 < 100) {
						var10 = 16776960 - (var11 - 50) * 327680;
					} else if (var11 < 150) {
						var10 = (var11 - 100) * 5 + 65280;
					}
				}
				if (this.chatColors[var13] == 10) {
					var11 = 150 - this.chatTimers[var13];
					if (var11 < 50) {
						var10 = var11 * 5 + 16711680;
					} else if (var11 < 100) {
						var10 = 16711935 - (var11 - 50) * 327680;
					} else if (var11 < 150) {
						var10 = (var11 - 100) * 327680 + 255 - (var11 - 100) * 5;
					}
				}
				if (this.chatColors[var13] == 11) {
					var11 = 150 - this.chatTimers[var13];
					if (var11 < 50) {
						var10 = 16777215 - var11 * 327685;
					} else if (var11 < 100) {
						var10 = (var11 - 50) * 327685 + 65280;
					} else if (var11 < 150) {
						var10 = 16777215 - (var11 - 100) * 327680;
					}
				}
				if (this.chatStyles[var13] == 0) {
					this.fontBold12.drawStringCenter(this.projectY + 1, (byte) 6, 0, var15, this.projectX);
					this.fontBold12.drawStringCenter(this.projectY, (byte) 6, var10, var15, this.projectX);
				}
				if (this.chatStyles[var13] == 1) {
					this.fontBold12.drawCenteredWave(this.sceneCycle, (byte) 8, this.projectX, this.projectY + 1, 0, var15);
					this.fontBold12.drawCenteredWave(this.sceneCycle, (byte) 8, this.projectX, this.projectY, var10, var15);
				}
				if (this.chatStyles[var13] == 2) {
					var11 = this.fontBold12.stringWidth(false, var15);
					int var12 = (150 - this.chatTimers[var13]) * (var11 + 100) / 150;
					Pix2D.setClipping(334, 0, this.projectX + 50, 789, this.projectX - 50);
					this.fontBold12.drawString(this.projectX + 50 - var12, this.projectY + 1, false, 0, var15);
					this.fontBold12.drawString(this.projectX + 50 - var12, this.projectY, false, var10, var15);
					Pix2D.resetClipping(0);
				}
			} else {
				this.fontBold12.drawStringCenter(this.projectY + 1, (byte) 6, 0, var15, this.projectX);
				this.fontBold12.drawStringCenter(this.projectY, (byte) 6, 16776960, var15, this.projectX);
			}
		}
	}

	@ObfuscatedName("client.c(B)V")
	public final void closeInterfaces(byte arg0) {
		if (arg0 != -60) {
			this.levelObjStacks = null;
		}
		this.out.p1isaac((byte) -34, 231);
		if (this.sidebarInterfaceId != -1) {
			this.sidebarInterfaceId = -1;
			this.redrawSidebar = true;
			this.pressedContinueOption = false;
			this.redrawSideicons = true;
		}
		if (this.chatInterfaceId != -1) {
			this.chatInterfaceId = -1;
			this.redrawChatback = true;
			this.pressedContinueOption = false;
		}
		this.viewportInterfaceId = -1;
	}

	@ObfuscatedName("client.e(I)V")
	public final void stopMidi(int arg0) {
		if (arg0 != 0) {
			this.load();
		}
		signlink.midifade = 0;
		signlink.midi = "stop";
	}

	@ObfuscatedName("client.f(I)V")
	public final void drawWildyLevel(int arg0) {
		int var2 = (this.localPlayer.x >> 7) + this.sceneBaseTileX;
		int var3 = (this.localPlayer.z >> 7) + this.sceneBaseTileZ;
		if (arg0 != 39734) {
			this.field1277 = !this.field1277;
		}
		if (var2 >= 2944 && var2 < 3392 && var3 >= 3520 && var3 < 6400) {
			this.wildernessLevel = (var3 - 3520) / 8 + 1;
		} else if (var2 >= 2944 && var2 < 3392 && var3 >= 9920 && var3 < 12800) {
			this.wildernessLevel = (var3 - 9920) / 8 + 1;
		} else {
			this.wildernessLevel = 0;
		}
		this.worldLocationState = 0;
		if (var2 >= 3328 && var2 < 3392 && var3 >= 3200 && var3 < 3264) {
			int var4 = var2 & 0x3F;
			int var5 = var3 & 0x3F;
			if (var4 >= 4 && var4 <= 29 && var5 >= 44 && var5 <= 58) {
				this.worldLocationState = 1;
			}
			if (var4 >= 36 && var4 <= 61 && var5 >= 44 && var5 <= 58) {
				this.worldLocationState = 1;
			}
			if (var4 >= 4 && var4 <= 29 && var5 >= 25 && var5 <= 39) {
				this.worldLocationState = 1;
			}
			if (var4 >= 36 && var4 <= 61 && var5 >= 25 && var5 <= 39) {
				this.worldLocationState = 1;
			}
			if (var4 >= 4 && var4 <= 29 && var5 >= 6 && var5 <= 20) {
				this.worldLocationState = 1;
			}
			if (var4 >= 36 && var4 <= 61 && var5 >= 6 && var5 <= 20) {
				this.worldLocationState = 1;
			}
		}
		if (this.worldLocationState == 0 && var2 >= 3328 && var2 <= 3393 && var3 >= 3203 && var3 <= 3325) {
			this.worldLocationState = 2;
		}
		this.overrideChat = 0;
		if (var2 >= 3053 && var2 <= 3156 && var3 >= 3056 && var3 <= 3136) {
			this.overrideChat = 1;
		}
		if (var2 >= 3072 && var2 <= 3118 && var3 >= 9492 && var3 <= 9535) {
			this.overrideChat = 1;
		}
		if (this.overrideChat == 1 && var2 >= 3139 && var2 <= 3199 && var3 >= 3008 && var3 <= 3062) {
			this.overrideChat = 0;
		}
	}

	@ObfuscatedName("client.g(I)V")
	public final void drawPrivateMessages(int arg0) {
		if (arg0 <= 0 || this.splitPrivateChat == 0) {
			return;
		}
		PixFont var2 = this.fontPlain12;
		int var3 = 0;
		if (this.systemUpdateTimer != 0) {
			var3 = 1;
		}
		for (int var4 = 0; var4 < 100; var4++) {
			if (this.messageText[var4] != null) {
				int var5 = this.messageType[var4];
				int var6;
				if ((var5 == 3 || var5 == 7) && (var5 == 7 || this.privateChatSetting == 0 || this.privateChatSetting == 1 && this.isFriend(-20, this.messageSender[var4]))) {
					var6 = 329 - var3 * 13;
					var2.drawString(4, var6, false, 0, "From " + this.messageSender[var4] + ": " + this.messageText[var4]);
					var2.drawString(4, var6 - 1, false, 65535, "From " + this.messageSender[var4] + ": " + this.messageText[var4]);
					var3++;
					if (var3 >= 5) {
						return;
					}
				}
				if (var5 == 5 && this.privateChatSetting < 2) {
					var6 = 329 - var3 * 13;
					var2.drawString(4, var6, false, 0, this.messageText[var4]);
					var2.drawString(4, var6 - 1, false, 65535, this.messageText[var4]);
					var3++;
					if (var3 >= 5) {
						return;
					}
				}
				if (var5 == 6 && this.privateChatSetting < 2) {
					var6 = 329 - var3 * 13;
					var2.drawString(4, var6, false, 0, "To " + this.messageSender[var4] + ": " + this.messageText[var4]);
					var2.drawString(4, var6 - 1, false, 65535, "To " + this.messageSender[var4] + ": " + this.messageText[var4]);
					var3++;
					if (var3 >= 5) {
						return;
					}
				}
			}
		}
	}

	@ObfuscatedName("client.a(Lkb;II)V")
	public final void getNpcPosExtended(Packet arg0, int arg1, int arg2) {
		for (int var4 = 0; var4 < this.entityUpdateCount; var4++) {
			int var5 = this.entityUpdateIds[var4];
			NpcEntity var6 = this.npcs[var5];
			int var7 = arg0.g1();
			int var8;
			if ((var7 & 0x2) == 2) {
				var8 = arg0.g2();
				if (var8 == 65535) {
					var8 = -1;
				}
				if (var8 == var6.primarySeqId) {
					var6.primarySeqLoop = 0;
				}
				int var9 = arg0.g1();
				if (var8 == -1 || var6.primarySeqId == -1 || SeqType.instances[var8].priority > SeqType.instances[var6.primarySeqId].priority || SeqType.instances[var6.primarySeqId].priority == 0) {
					var6.primarySeqId = var8;
					var6.primarySeqFrame = 0;
					var6.primarySeqCycle = 0;
					var6.primarySeqDelay = var9;
					var6.primarySeqLoop = 0;
				}
			}
			if ((var7 & 0x4) == 4) {
				var6.targetId = arg0.g2();
				if (var6.targetId == 65535) {
					var6.targetId = -1;
				}
			}
			if ((var7 & 0x8) == 8) {
				var6.chat = arg0.gjstr();
				var6.chatTimer = 100;
			}
			if ((var7 & 0x10) == 16) {
				var6.damage = arg0.g1();
				var6.damageType = arg0.g1();
				var6.combatCycle = loopCycle + 400;
				var6.health = arg0.g1();
				var6.totalHealth = arg0.g1();
			}
			if ((var7 & 0x20) == 32) {
				var6.type = NpcType.get(arg0.g2());
				var6.seqWalkId = var6.type.walkanim;
				var6.seqTurnAroundId = var6.type.walkanim_b;
				var6.seqTurnLeftId = var6.type.walkanim_r;
				var6.seqTurnRightId = var6.type.walkanim_l;
				var6.seqStandId = var6.type.readyanim;
			}
			if ((var7 & 0x40) == 64) {
				var6.spotanimId = arg0.g2();
				var8 = arg0.g4();
				var6.spotanimOffset = var8 >> 16;
				var6.spotanimLastCycle = loopCycle + (var8 & 0xFFFF);
				var6.spotanimFrame = 0;
				var6.spotanimCycle = 0;
				if (var6.spotanimLastCycle > loopCycle) {
					var6.spotanimFrame = -1;
				}
				if (var6.spotanimId == 65535) {
					var6.spotanimId = -1;
				}
			}
			if ((var7 & 0x80) == 128) {
				var6.targetTileX = arg0.g2();
				var6.targetTileZ = arg0.g2();
			}
		}
		if (arg2 <= 0) {
			this.field1121 = this.randomIn.takeNextValue();
		}
	}

	@ObfuscatedName("client.a(JB)V")
	public final void addIgnore(long arg0, byte arg1) {
		if (arg0 == 0L) {
			return;
		}
		if (this.ignoreCount >= 100) {
			this.addMessage(0, "Your ignore list is full. Max of 100 hit", (byte) 4, "");
			return;
		}
		String var4 = JString.formatName(0, JString.fromBase37(arg0, false));
		for (int var5 = 0; var5 < this.ignoreCount; var5++) {
			if (this.ignoreName37[var5] == arg0) {
				this.addMessage(0, var4 + " is already on your ignore list", (byte) 4, "");
				return;
			}
		}
		for (int var6 = 0; var6 < this.friendCount; var6++) {
			if (this.friendName37[var6] == arg0) {
				this.addMessage(0, "Please remove " + var4 + " from your friend list first", (byte) 4, "");
				return;
			}
		}
		this.ignoreName37[this.ignoreCount++] = arg0;
		this.redrawSidebar = true;
		if (arg1 == 3) {
			this.out.p1isaac((byte) -34, 79);
			this.out.p8(true, arg0);
		}
	}

	@ObfuscatedName("client.a(BLkb;I)V")
	public final void readZonePacket(byte arg0, Packet arg1, int arg2) {
		if (arg0 != -45) {
			this.load();
		}
		int var4;
		int var5;
		int var6;
		int var7;
		int var8;
		int var9;
		int var10;
		int var11;
		int var14;
		int var15;
		int var16;
		if (arg2 == 59 || arg2 == 76) {
			var4 = arg1.g1();
			var5 = this.baseX + (var4 >> 4 & 0x7);
			var6 = this.baseZ + (var4 & 0x7);
			var7 = arg1.g1();
			var8 = var7 >> 2;
			var9 = var7 & 0x3;
			var10 = this.LOC_SHAPE_TO_LAYER[var8];
			if (arg2 == 76) {
				var11 = -1;
			} else {
				var11 = arg1.g2();
			}
			if (var5 >= 0 && var6 >= 0 && var5 < 104 && var6 < 104) {
				LocAddEntity var12 = null;
				for (LocAddEntity var13 = (LocAddEntity) this.spawnedLocations.head(); var13 != null; var13 = (LocAddEntity) this.spawnedLocations.next(551)) {
					if (var13.plane == this.currentLevel && var13.x == var5 && var13.z == var6 && var13.layer == var10) {
						var12 = var13;
						break;
					}
				}
				if (var12 == null) {
					var14 = 0;
					var15 = -1;
					var16 = 0;
					int var17 = 0;
					if (var10 == 0) {
						var14 = this.scene.getWallBitset(this.currentLevel, var5, var6);
					}
					if (var10 == 1) {
						var14 = this.scene.getWallDecorationBitset(this.currentLevel, var6, 3, var5);
					}
					if (var10 == 2) {
						var14 = this.scene.getLocBitset(this.currentLevel, var5, var6);
					}
					if (var10 == 3) {
						var14 = this.scene.getGroundDecorationBitset(this.currentLevel, var5, var6);
					}
					if (var14 != 0) {
						int var18 = this.scene.getInfo(this.currentLevel, var5, var6, var14);
						var15 = var14 >> 14 & 0x7FFF;
						var16 = var18 & 0x1F;
						var17 = var18 >> 6;
					}
					var12 = new LocAddEntity();
					var12.plane = this.currentLevel;
					var12.layer = var10;
					var12.x = var5;
					var12.z = var6;
					var12.lastLocIndex = var15;
					var12.lastShape = var16;
					var12.lastAngle = var17;
					this.spawnedLocations.addTail(var12);
				}
				var12.locIndex = var11;
				var12.shape = var8;
				var12.angle = var9;
				this.addLoc(var9, var5, var6, var10, var11, var8, -27819, this.currentLevel);
			}
		} else if (arg2 == 42) {
			var4 = arg1.g1();
			var5 = this.baseX + (var4 >> 4 & 0x7);
			var6 = this.baseZ + (var4 & 0x7);
			var7 = arg1.g1();
			var8 = var7 >> 2;
			var9 = this.LOC_SHAPE_TO_LAYER[var8];
			var10 = arg1.g2();
			if (var5 >= 0 && var6 >= 0 && var5 < 104 && var6 < 104) {
				var11 = 0;
				if (var9 == 0) {
					var11 = this.scene.getWallBitset(this.currentLevel, var5, var6);
				}
				if (var9 == 1) {
					var11 = this.scene.getWallDecorationBitset(this.currentLevel, var6, 3, var5);
				}
				if (var9 == 2) {
					var11 = this.scene.getLocBitset(this.currentLevel, var5, var6);
				}
				if (var9 == 3) {
					var11 = this.scene.getGroundDecorationBitset(this.currentLevel, var5, var6);
				}
				if (var11 != 0) {
					LocEntity var38 = new LocEntity(false, var11 >> 14 & 0x7FFF, this.currentLevel, 0, var9, SeqType.instances[var10], var6, var5);
					this.locList.addTail(var38);
				}
			}
		} else {
			ObjStackEntity var32;
			if (arg2 == 223) {
				var4 = arg1.g1();
				var5 = this.baseX + (var4 >> 4 & 0x7);
				var6 = this.baseZ + (var4 & 0x7);
				var7 = arg1.g2();
				var8 = arg1.g2();
				if (var5 >= 0 && var6 >= 0 && var5 < 104 && var6 < 104) {
					var32 = new ObjStackEntity();
					var32.index = var7;
					var32.count = var8;
					if (this.levelObjStacks[this.currentLevel][var5][var6] == null) {
						this.levelObjStacks[this.currentLevel][var5][var6] = new LinkList(0);
					}
					this.levelObjStacks[this.currentLevel][var5][var6].addTail(var32);
					this.sortObjStacks(var5, var6);
				}
			} else if (arg2 == 49) {
				var4 = arg1.g1();
				var5 = this.baseX + (var4 >> 4 & 0x7);
				var6 = this.baseZ + (var4 & 0x7);
				var7 = arg1.g2();
				if (var5 >= 0 && var6 >= 0 && var5 < 104 && var6 < 104) {
					LinkList var30 = this.levelObjStacks[this.currentLevel][var5][var6];
					if (var30 != null) {
						for (var32 = (ObjStackEntity) var30.head(); var32 != null; var32 = (ObjStackEntity) var30.next(551)) {
							if (var32.index == (var7 & 0x7FFF)) {
								var32.unlink();
								break;
							}
						}
						if (var30.head() == null) {
							this.levelObjStacks[this.currentLevel][var5][var6] = null;
						}
						this.sortObjStacks(var5, var6);
					}
				}
			} else {
				int var36;
				int var37;
				if (arg2 == 69) {
					var4 = arg1.g1();
					var5 = this.baseX + (var4 >> 4 & 0x7);
					var6 = this.baseZ + (var4 & 0x7);
					var7 = var5 + arg1.g1b();
					var8 = var6 + arg1.g1b();
					var9 = arg1.g2b();
					var10 = arg1.g2();
					var11 = arg1.g1();
					var36 = arg1.g1();
					var37 = arg1.g2();
					var14 = arg1.g2();
					var15 = arg1.g1();
					var16 = arg1.g1();
					if (var5 >= 0 && var6 >= 0 && var5 < 104 && var6 < 104 && var7 >= 0 && var8 >= 0 && var7 < 104 && var8 < 104) {
						var5 = var5 * 128 + 64;
						var6 = var6 * 128 + 64;
						var7 = var7 * 128 + 64;
						var8 = var8 * 128 + 64;
						ProjectileEntity var43 = new ProjectileEntity(var36, var15, var6, var14 + loopCycle, this.currentLevel, var9, var37 + loopCycle, var16, 0, this.getHeightmapY(this.currentLevel, var5, (byte) 5, var6) - var11, var10, var5);
						var43.updateVelocity(this.getHeightmapY(this.currentLevel, var7, (byte) 5, var8) - var36, var8, var7, -855, var37 + loopCycle);
						this.projectiles.addTail(var43);
					}
				} else if (arg2 == 191) {
					var4 = arg1.g1();
					var5 = this.baseX + (var4 >> 4 & 0x7);
					var6 = this.baseZ + (var4 & 0x7);
					var7 = arg1.g2();
					var8 = arg1.g1();
					var9 = arg1.g2();
					if (var5 >= 0 && var6 >= 0 && var5 < 104 && var6 < 104) {
						var5 = var5 * 128 + 64;
						var6 = var6 * 128 + 64;
						SpotAnimEntity var34 = new SpotAnimEntity(var5, var7, false, var6, var9, this.getHeightmapY(this.currentLevel, var5, (byte) 5, var6) - var8, this.currentLevel, loopCycle);
						this.spotanims.addTail(var34);
					}
				} else if (arg2 == 50) {
					var4 = arg1.g1();
					var5 = this.baseX + (var4 >> 4 & 0x7);
					var6 = this.baseZ + (var4 & 0x7);
					var7 = arg1.g2();
					var8 = arg1.g2();
					var9 = arg1.g2();
					if (var5 >= 0 && var6 >= 0 && var5 < 104 && var6 < 104 && var9 != this.localPid) {
						ObjStackEntity var33 = new ObjStackEntity();
						var33.index = var7;
						var33.count = var8;
						if (this.levelObjStacks[this.currentLevel][var5][var6] == null) {
							this.levelObjStacks[this.currentLevel][var5][var6] = new LinkList(0);
						}
						this.levelObjStacks[this.currentLevel][var5][var6].addTail(var33);
						this.sortObjStacks(var5, var6);
					}
				} else {
					if (arg2 == 23) {
						var4 = arg1.g1();
						var5 = this.baseX + (var4 >> 4 & 0x7);
						var6 = this.baseZ + (var4 & 0x7);
						var7 = arg1.g1();
						var8 = var7 >> 2;
						var9 = var7 & 0x3;
						var10 = this.LOC_SHAPE_TO_LAYER[var8];
						var11 = arg1.g2();
						var36 = arg1.g2();
						var37 = arg1.g2();
						var14 = arg1.g2();
						byte var39 = arg1.g1b();
						byte var40 = arg1.g1b();
						byte var41 = arg1.g1b();
						byte var42 = arg1.g1b();
						PlayerEntity var19;
						if (var14 == this.localPid) {
							var19 = this.localPlayer;
						} else {
							var19 = this.players[var14];
						}
						if (var19 != null) {
							LocMergeEntity var20 = new LocMergeEntity(this.currentLevel, var9, var6, var36 + loopCycle, 2, var8, -1, var5, var10);
							this.mergedLocations.addTail(var20);
							LocMergeEntity var21 = new LocMergeEntity(this.currentLevel, var9, var6, var37 + loopCycle, 2, var8, var11, var5, var10);
							this.mergedLocations.addTail(var21);
							int var22 = this.levelHeightmap[this.currentLevel][var5][var6];
							int var23 = this.levelHeightmap[this.currentLevel][var5 + 1][var6];
							int var24 = this.levelHeightmap[this.currentLevel][var5 + 1][var6 + 1];
							int var25 = this.levelHeightmap[this.currentLevel][var5][var6 + 1];
							LocType var26 = LocType.get(var11);
							var19.locStartCycle = var36 + loopCycle;
							var19.locStopCycle = var37 + loopCycle;
							var19.locModel = var26.getModel(var8, var9, var22, var23, var24, var25, -1);
							int var27 = var26.width;
							int var28 = var26.length;
							if (var9 == 1 || var9 == 3) {
								var27 = var26.length;
								var28 = var26.width;
							}
							var19.locOffsetX = var5 * 128 + var27 * 64;
							var19.locOffsetZ = var6 * 128 + var28 * 64;
							var19.locOffsetY = this.getHeightmapY(this.currentLevel, var19.locOffsetX, (byte) 5, var19.locOffsetZ);
							byte var29;
							if (var39 > var41) {
								var29 = var39;
								var39 = var41;
								var41 = var29;
							}
							if (var40 > var42) {
								var29 = var40;
								var40 = var42;
								var42 = var29;
							}
							var19.minTileX = var5 + var39;
							var19.maxTileX = var5 + var41;
							var19.minTileZ = var6 + var40;
							var19.maxTileZ = var6 + var42;
						}
					}
					if (arg2 == 151) {
						var4 = arg1.g1();
						var5 = this.baseX + (var4 >> 4 & 0x7);
						var6 = this.baseZ + (var4 & 0x7);
						var7 = arg1.g2();
						var8 = arg1.g2();
						var9 = arg1.g2();
						if (var5 >= 0 && var6 >= 0 && var5 < 104 && var6 < 104) {
							LinkList var31 = this.levelObjStacks[this.currentLevel][var5][var6];
							if (var31 != null) {
								for (ObjStackEntity var35 = (ObjStackEntity) var31.head(); var35 != null; var35 = (ObjStackEntity) var31.next(551)) {
									if (var35.index == (var7 & 0x7FFF) && var35.count == var8) {
										var35.count = var9;
										break;
									}
								}
								this.sortObjStacks(var5, var6);
							}
						}
					}
				}
			}
		}
	}

	@ObfuscatedName("client.d(B)I")
	public final int getTopLevel(byte arg0) {
		if (arg0 != 106) {
			this.packetType = this.in.g1();
		}
		int var2 = 3;
		if (this.cameraPitch < 310) {
			int var3 = this.cameraX >> 7;
			int var4 = this.cameraZ >> 7;
			int var5 = this.localPlayer.x >> 7;
			int var6 = this.localPlayer.z >> 7;
			if ((this.levelTileFlags[this.currentLevel][var3][var4] & 0x4) != 0) {
				var2 = this.currentLevel;
			}
			int var7;
			if (var5 > var3) {
				var7 = var5 - var3;
			} else {
				var7 = var3 - var5;
			}
			int var8;
			if (var6 > var4) {
				var8 = var6 - var4;
			} else {
				var8 = var4 - var6;
			}
			int var9;
			int var10;
			if (var7 > var8) {
				var9 = var8 * 65536 / var7;
				var10 = 32768;
				while (var3 != var5) {
					if (var3 < var5) {
						var3++;
					} else if (var3 > var5) {
						var3--;
					}
					if ((this.levelTileFlags[this.currentLevel][var3][var4] & 0x4) != 0) {
						var2 = this.currentLevel;
					}
					var10 += var9;
					if (var10 >= 65536) {
						var10 -= 65536;
						if (var4 < var6) {
							var4++;
						} else if (var4 > var6) {
							var4--;
						}
						if ((this.levelTileFlags[this.currentLevel][var3][var4] & 0x4) != 0) {
							var2 = this.currentLevel;
						}
					}
				}
			} else {
				var9 = var7 * 65536 / var8;
				var10 = 32768;
				while (var4 != var6) {
					if (var4 < var6) {
						var4++;
					} else if (var4 > var6) {
						var4--;
					}
					if ((this.levelTileFlags[this.currentLevel][var3][var4] & 0x4) != 0) {
						var2 = this.currentLevel;
					}
					var10 += var9;
					if (var10 >= 65536) {
						var10 -= 65536;
						if (var3 < var5) {
							var3++;
						} else if (var3 > var5) {
							var3--;
						}
						if ((this.levelTileFlags[this.currentLevel][var3][var4] & 0x4) != 0) {
							var2 = this.currentLevel;
						}
					}
				}
			}
		}
		if ((this.levelTileFlags[this.currentLevel][this.localPlayer.x >> 7][this.localPlayer.z >> 7] & 0x4) != 0) {
			var2 = this.currentLevel;
		}
		return var2;
	}

	@ObfuscatedName("client.h(I)I")
	public final int getTopLevelCutscene(int arg0) {
		int var2 = this.getHeightmapY(this.currentLevel, this.cameraX, (byte) 5, this.cameraZ);
		this.packetSize += arg0;
		return var2 - this.cameraY >= 800 || (this.levelTileFlags[this.currentLevel][this.cameraX >> 7][this.cameraZ >> 7] & 0x4) == 0 ? 3 : this.currentLevel;
	}

	@ObfuscatedName("client.i(I)V")
	public final void drawScene(int arg0) {
		this.sceneCycle++;
		this.pushPlayers(284);
		this.pushNpcs(false);
		this.packetSize += arg0;
		this.pushProjectiles((byte) -26);
		this.pushSpotanims((byte) 106);
		this.pushLocs(254);
		int var2;
		int var3;
		int var4;
		if (!this.cutscene) {
			var2 = this.orbitCameraPitch;
			if (this.cameraPitchClamp / 256 > var2) {
				var2 = this.cameraPitchClamp / 256;
			}
			if (this.cameraModifierEnabled[4] && this.cameraModifierWobbleScale[4] + 128 > var2) {
				var2 = this.cameraModifierWobbleScale[4] + 128;
			}
			var3 = this.orbitCameraYaw + this.cameraAnticheatAngle & 0x7FF;
			this.orbitCamera(this.getHeightmapY(this.currentLevel, this.localPlayer.x, (byte) 5, this.localPlayer.z) - 50, this.orbitCameraX, var3, var2, 16418, this.orbitCameraZ, var2 * 3 + 600);
			cyclelogic2++;
			if (cyclelogic2 > 1802) {
				cyclelogic2 = 0;
				this.out.p1isaac((byte) -34, 146);
				this.out.p1(0);
				var4 = this.out.pos;
				this.out.p2(29711);
				this.out.p1(70);
				this.out.p1((int) (Math.random() * 256.0D));
				this.out.p1(242);
				this.out.p1(186);
				this.out.p1(39);
				this.out.p1(61);
				if ((int) (Math.random() * 2.0D) == 0) {
					this.out.p1(13);
				}
				if ((int) (Math.random() * 2.0D) == 0) {
					this.out.p2(57856);
				}
				this.out.p2((int) (Math.random() * 65536.0D));
				this.out.psize1(0, this.out.pos - var4);
			}
		}
		if (this.cutscene) {
			var2 = this.getTopLevelCutscene(0);
		} else {
			var2 = this.getTopLevel((byte) 106);
		}
		var3 = this.cameraX;
		var4 = this.cameraY;
		int var5 = this.cameraZ;
		int var6 = this.cameraPitch;
		int var7 = this.cameraYaw;
		int var9;
		for (int var8 = 0; var8 < 5; var8++) {
			if (this.cameraModifierEnabled[var8]) {
				var9 = (int) (Math.random() * (double) (this.cameraModifierJitter[var8] * 2 + 1) - (double) this.cameraModifierJitter[var8] + Math.sin((double) this.cameraModifierCycle[var8] * ((double) this.cameraModifierWobbleSpeed[var8] / 100.0D)) * (double) this.cameraModifierWobbleScale[var8]);
				if (var8 == 0) {
					this.cameraX += var9;
				}
				if (var8 == 1) {
					this.cameraY += var9;
				}
				if (var8 == 2) {
					this.cameraZ += var9;
				}
				if (var8 == 3) {
					this.cameraYaw = this.cameraYaw + var9 & 0x7FF;
				}
				if (var8 == 4) {
					this.cameraPitch += var9;
					if (this.cameraPitch < 128) {
						this.cameraPitch = 128;
					}
					if (this.cameraPitch > 383) {
						this.cameraPitch = 383;
					}
				}
			}
		}
		var9 = Pix3D.cycle;
		Model.checkHover = true;
		Model.pickedCount = 0;
		Model.mouseX = super.mouseX - 8;
		Model.mouseZ = super.mouseY - 11;
		Pix2D.clear(this.field1508);
		this.scene.draw(this.cameraYaw, this.cameraX, var2, this.cameraPitch, this.cameraY, this.cameraZ, 0);
		this.scene.clearTemporaryLocs(0);
		this.draw2DEntityElements(this.field1170);
		this.drawTileHint((byte) -11);
		this.updateTextures(var9, true);
		this.draw3DEntityElements(9);
		this.areaViewport.draw(11, super.graphics, 8, 5193);
		this.cameraX = var3;
		this.cameraY = var4;
		this.cameraZ = var5;
		this.cameraPitch = var6;
		this.cameraYaw = var7;
	}

	@ObfuscatedName("client.c(Z)V")
	public final void runMidi(boolean arg0) {
		this.startMidiThread = false;
		if (!arg0) {
			return;
		}
		while (this.midiThreadActive) {
			try {
				Thread.sleep(50L);
			} catch (Exception var12) {
			}
			Object var5 = this.midiSync;
			String var2;
			int var3;
			int var4;
			synchronized (this.midiSync) {
				var2 = this.midiSyncName;
				var3 = this.midiSyncCrc;
				var4 = this.midiSyncLen;
				this.midiSyncName = null;
				this.midiSyncCrc = 0;
				this.midiSyncLen = 0;
			}
			if (var2 != null) {
				byte[] var14 = signlink.cacheload(var2 + ".mid");
				int var6;
				if (var14 != null && var3 != 12345678) {
					this.crc32.reset();
					this.crc32.update(var14);
					var6 = (int) this.crc32.getValue();
					if (var6 != var3) {
						var14 = null;
					}
				}
				if (var14 == null) {
					try {
						DataInputStream var15 = this.openUrl(var2 + "_" + var3 + ".mid");
						var14 = new byte[var4];
						int var8;
						for (int var7 = 0; var7 < var4; var7 += var8) {
							var8 = var15.read(var14, var7, var4 - var7);
							if (var8 == -1) {
								byte[] var9 = new byte[var7];
								for (int var10 = 0; var10 < var7; var10++) {
									var9[var10] = var14[var10];
								}
								var14 = var9;
								var4 = var7;
								break;
							}
						}
						var15.close();
						signlink.cachesave(var2 + ".mid", var14);
					} catch (Exception var13) {
					}
				}
				if (var14 == null) {
					return;
				}
				var6 = (new Packet(363, var14)).g4();
				byte[] var16 = new byte[var6];
				BZip2.read(var16, var6, var14, var4, 4);
				this.saveMidi(var16, 625, var6, true);
			}
		}
	}

	@ObfuscatedName("client.d(Z)V")
	public static final void setLowMemory(boolean arg0) {
		if (!arg0) {
			field1235 = !field1235;
		}
		World3D.lowMemory = true;
		Pix3D.lowDetail = true;
		lowMemory = true;
		World.lowMemory = true;
	}

	@ObfuscatedName("client.e(Z)V")
	public final void drawFlames(boolean arg0) {
		short var2 = 256;
		int var3;
		if (this.flameGradientCycle0 > 0) {
			for (var3 = 0; var3 < 256; var3++) {
				if (this.flameGradientCycle0 > 768) {
					this.flameGradient[var3] = this.mix(this.flameGradient0[var3], 1024 - this.flameGradientCycle0, this.flameGradient1[var3], 997);
				} else if (this.flameGradientCycle0 > 256) {
					this.flameGradient[var3] = this.flameGradient1[var3];
				} else {
					this.flameGradient[var3] = this.mix(this.flameGradient1[var3], 256 - this.flameGradientCycle0, this.flameGradient0[var3], 997);
				}
			}
		} else if (this.flameGradientCycle1 > 0) {
			for (var3 = 0; var3 < 256; var3++) {
				if (this.flameGradientCycle1 > 768) {
					this.flameGradient[var3] = this.mix(this.flameGradient0[var3], 1024 - this.flameGradientCycle1, this.flameGradient2[var3], 997);
				} else if (this.flameGradientCycle1 > 256) {
					this.flameGradient[var3] = this.flameGradient2[var3];
				} else {
					this.flameGradient[var3] = this.mix(this.flameGradient2[var3], 256 - this.flameGradientCycle1, this.flameGradient0[var3], 997);
				}
			}
		} else {
			for (var3 = 0; var3 < 256; var3++) {
				this.flameGradient[var3] = this.flameGradient0[var3];
			}
		}
		for (var3 = 0; var3 < 33920; var3++) {
			this.imageTitle0.pixels[var3] = this.imageFlamesLeft.pixels[var3];
		}
		int var4 = 0;
		int var5 = 1152;
		int var7;
		int var8;
		int var9;
		int var10;
		int var11;
		int var12;
		int var13;
		for (int var6 = 1; var6 < var2 - 1; var6++) {
			var7 = this.flameLineOffset[var6] * (var2 - var6) / var2;
			var8 = var7 + 22;
			if (var8 < 0) {
				var8 = 0;
			}
			var4 += var8;
			for (var9 = var8; var9 < 128; var9++) {
				var10 = this.flameBuffer3[var4++];
				if (var10 == 0) {
					var5++;
				} else {
					var11 = var10;
					var12 = 256 - var10;
					var10 = this.flameGradient[var10];
					var13 = this.imageTitle0.pixels[var5];
					this.imageTitle0.pixels[var5++] = ((var10 & 0xFF00FF) * var11 + (var13 & 0xFF00FF) * var12 & 0xFF00FF00) + ((var10 & 0xFF00) * var11 + (var13 & 0xFF00) * var12 & 0xFF0000) >> 8;
				}
			}
			var5 += var8;
		}
		this.imageTitle0.draw(0, super.graphics, 0, 5193);
		for (var7 = 0; var7 < 33920; var7++) {
			this.imageTitle1.pixels[var7] = this.imageFlamesRight.pixels[var7];
		}
		var4 = 0;
		var5 = 1176;
		for (var8 = 1; var8 < var2 - 1; var8++) {
			var9 = this.flameLineOffset[var8] * (var2 - var8) / var2;
			var10 = 103 - var9;
			var5 += var9;
			for (var11 = 0; var11 < var10; var11++) {
				var12 = this.flameBuffer3[var4++];
				if (var12 == 0) {
					var5++;
				} else {
					var13 = var12;
					int var14 = 256 - var12;
					var12 = this.flameGradient[var12];
					int var15 = this.imageTitle1.pixels[var5];
					this.imageTitle1.pixels[var5++] = ((var12 & 0xFF00FF) * var13 + (var15 & 0xFF00FF) * var14 & 0xFF00FF00) + ((var12 & 0xFF00) * var13 + (var15 & 0xFF00) * var14 & 0xFF0000) >> 8;
				}
			}
			var4 += 128 - var10;
			var5 += 128 - var10 - var9;
		}
		if (!arg0) {
			field1323 = 281;
		}
		this.imageTitle1.draw(0, super.graphics, 661, 5193);
	}

	@ObfuscatedName("client.a(IIILhc;III)V")
	public final void handleInterfaceInput(int arg0, int arg1, int arg2, Component arg3, int arg4, int arg5, int arg6) {
		if (arg3.type != 0 || arg3.childId == null || arg3.hide || (arg1 < arg5 || arg0 < arg2 || arg1 > arg5 + arg3.width || arg0 > arg2 + arg3.height)) {
			return;
		}
		int var8 = arg3.childId.length;
		if (arg4 != 5082) {
			this.packetType = this.in.g1();
		}
		for (int var9 = 0; var9 < var8; var9++) {
			int var10 = arg3.childX[var9] + arg5;
			int var11 = arg3.childY[var9] + arg2 - arg6;
			Component var12 = Component.instances[arg3.childId[var9]];
			int var20 = var10 + var12.x;
			int var21 = var11 + var12.y;
			if ((var12.overLayer >= 0 || var12.overColour != 0) && arg1 >= var20 && arg0 >= var21 && arg1 < var20 + var12.width && arg0 < var21 + var12.height) {
				if (var12.overLayer >= 0) {
					this.lastHoveredInterfaceId = var12.overLayer;
				} else {
					this.lastHoveredInterfaceId = var12.id;
				}
			}
			if (var12.type == 0) {
				this.handleInterfaceInput(arg0, arg1, var21, var12, 5082, var20, var12.scrollPosition);
				if (var12.scroll > var12.height) {
					this.handleScrollInput(arg1, 0, arg0, var12.scroll, var12.height, true, var20 + var12.width, var21, var12);
				}
			} else {
				if (var12.buttonType == 1 && arg1 >= var20 && arg0 >= var21 && arg1 < var20 + var12.width && arg0 < var21 + var12.height) {
					boolean var13 = false;
					if (var12.clientCode != 0) {
						var13 = this.handleSocialMenuOption(var12, (byte) 8);
					}
					if (!var13) {
						this.menuOption[this.menuSize] = var12.option;
						this.menuAction[this.menuSize] = 951;
						this.menuParamC[this.menuSize] = var12.id;
						this.menuSize++;
					}
				}
				if (var12.buttonType == 2 && this.spellSelected == 0 && arg1 >= var20 && arg0 >= var21 && arg1 < var20 + var12.width && arg0 < var21 + var12.height) {
					String var22 = var12.actionVerb;
					if (var22.indexOf(" ") != -1) {
						var22 = var22.substring(0, var22.indexOf(" "));
					}
					this.menuOption[this.menuSize] = var22 + " @gre@" + var12.action;
					this.menuAction[this.menuSize] = 930;
					this.menuParamC[this.menuSize] = var12.id;
					this.menuSize++;
				}
				if (var12.buttonType == 3 && arg1 >= var20 && arg0 >= var21 && arg1 < var20 + var12.width && arg0 < var21 + var12.height) {
					this.menuOption[this.menuSize] = "Close";
					this.menuAction[this.menuSize] = 947;
					this.menuParamC[this.menuSize] = var12.id;
					this.menuSize++;
				}
				if (var12.buttonType == 4 && arg1 >= var20 && arg0 >= var21 && arg1 < var20 + var12.width && arg0 < var21 + var12.height) {
					this.menuOption[this.menuSize] = var12.option;
					this.menuAction[this.menuSize] = 465;
					this.menuParamC[this.menuSize] = var12.id;
					this.menuSize++;
				}
				if (var12.buttonType == 5 && arg1 >= var20 && arg0 >= var21 && arg1 < var20 + var12.width && arg0 < var21 + var12.height) {
					this.menuOption[this.menuSize] = var12.option;
					this.menuAction[this.menuSize] = 960;
					this.menuParamC[this.menuSize] = var12.id;
					this.menuSize++;
				}
				if (var12.buttonType == 6 && !this.pressedContinueOption && arg1 >= var20 && arg0 >= var21 && arg1 < var20 + var12.width && arg0 < var21 + var12.height) {
					this.menuOption[this.menuSize] = var12.option;
					this.menuAction[this.menuSize] = 44;
					this.menuParamC[this.menuSize] = var12.id;
					this.menuSize++;
				}
				if (var12.type == 2) {
					int var23 = 0;
					for (int var14 = 0; var14 < var12.height; var14++) {
						for (int var15 = 0; var15 < var12.width; var15++) {
							int var16 = var20 + var15 * (var12.marginX + 32);
							int var17 = var21 + var14 * (var12.marginY + 32);
							if (var23 < 20) {
								var16 += var12.invSlotOffsetX[var23];
								var17 += var12.invSlotOffsetY[var23];
							}
							if (arg1 >= var16 && arg0 >= var17 && arg1 < var16 + 32 && arg0 < var17 + 32) {
								this.hoveredSlot = var23;
								this.hoveredSlotParentId = var12.id;
								if (var12.invSlotObjId[var23] > 0) {
									ObjType var18 = ObjType.get(var12.invSlotObjId[var23] - 1);
									if (this.objSelected == 1 && var12.interactable) {
										if (var12.id != this.objSelectedInterface || var23 != this.objSelectedSlot) {
											this.menuOption[this.menuSize] = "Use " + this.objSelectedName + " with @lre@" + var18.name;
											this.menuAction[this.menuSize] = 881;
											this.menuParamA[this.menuSize] = var18.index;
											this.menuParamB[this.menuSize] = var23;
											this.menuParamC[this.menuSize] = var12.id;
											this.menuSize++;
										}
									} else if (this.spellSelected != 1 || !var12.interactable) {
										int var19;
										if (var12.interactable) {
											for (var19 = 4; var19 >= 3; var19--) {
												if (var18.iop != null && var18.iop[var19] != null) {
													this.menuOption[this.menuSize] = var18.iop[var19] + " @lre@" + var18.name;
													if (var19 == 3) {
														this.menuAction[this.menuSize] = 478;
													}
													if (var19 == 4) {
														this.menuAction[this.menuSize] = 347;
													}
													this.menuParamA[this.menuSize] = var18.index;
													this.menuParamB[this.menuSize] = var23;
													this.menuParamC[this.menuSize] = var12.id;
													this.menuSize++;
												} else if (var19 == 4) {
													this.menuOption[this.menuSize] = "Drop @lre@" + var18.name;
													this.menuAction[this.menuSize] = 347;
													this.menuParamA[this.menuSize] = var18.index;
													this.menuParamB[this.menuSize] = var23;
													this.menuParamC[this.menuSize] = var12.id;
													this.menuSize++;
												}
											}
										}
										if (var12.usable) {
											this.menuOption[this.menuSize] = "Use @lre@" + var18.name;
											this.menuAction[this.menuSize] = 188;
											this.menuParamA[this.menuSize] = var18.index;
											this.menuParamB[this.menuSize] = var23;
											this.menuParamC[this.menuSize] = var12.id;
											this.menuSize++;
										}
										if (var12.interactable && var18.iop != null) {
											for (var19 = 2; var19 >= 0; var19--) {
												if (var18.iop[var19] != null) {
													this.menuOption[this.menuSize] = var18.iop[var19] + " @lre@" + var18.name;
													if (var19 == 0) {
														this.menuAction[this.menuSize] = 405;
													}
													if (var19 == 1) {
														this.menuAction[this.menuSize] = 38;
													}
													if (var19 == 2) {
														this.menuAction[this.menuSize] = 422;
													}
													this.menuParamA[this.menuSize] = var18.index;
													this.menuParamB[this.menuSize] = var23;
													this.menuParamC[this.menuSize] = var12.id;
													this.menuSize++;
												}
											}
										}
										if (var12.iops != null) {
											for (var19 = 4; var19 >= 0; var19--) {
												if (var12.iops[var19] != null) {
													this.menuOption[this.menuSize] = var12.iops[var19] + " @lre@" + var18.name;
													if (var19 == 0) {
														this.menuAction[this.menuSize] = 602;
													}
													if (var19 == 1) {
														this.menuAction[this.menuSize] = 596;
													}
													if (var19 == 2) {
														this.menuAction[this.menuSize] = 22;
													}
													if (var19 == 3) {
														this.menuAction[this.menuSize] = 892;
													}
													if (var19 == 4) {
														this.menuAction[this.menuSize] = 415;
													}
													this.menuParamA[this.menuSize] = var18.index;
													this.menuParamB[this.menuSize] = var23;
													this.menuParamC[this.menuSize] = var12.id;
													this.menuSize++;
												}
											}
										}
										this.menuOption[this.menuSize] = "Examine @lre@" + var18.name;
										this.menuAction[this.menuSize] = 1773;
										this.menuParamA[this.menuSize] = var18.index;
										this.menuParamC[this.menuSize] = var12.invSlotObjCount[var23];
										this.menuSize++;
									} else if ((this.activeSpellFlags & 0x10) == 16) {
										this.menuOption[this.menuSize] = this.spellCaption + " @lre@" + var18.name;
										this.menuAction[this.menuSize] = 391;
										this.menuParamA[this.menuSize] = var18.index;
										this.menuParamB[this.menuSize] = var23;
										this.menuParamC[this.menuSize] = var12.id;
										this.menuSize++;
									}
								}
							}
							var23++;
						}
					}
				}
			}
		}
	}

	@ObfuscatedName("client.j(I)V")
	public final void handleChatSettingsInput(int arg0) {
		this.packetSize += arg0;
		if (super.mouseClickButton != 1) {
			return;
		}
		if (super.mouseClickX >= 8 && super.mouseClickX <= 108 && super.mouseClickY >= 490 && super.mouseClickY <= 522) {
			this.publicChatSetting = (this.publicChatSetting + 1) % 4;
			this.redrawPrivacySettings = true;
			this.redrawChatback = true;
			this.out.p1isaac((byte) -34, 244);
			this.out.p1(this.publicChatSetting);
			this.out.p1(this.privateChatSetting);
			this.out.p1(this.tradeChatSetting);
		}
		if (super.mouseClickX >= 137 && super.mouseClickX <= 237 && super.mouseClickY >= 490 && super.mouseClickY <= 522) {
			this.privateChatSetting = (this.privateChatSetting + 1) % 3;
			this.redrawPrivacySettings = true;
			this.redrawChatback = true;
			this.out.p1isaac((byte) -34, 244);
			this.out.p1(this.publicChatSetting);
			this.out.p1(this.privateChatSetting);
			this.out.p1(this.tradeChatSetting);
		}
		if (super.mouseClickX >= 275 && super.mouseClickX <= 375 && super.mouseClickY >= 490 && super.mouseClickY <= 522) {
			this.tradeChatSetting = (this.tradeChatSetting + 1) % 3;
			this.redrawPrivacySettings = true;
			this.redrawChatback = true;
			this.out.p1isaac((byte) -34, 244);
			this.out.p1(this.publicChatSetting);
			this.out.p1(this.privateChatSetting);
			this.out.p1(this.tradeChatSetting);
		}
		if (super.mouseClickX < 416 || super.mouseClickX > 516 || super.mouseClickY < 490 || super.mouseClickY > 522) {
			return;
		}
		this.closeInterfaces((byte) -60);
		this.reportAbuseInput = "";
		this.reportAbuseMuteOption = false;
		for (int var2 = 0; var2 < Component.instances.length; var2++) {
			if (Component.instances[var2] != null && Component.instances[var2].clientCode == 600) {
				this.reportAbuseInterfaceID = this.viewportInterfaceId = Component.instances[var2].layer;
				return;
			}
		}
		return;
	}

	@ObfuscatedName("client.b(III)V")
	public final void handleChatMouseInput(int arg0, int arg1, int arg2) {
		int var4 = 0;
		for (int var5 = 0; var5 < 100; var5++) {
			if (this.messageText[var5] != null) {
				int var6 = this.messageType[var5];
				int var7 = 70 - var4 * 14 + this.chatScrollOffset + 4;
				if (var7 < -20) {
					break;
				}
				if (var6 == 0) {
					var4++;
				}
				if ((var6 == 1 || var6 == 2) && (var6 == 1 || this.publicChatSetting == 0 || this.publicChatSetting == 1 && this.isFriend(-20, this.messageSender[var5]))) {
					if (arg0 > var7 - 14 && arg0 <= var7 && !this.messageSender[var5].equals(this.localPlayer.name)) {
						if (this.rights) {
							this.menuOption[this.menuSize] = "Report abuse @whi@" + this.messageSender[var5];
							this.menuAction[this.menuSize] = 34;
							this.menuSize++;
						}
						this.menuOption[this.menuSize] = "Add ignore @whi@" + this.messageSender[var5];
						this.menuAction[this.menuSize] = 436;
						this.menuSize++;
						this.menuOption[this.menuSize] = "Add friend @whi@" + this.messageSender[var5];
						this.menuAction[this.menuSize] = 406;
						this.menuSize++;
					}
					var4++;
				}
				if ((var6 == 3 || var6 == 7) && this.splitPrivateChat == 0 && (var6 == 7 || this.privateChatSetting == 0 || this.privateChatSetting == 1 && this.isFriend(-20, this.messageSender[var5]))) {
					if (arg0 > var7 - 14 && arg0 <= var7) {
						if (this.rights) {
							this.menuOption[this.menuSize] = "Report abuse @whi@" + this.messageSender[var5];
							this.menuAction[this.menuSize] = 34;
							this.menuSize++;
						}
						this.menuOption[this.menuSize] = "Add ignore @whi@" + this.messageSender[var5];
						this.menuAction[this.menuSize] = 436;
						this.menuSize++;
						this.menuOption[this.menuSize] = "Add friend @whi@" + this.messageSender[var5];
						this.menuAction[this.menuSize] = 406;
						this.menuSize++;
					}
					var4++;
				}
				if (var6 == 4 && (this.tradeChatSetting == 0 || this.tradeChatSetting == 1 && this.isFriend(-20, this.messageSender[var5]))) {
					if (arg0 > var7 - 14 && arg0 <= var7) {
						this.menuOption[this.menuSize] = "Accept trade @whi@" + this.messageSender[var5];
						this.menuAction[this.menuSize] = 903;
						this.menuSize++;
					}
					var4++;
				}
				if ((var6 == 5 || var6 == 6) && this.splitPrivateChat == 0 && this.privateChatSetting < 2) {
					var4++;
				}
				if (var6 == 8 && (this.tradeChatSetting == 0 || this.tradeChatSetting == 1 && this.isFriend(-20, this.messageSender[var5]))) {
					if (arg0 > var7 - 14 && arg0 <= var7) {
						this.menuOption[this.menuSize] = "Accept duel @whi@" + this.messageSender[var5];
						this.menuAction[this.menuSize] = 363;
						this.menuSize++;
					}
					var4++;
				}
			}
		}
		this.packetSize += arg1;
	}

	@ObfuscatedName("client.k(I)V")
	public final void pushPlayers(int arg0) {
		if (this.localPlayer.x >> 7 == this.flagSceneTileX && this.localPlayer.z >> 7 == this.flagSceneTileZ) {
			this.flagSceneTileX = 0;
		}
		for (int var2 = -1; var2 < this.playerCount; var2++) {
			PlayerEntity var3;
			int var4;
			if (var2 == -1) {
				var3 = this.localPlayer;
				var4 = this.LOCAL_PLAYER_INDEX << 14;
			} else {
				var3 = this.players[this.playerIds[var2]];
				var4 = this.playerIds[var2] << 14;
			}
			if (var3 != null && var3.isVisible(false)) {
				var3.lowMemory = false;
				if ((lowMemory && this.playerCount > 50 || this.playerCount > 200) && var2 != -1 && var3.secondarySeqId == var3.seqStandId) {
					var3.lowMemory = true;
				}
				int var5 = var3.x >> 7;
				int var6 = var3.z >> 7;
				if (var5 >= 0 && var5 < 104 && var6 >= 0 && var6 < 104) {
					if (var3.locModel == null || loopCycle < var3.locStartCycle || loopCycle >= var3.locStopCycle) {
						if ((var3.x & 0x7F) == 64 && (var3.z & 0x7F) == 64) {
							if (this.tileLastOccupiedCycle[var5][var6] == this.sceneCycle) {
								continue;
							}
							this.tileLastOccupiedCycle[var5][var6] = this.sceneCycle;
						}
						var3.y = this.getHeightmapY(this.currentLevel, var3.x, (byte) 5, var3.z);
						this.scene.addTemporary(-44713, var3.z, 60, var3.yaw, var3.x, var4, var3.seqStretches, null, var3, var3.y, this.currentLevel);
					} else {
						var3.lowMemory = false;
						var3.y = this.getHeightmapY(this.currentLevel, var3.x, (byte) 5, var3.z);
						this.scene.addTemporary(var3.maxTileX, 60, null, var3.z, var3.y, var4, var3.yaw, var3.minTileZ, var3.minTileX, var3, false, this.currentLevel, var3.maxTileZ, var3.x);
					}
				}
			}
		}
		if (arg0 <= 0) {
			this.out.p1(210);
		}
	}

	@ObfuscatedName("client.a(IIBI)I")
	public final int getHeightmapY(int arg0, int arg1, byte arg2, int arg3) {
		if (arg2 != 5) {
			this.packetType = this.in.g1();
		}
		int var5 = arg1 >> 7;
		int var6 = arg3 >> 7;
		int var7 = arg0;
		if (arg0 < 3 && (this.levelTileFlags[1][var5][var6] & 0x2) == 2) {
			var7 = arg0 + 1;
		}
		int var8 = arg1 & 0x7F;
		int var9 = arg3 & 0x7F;
		int var10 = this.levelHeightmap[var7][var5][var6] * (128 - var8) + this.levelHeightmap[var7][var5 + 1][var6] * var8 >> 7;
		int var11 = this.levelHeightmap[var7][var5][var6 + 1] * (128 - var8) + this.levelHeightmap[var7][var5 + 1][var6 + 1] * var8 >> 7;
		return var10 * (128 - var9) + var11 * var9 >> 7;
	}

	@ObfuscatedName("client.a(Lbc;IIII)V")
	public final void addNpcOptions(NpcType arg0, int arg1, int arg2, int arg3, int arg4) {
		while (arg1 >= 0) {
			this.packetType = -1;
		}
		if (this.menuSize >= 400) {
			return;
		}
		String var6 = arg0.name;
		if (arg0.vislevel != 0) {
			var6 = var6 + getCombatLevelColorTag(this.localPlayer.combatLevel, false, arg0.vislevel) + " (level-" + arg0.vislevel + ")";
		}
		if (this.objSelected == 1) {
			this.menuOption[this.menuSize] = "Use " + this.objSelectedName + " with @yel@" + var6;
			this.menuAction[this.menuSize] = 900;
			this.menuParamA[this.menuSize] = arg4;
			this.menuParamB[this.menuSize] = arg3;
			this.menuParamC[this.menuSize] = arg2;
			this.menuSize++;
		} else if (this.spellSelected != 1) {
			int var7;
			if (arg0.op != null) {
				for (var7 = 4; var7 >= 0; var7--) {
					if (arg0.op[var7] != null && !arg0.op[var7].equalsIgnoreCase("attack")) {
						this.menuOption[this.menuSize] = arg0.op[var7] + " @yel@" + var6;
						if (var7 == 0) {
							this.menuAction[this.menuSize] = 728;
						}
						if (var7 == 1) {
							this.menuAction[this.menuSize] = 542;
						}
						if (var7 == 2) {
							this.menuAction[this.menuSize] = 6;
						}
						if (var7 == 3) {
							this.menuAction[this.menuSize] = 963;
						}
						if (var7 == 4) {
							this.menuAction[this.menuSize] = 245;
						}
						this.menuParamA[this.menuSize] = arg4;
						this.menuParamB[this.menuSize] = arg3;
						this.menuParamC[this.menuSize] = arg2;
						this.menuSize++;
					}
				}
			}
			if (arg0.op != null) {
				for (var7 = 4; var7 >= 0; var7--) {
					if (arg0.op[var7] != null && arg0.op[var7].equalsIgnoreCase("attack")) {
						short var8 = 0;
						if (arg0.vislevel > this.localPlayer.combatLevel) {
							var8 = 2000;
						}
						this.menuOption[this.menuSize] = arg0.op[var7] + " @yel@" + var6;
						if (var7 == 0) {
							this.menuAction[this.menuSize] = var8 + 728;
						}
						if (var7 == 1) {
							this.menuAction[this.menuSize] = var8 + 542;
						}
						if (var7 == 2) {
							this.menuAction[this.menuSize] = var8 + 6;
						}
						if (var7 == 3) {
							this.menuAction[this.menuSize] = var8 + 963;
						}
						if (var7 == 4) {
							this.menuAction[this.menuSize] = var8 + 245;
						}
						this.menuParamA[this.menuSize] = arg4;
						this.menuParamB[this.menuSize] = arg3;
						this.menuParamC[this.menuSize] = arg2;
						this.menuSize++;
					}
				}
			}
			this.menuOption[this.menuSize] = "Examine @yel@" + var6;
			this.menuAction[this.menuSize] = 1607;
			this.menuParamA[this.menuSize] = arg4;
			this.menuParamB[this.menuSize] = arg3;
			this.menuParamC[this.menuSize] = arg2;
			this.menuSize++;
		} else if ((this.activeSpellFlags & 0x2) == 2) {
			this.menuOption[this.menuSize] = this.spellCaption + " @yel@" + var6;
			this.menuAction[this.menuSize] = 265;
			this.menuParamA[this.menuSize] = arg4;
			this.menuParamB[this.menuSize] = arg3;
			this.menuParamC[this.menuSize] = arg2;
			this.menuSize++;
			return;
		}
	}

	@ObfuscatedName("client.l(I)V")
	public final void handleInputKey(int arg0) {
		if (arg0 != 7) {
			this.field1497 = this.randomIn.takeNextValue();
		}
		while (true) {
			int var2;
			do {
				while (true) {
					var2 = this.pollKey(false);
					if (var2 == -1) {
						return;
					}
					if (this.viewportInterfaceId != -1 && this.viewportInterfaceId == this.reportAbuseInterfaceID) {
						if (var2 == 8 && this.reportAbuseInput.length() > 0) {
							this.reportAbuseInput = this.reportAbuseInput.substring(0, this.reportAbuseInput.length() - 1);
						}
						break;
					}
					int var7;
					if (this.showSocialInput) {
						if (var2 >= 32 && var2 <= 122 && this.socialInput.length() < 80) {
							this.socialInput = this.socialInput + (char) var2;
							this.redrawChatback = true;
						}
						if (var2 == 8 && this.socialInput.length() > 0) {
							this.socialInput = this.socialInput.substring(0, this.socialInput.length() - 1);
							this.redrawChatback = true;
						}
						if (var2 == 13 || var2 == 10) {
							this.showSocialInput = false;
							this.redrawChatback = true;
							long var8;
							if (this.socialAction == 1) {
								var8 = JString.toBase37(this.socialInput);
								this.addFriend(var8, -460);
							}
							if (this.socialAction == 2 && this.friendCount > 0) {
								var8 = JString.toBase37(this.socialInput);
								this.removeFriend(43808, var8);
							}
							if (this.socialAction == 3 && this.socialInput.length() > 0) {
								this.out.p1isaac((byte) -34, 148);
								this.out.p1(0);
								var7 = this.out.pos;
								this.out.p8(true, this.socialName37);
								WordPack.pack(this.out, true, this.socialInput);
								this.out.psize1(0, this.out.pos - var7);
								this.socialInput = JString.toSentenceCase(this.socialInput, 0);
								this.socialInput = WordFilter.filter(this.socialInput, 0);
								this.addMessage(6, this.socialInput, (byte) 4, JString.formatName(0, JString.fromBase37(this.socialName37, false)));
								if (this.privateChatSetting == 2) {
									this.privateChatSetting = 1;
									this.redrawPrivacySettings = true;
									this.out.p1isaac((byte) -34, 244);
									this.out.p1(this.publicChatSetting);
									this.out.p1(this.privateChatSetting);
									this.out.p1(this.tradeChatSetting);
								}
							}
							if (this.socialAction == 4 && this.ignoreCount < 100) {
								var8 = JString.toBase37(this.socialInput);
								this.addIgnore(var8, (byte) 3);
							}
							if (this.socialAction == 5 && this.ignoreCount > 0) {
								var8 = JString.toBase37(this.socialInput);
								this.removeIgnore(1, var8);
							}
						}
					} else if (this.chatbackInputOpen) {
						if (var2 >= 48 && var2 <= 57 && this.chatbackInput.length() < 10) {
							this.chatbackInput = this.chatbackInput + (char) var2;
							this.redrawChatback = true;
						}
						if (var2 == 8 && this.chatbackInput.length() > 0) {
							this.chatbackInput = this.chatbackInput.substring(0, this.chatbackInput.length() - 1);
							this.redrawChatback = true;
						}
						if (var2 == 13 || var2 == 10) {
							if (this.chatbackInput.length() > 0) {
								var7 = 0;
								try {
									var7 = Integer.parseInt(this.chatbackInput);
								} catch (Exception var6) {
								}
								this.out.p1isaac((byte) -34, 237);
								this.out.p4(var7);
							}
							this.chatbackInputOpen = false;
							this.redrawChatback = true;
						}
					} else if (this.chatInterfaceId == -1) {
						if (var2 >= 32 && var2 <= 122 && this.chatTyped.length() < 80) {
							this.chatTyped = this.chatTyped + (char) var2;
							this.redrawChatback = true;
						}
						if (var2 == 8 && this.chatTyped.length() > 0) {
							this.chatTyped = this.chatTyped.substring(0, this.chatTyped.length() - 1);
							this.redrawChatback = true;
						}
						if ((var2 == 13 || var2 == 10) && this.chatTyped.length() > 0) {
							if (this.chatTyped.equals("::clientdrop") && (super.frame != null || this.getHost(-7437).indexOf("192.168.1.") != -1)) {
								this.tryReconnect(false);
							} else if (this.chatTyped.startsWith("::")) {
								this.out.p1isaac((byte) -34, 4);
								this.out.p1(this.chatTyped.length() - 1);
								this.out.pjstr(this.chatTyped.substring(2));
							} else {
								byte var3 = 0;
								if (this.chatTyped.startsWith("yellow:")) {
									var3 = 0;
									this.chatTyped = this.chatTyped.substring(7);
								}
								if (this.chatTyped.startsWith("red:")) {
									var3 = 1;
									this.chatTyped = this.chatTyped.substring(4);
								}
								if (this.chatTyped.startsWith("green:")) {
									var3 = 2;
									this.chatTyped = this.chatTyped.substring(6);
								}
								if (this.chatTyped.startsWith("cyan:")) {
									var3 = 3;
									this.chatTyped = this.chatTyped.substring(5);
								}
								if (this.chatTyped.startsWith("purple:")) {
									var3 = 4;
									this.chatTyped = this.chatTyped.substring(7);
								}
								if (this.chatTyped.startsWith("white:")) {
									var3 = 5;
									this.chatTyped = this.chatTyped.substring(6);
								}
								if (this.chatTyped.startsWith("flash1:")) {
									var3 = 6;
									this.chatTyped = this.chatTyped.substring(7);
								}
								if (this.chatTyped.startsWith("flash2:")) {
									var3 = 7;
									this.chatTyped = this.chatTyped.substring(7);
								}
								if (this.chatTyped.startsWith("flash3:")) {
									var3 = 8;
									this.chatTyped = this.chatTyped.substring(7);
								}
								if (this.chatTyped.startsWith("glow1:")) {
									var3 = 9;
									this.chatTyped = this.chatTyped.substring(6);
								}
								if (this.chatTyped.startsWith("glow2:")) {
									var3 = 10;
									this.chatTyped = this.chatTyped.substring(6);
								}
								if (this.chatTyped.startsWith("glow3:")) {
									var3 = 11;
									this.chatTyped = this.chatTyped.substring(6);
								}
								byte var4 = 0;
								if (this.chatTyped.startsWith("wave:")) {
									var4 = 1;
									this.chatTyped = this.chatTyped.substring(5);
								}
								if (this.chatTyped.startsWith("scroll:")) {
									var4 = 2;
									this.chatTyped = this.chatTyped.substring(7);
								}
								this.out.p1isaac((byte) -34, 158);
								this.out.p1(0);
								int var5 = this.out.pos;
								this.out.p1(var3);
								this.out.p1(var4);
								WordPack.pack(this.out, true, this.chatTyped);
								this.out.psize1(0, this.out.pos - var5);
								this.chatTyped = JString.toSentenceCase(this.chatTyped, 0);
								this.chatTyped = WordFilter.filter(this.chatTyped, 0);
								this.localPlayer.chat = this.chatTyped;
								this.localPlayer.chatColor = var3;
								this.localPlayer.chatStyle = var4;
								this.localPlayer.chatTimer = 150;
								this.addMessage(2, this.localPlayer.chat, (byte) 4, this.localPlayer.name);
								if (this.publicChatSetting == 2) {
									this.publicChatSetting = 3;
									this.redrawPrivacySettings = true;
									this.out.p1isaac((byte) -34, 244);
									this.out.p1(this.publicChatSetting);
									this.out.p1(this.privateChatSetting);
									this.out.p1(this.tradeChatSetting);
								}
							}
							this.chatTyped = "";
							this.redrawChatback = true;
						}
					}
				}
			} while ((var2 < 97 || var2 > 122) && (var2 < 65 || var2 > 90) && (var2 < 48 || var2 > 57) && var2 != 32);
			if (this.reportAbuseInput.length() < 12) {
				this.reportAbuseInput = this.reportAbuseInput + (char) var2;
			}
		}
	}

	@ObfuscatedName("client.b(Z)V")
	public final void draw(boolean arg0) {
		if (arg0) {
			this.field1161 = -398;
		}
		if (this.errorStarted || this.errorLoading || this.errorHost) {
			this.drawError(false);
			return;
		}
		if (this.ingame) {
			this.drawGame(29510);
		} else {
			this.drawTitleScreen(4);
		}
		this.dragCycles = 0;
	}

	@ObfuscatedName("client.e(B)V")
	public final void updateTitle(byte arg0) {
		if (arg0 != -70) {
			this.levelObjStacks = null;
		}
		int var2;
		int var3;
		if (this.titleScreenState == 0) {
			var2 = super.screenWidth / 2 - 80;
			var3 = super.screenHeight / 2 + 20;
			var3 += 20;
			if (super.mouseClickButton == 1 && super.mouseClickX >= var2 - 75 && super.mouseClickX <= var2 + 75 && super.mouseClickY >= var3 - 20 && super.mouseClickY <= var3 + 20) {
				this.titleScreenState = 3;
				this.titleLoginField = 0;
			}
			var2 = super.screenWidth / 2 + 80;
			if (super.mouseClickButton == 1 && super.mouseClickX >= var2 - 75 && super.mouseClickX <= var2 + 75 && super.mouseClickY >= var3 - 20 && super.mouseClickY <= var3 + 20) {
				this.loginMessage0 = "";
				this.loginMessage1 = "Enter your username & password.";
				this.titleScreenState = 2;
				this.titleLoginField = 0;
			}
		} else if (this.titleScreenState == 2) {
			var2 = super.screenHeight / 2 - 40;
			var2 += 30;
			var2 += 25;
			if (super.mouseClickButton == 1 && super.mouseClickY >= var2 - 15 && super.mouseClickY < var2) {
				this.titleLoginField = 0;
			}
			var2 += 15;
			if (super.mouseClickButton == 1 && super.mouseClickY >= var2 - 15 && super.mouseClickY < var2) {
				this.titleLoginField = 1;
			}
			var2 += 15;
			var3 = super.screenWidth / 2 - 80;
			int var4 = super.screenHeight / 2 + 50;
			int var9 = var4 + 20;
			if (super.mouseClickButton == 1 && super.mouseClickX >= var3 - 75 && super.mouseClickX <= var3 + 75 && super.mouseClickY >= var9 - 20 && super.mouseClickY <= var9 + 20) {
				this.login(this.username, this.password, false);
			}
			var3 = super.screenWidth / 2 + 80;
			if (super.mouseClickButton == 1 && super.mouseClickX >= var3 - 75 && super.mouseClickX <= var3 + 75 && super.mouseClickY >= var9 - 20 && super.mouseClickY <= var9 + 20) {
				this.titleScreenState = 0;
				this.username = "";
				this.password = "";
			}
			while (true) {
				while (true) {
					int var5 = this.pollKey(false);
					if (var5 == -1) {
						return;
					}
					boolean var6 = false;
					for (int var7 = 0; var7 < CHARSET.length(); var7++) {
						if (var5 == CHARSET.charAt(var7)) {
							var6 = true;
							break;
						}
					}
					if (this.titleLoginField == 0) {
						if (var5 == 8 && this.username.length() > 0) {
							this.username = this.username.substring(0, this.username.length() - 1);
						}
						if (var5 == 9 || var5 == 10 || var5 == 13) {
							this.titleLoginField = 1;
						}
						if (var6) {
							this.username = this.username + (char) var5;
						}
						if (this.username.length() > 12) {
							this.username = this.username.substring(0, 12);
						}
					} else if (this.titleLoginField == 1) {
						if (var5 == 8 && this.password.length() > 0) {
							this.password = this.password.substring(0, this.password.length() - 1);
						}
						if (var5 == 9 || var5 == 10 || var5 == 13) {
							this.titleLoginField = 0;
						}
						if (var6) {
							this.password = this.password + (char) var5;
						}
						if (this.password.length() > 20) {
							this.password = this.password.substring(0, 20);
						}
					}
				}
			}
		} else if (this.titleScreenState == 3) {
			var2 = super.screenWidth / 2;
			var3 = super.screenHeight / 2 + 50;
			int var8 = var3 + 20;
			if (super.mouseClickButton == 1 && super.mouseClickX >= var2 - 75 && super.mouseClickX <= var2 + 75 && super.mouseClickY >= var8 - 20 && super.mouseClickY <= var8 + 20) {
				this.titleScreenState = 0;
				return;
			}
		}
	}

	@ObfuscatedName("client.a(Ljava/lang/String;ILjava/lang/String;II)Lub;")
	public final Jagfile loadArchive(String arg0, int arg1, String arg2, int arg3, int arg4) {
		int var7 = 5;
		byte[] var6 = signlink.cacheload(arg2);
		int var8;
		if (var6 != null) {
			this.crc32.reset();
			this.crc32.update(var6);
			var8 = (int) this.crc32.getValue();
			if (var8 != arg1) {
				var6 = null;
			}
		}
		if (var6 != null) {
			return new Jagfile(var6, this.field1277);
		}
		while (var6 == null) {
			this.drawProgress(true, "Requesting " + arg0, arg3);
			try {
				var8 = 0;
				DataInputStream var9 = this.openUrl(arg2 + arg1);
				byte[] var10 = new byte[6];
				var9.readFully(var10, 0, 6);
				Packet var11 = new Packet(363, var10);
				var11.pos = 3;
				int var12 = var11.g3() + 6;
				int var13 = 6;
				var6 = new byte[var12];
				for (int var14 = 0; var14 < 6; var14++) {
					var6[var14] = var10[var14];
				}
				while (var13 < var12) {
					int var15 = var12 - var13;
					if (var15 > 1000) {
						var15 = 1000;
					}
					var13 += var9.read(var6, var13, var15);
					int var16 = var13 * 100 / var12;
					if (var16 != var8) {
						this.drawProgress(true, "Loading " + arg0 + " - " + var16 + "%", arg3);
					}
					var8 = var16;
				}
				var9.close();
			} catch (IOException var18) {
				var6 = null;
				for (var8 = var7; var8 > 0; var8--) {
					this.drawProgress(true, "Error loading - Will retry in " + var8 + " secs.", arg3);
					try {
						Thread.sleep(1000L);
					} catch (Exception var17) {
					}
				}
				var7 *= 2;
				if (var7 > 60) {
					var7 = 60;
				}
			}
		}
		signlink.cachesave(arg2, var6);
		if (arg4 != 0) {
			for (var8 = 1; var8 > 0; var8++) {
			}
		}
		return new Jagfile(var6, this.field1277);
	}

	@ObfuscatedName("client.f(Z)V")
	public final void unloadTitle(boolean arg0) {
		this.flameActive = false;
		while (this.flameThread) {
			this.flameActive = false;
			try {
				Thread.sleep(50L);
			} catch (Exception var3) {
			}
		}
		this.imageTitlebox = null;
		this.imageTitlebutton = null;
		this.imageRunes = null;
		this.flameGradient = null;
		this.flameGradient0 = null;
		this.flameGradient1 = null;
		this.flameGradient2 = null;
		this.flameBuffer0 = null;
		this.flameBuffer1 = null;
		if (!arg0) {
			for (int var2 = 1; var2 > 0; var2++) {
			}
		}
		this.flameBuffer3 = null;
		this.flameBuffer2 = null;
		this.imageFlamesLeft = null;
		this.imageFlamesRight = null;
	}

	@ObfuscatedName("client.a(IIIIIII)V")
	public final void orbitCamera(int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6) {
		int var8 = 2048 - arg3 & 0x7FF;
		int var9 = 2048 - arg2 & 0x7FF;
		int var10 = 0;
		int var11 = 0;
		int var12 = arg6;
		int var13;
		int var14;
		int var15;
		if (var8 != 0) {
			var13 = Model.sin[var8];
			var14 = Model.cos[var8];
			var15 = var11 * var14 - arg6 * var13 >> 16;
			var12 = var11 * var13 + arg6 * var14 >> 16;
			var11 = var15;
		}
		if (var9 != 0) {
			var13 = Model.sin[var9];
			var14 = Model.cos[var9];
			var15 = var12 * var13 + var10 * var14 >> 16;
			var12 = var12 * var14 - var10 * var13 >> 16;
			var10 = var15;
		}
		this.cameraX = arg1 - var10;
		this.cameraY = arg0 - var11;
		this.cameraZ = arg5 - var12;
		this.cameraPitch = arg3;
		if (arg4 != 16418) {
			this.levelObjStacks = null;
		}
		this.cameraYaw = arg2;
	}

	@ObfuscatedName("client.b(II)Ljava/lang/String;")
	public static final String formatObjCountTagged(int arg0, int arg1) {
		String var2 = String.valueOf(arg0);
		for (int var3 = var2.length() - 3; var3 > 0; var3 -= 3) {
			var2 = var2.substring(0, var3) + "," + var2.substring(var3);
		}
		if (arg1 != 0) {
			throw new NullPointerException();
		}
		if (var2.length() > 8) {
			var2 = "@gre@" + var2.substring(0, var2.length() - 8) + " million @whi@(" + var2 + ")";
		} else if (var2.length() > 4) {
			var2 = "@cya@" + var2.substring(0, var2.length() - 4) + "K @whi@(" + var2 + ")";
		}
		return " " + var2;
	}

	@ObfuscatedName("client.a(IZ)V")
	public final void updateTextures(int arg0, boolean arg1) {
		if (!arg1) {
			this.packetType = -1;
		}
		if (lowMemory) {
			return;
		}
		Pix8 var3;
		int var4;
		int var5;
		byte[] var6;
		byte[] var7;
		int var8;
		if (Pix3D.textureCycle[17] >= arg0) {
			var3 = Pix3D.textures[17];
			var4 = var3.width * var3.height - 1;
			var5 = var3.width * this.sceneDelta * 2;
			var6 = var3.pixels;
			var7 = this.textureBuffer;
			for (var8 = 0; var8 <= var4; var8++) {
				var7[var8] = var6[var8 - var5 & var4];
			}
			var3.pixels = var7;
			this.textureBuffer = var6;
			Pix3D.pushTexture(17, 150);
		}
		if (Pix3D.textureCycle[24] < arg0) {
			return;
		}
		var3 = Pix3D.textures[24];
		var4 = var3.width * var3.height - 1;
		var5 = var3.width * this.sceneDelta * 2;
		var6 = var3.pixels;
		var7 = this.textureBuffer;
		for (var8 = 0; var8 <= var4; var8++) {
			var7[var8] = var6[var8 - var5 & var4];
		}
		var3.pixels = var7;
		this.textureBuffer = var6;
		Pix3D.pushTexture(24, 150);
		return;
	}

	@ObfuscatedName("client.g(Z)V")
	public final void updateFlames(boolean arg0) {
		short var2 = 256;
		int var4;
		for (int var3 = 10; var3 < 117; var3++) {
			var4 = (int) (Math.random() * 100.0D);
			if (var4 < 50) {
				this.flameBuffer3[var3 + (var2 - 2 << 7)] = 255;
			}
		}
		int var5;
		int var6;
		int var7;
		for (var4 = 0; var4 < 100; var4++) {
			var5 = (int) (Math.random() * 124.0D) + 2;
			var6 = (int) (Math.random() * 128.0D) + 128;
			var7 = var5 + (var6 << 7);
			this.flameBuffer3[var7] = 192;
		}
		if (!arg0) {
			this.packetType = this.in.g1();
		}
		for (var5 = 1; var5 < var2 - 1; var5++) {
			for (var6 = 1; var6 < 127; var6++) {
				var7 = var6 + (var5 << 7);
				this.flameBuffer2[var7] = (this.flameBuffer3[var7 - 1] + this.flameBuffer3[var7 + 1] + this.flameBuffer3[var7 - 128] + this.flameBuffer3[var7 + 128]) / 4;
			}
		}
		this.flameCycle0 += 128;
		if (this.flameCycle0 > this.flameBuffer0.length) {
			this.flameCycle0 -= this.flameBuffer0.length;
			var6 = (int) (Math.random() * 12.0D);
			this.updateFlameBuffer(578, this.imageRunes[var6]);
		}
		int var8;
		for (var6 = 1; var6 < var2 - 1; var6++) {
			for (var7 = 1; var7 < 127; var7++) {
				var8 = var7 + (var6 << 7);
				int var9 = this.flameBuffer2[var8 + 128] - this.flameBuffer0[var8 + this.flameCycle0 & this.flameBuffer0.length - 1] / 5;
				if (var9 < 0) {
					var9 = 0;
				}
				this.flameBuffer3[var8] = var9;
			}
		}
		for (var7 = 0; var7 < var2 - 1; var7++) {
			this.flameLineOffset[var7] = this.flameLineOffset[var7 + 1];
		}
		this.flameLineOffset[var2 - 1] = (int) (Math.sin((double) loopCycle / 14.0D) * 16.0D + Math.sin((double) loopCycle / 15.0D) * 14.0D + Math.sin((double) loopCycle / 16.0D) * 12.0D);
		if (this.flameGradientCycle0 > 0) {
			this.flameGradientCycle0 -= 4;
		}
		if (this.flameGradientCycle1 > 0) {
			this.flameGradientCycle1 -= 4;
		}
		if (this.flameGradientCycle0 != 0 || this.flameGradientCycle1 != 0) {
			return;
		}
		var8 = (int) (Math.random() * 2000.0D);
		if (var8 == 0) {
			this.flameGradientCycle0 = 1024;
		}
		if (var8 == 1) {
			this.flameGradientCycle1 = 1024;
			return;
		}
	}

	@ObfuscatedName("client.f(B)V")
	public final void drawMinimap(byte arg0) {
		this.areaMapback.bind((byte) 62);
		if (arg0 != -46) {
			this.load();
		}
		int var2 = this.orbitCameraYaw + this.minimapAnticheatAngle & 0x7FF;
		int var3 = this.localPlayer.x / 32 + 48;
		int var4 = 464 - this.localPlayer.z / 32;
		this.imageMinimap.drawRotatedMasked(var2, 146, this.minimapMaskLineOffsets, 151, var4, this.minimapZoom + 256, var3, 21, 9, false, this.minimapMaskLineLengths);
		this.imageCompass.drawRotatedMasked(this.orbitCameraYaw, 33, this.compassMaskLineOffsets, 33, 25, 256, 25, 0, 0, false, this.compassMaskLineLengths);
		for (int var5 = 0; var5 < this.activeMapFunctionCount; var5++) {
			var3 = this.activeMapFunctionX[var5] * 4 + 2 - this.localPlayer.x / 32;
			var4 = this.activeMapFunctionZ[var5] * 4 + 2 - this.localPlayer.z / 32;
			this.drawOnMinimap(var4, 4, this.activeMapFunctions[var5], var3);
		}
		int var7;
		for (int var6 = 0; var6 < 104; var6++) {
			for (var7 = 0; var7 < 104; var7++) {
				LinkList var8 = this.levelObjStacks[this.currentLevel][var6][var7];
				if (var8 != null) {
					var3 = var6 * 4 + 2 - this.localPlayer.x / 32;
					var4 = var7 * 4 + 2 - this.localPlayer.z / 32;
					this.drawOnMinimap(var4, 4, this.imageMapdot0, var3);
				}
			}
		}
		for (var7 = 0; var7 < this.npcCount; var7++) {
			NpcEntity var14 = this.npcs[this.npcIds[var7]];
			if (var14 != null && var14.isVisible(false) && var14.type.minimap) {
				var3 = var14.x / 32 - this.localPlayer.x / 32;
				var4 = var14.z / 32 - this.localPlayer.z / 32;
				this.drawOnMinimap(var4, 4, this.imageMapdot1, var3);
			}
		}
		for (int var15 = 0; var15 < this.playerCount; var15++) {
			PlayerEntity var9 = this.players[this.playerIds[var15]];
			if (var9 != null && var9.isVisible(false)) {
				var3 = var9.x / 32 - this.localPlayer.x / 32;
				var4 = var9.z / 32 - this.localPlayer.z / 32;
				boolean var10 = false;
				long var11 = JString.toBase37(var9.name);
				for (int var13 = 0; var13 < this.friendCount; var13++) {
					if (var11 == this.friendName37[var13] && this.friendWorld[var13] != 0) {
						var10 = true;
						break;
					}
				}
				if (var10) {
					this.drawOnMinimap(var4, 4, this.imageMapdot3, var3);
				} else {
					this.drawOnMinimap(var4, 4, this.imageMapdot2, var3);
				}
			}
		}
		if (this.flagSceneTileX != 0) {
			var3 = this.flagSceneTileX * 4 + 2 - this.localPlayer.x / 32;
			var4 = this.flagSceneTileZ * 4 + 2 - this.localPlayer.z / 32;
			this.drawOnMinimap(var4, 4, this.imageMapflag, var3);
		}
		Pix2D.fillRect(82, 93, 16777215, (byte) 93, 3, 3);
		this.areaViewport.bind((byte) 62);
	}

	@ObfuscatedName("client.b(B)Ljava/awt/Component;")
	public final java.awt.Component getBaseComponent(byte arg0) {
		if (arg0 != 3) {
			this.field1124 = 260;
		}
		if (signlink.mainapp == null) {
			return super.frame == null ? this : super.frame;
		} else {
			return signlink.mainapp;
		}
	}

	@ObfuscatedName("client.m(I)V")
	public final void updateMergeLocs(int arg0) {
		if (arg0 <= 0) {
			this.load();
		}
		if (this.sceneState != 2) {
			return;
		}
		for (LocMergeEntity var2 = (LocMergeEntity) this.mergedLocations.head(); var2 != null; var2 = (LocMergeEntity) this.mergedLocations.next(551)) {
			if (loopCycle >= var2.lastCycle) {
				this.addLoc(var2.angle, var2.x, var2.z, var2.layer, var2.locIndex, var2.shape, -27819, var2.plane);
				var2.unlink();
			}
		}
		cyclelogic5++;
		if (cyclelogic5 > 85) {
			cyclelogic5 = 0;
			this.out.p1isaac((byte) -34, 85);
			return;
		}
	}

	@ObfuscatedName("client.c(II)V")
	public final void createMinimap(int arg0, int arg1) {
		int[] var3 = this.imageMinimap.pixels;
		if (arg1 >= 0) {
			return;
		}
		int var4 = var3.length;
		for (int var5 = 0; var5 < var4; var5++) {
			var3[var5] = 0;
		}
		int var7;
		int var8;
		for (int var6 = 1; var6 < 103; var6++) {
			var7 = (103 - var6) * 512 * 4 + 24628;
			for (var8 = 1; var8 < 103; var8++) {
				if ((this.levelTileFlags[arg0][var8][var6] & 0x18) == 0) {
					this.scene.drawMinimapTile(var3, var7, 512, arg0, var8, var6);
				}
				if (arg0 < 3 && (this.levelTileFlags[arg0 + 1][var8][var6] & 0x8) != 0) {
					this.scene.drawMinimapTile(var3, var7, 512, arg0 + 1, var8, var6);
				}
				var7 += 4;
			}
		}
		var7 = ((int) (Math.random() * 20.0D) + 238 - 10 << 16) + ((int) (Math.random() * 20.0D) + 238 - 10 << 8) + ((int) (Math.random() * 20.0D) + 238 - 10);
		var8 = (int) (Math.random() * 20.0D) + 238 - 10 << 16;
		this.imageMinimap.bind((byte) 62);
		int var10;
		for (int var9 = 1; var9 < 103; var9++) {
			for (var10 = 1; var10 < 103; var10++) {
				if ((this.levelTileFlags[arg0][var10][var9] & 0x18) == 0) {
					this.drawMinimapLoc(field1264, arg0, var7, var10, var8, var9);
				}
				if (arg0 < 3 && (this.levelTileFlags[arg0 + 1][var10][var9] & 0x8) != 0) {
					this.drawMinimapLoc(field1264, arg0 + 1, var7, var10, var8, var9);
				}
			}
		}
		this.areaViewport.bind((byte) 62);
		this.activeMapFunctionCount = 0;
		for (var10 = 0; var10 < 104; var10++) {
			for (int var11 = 0; var11 < 104; var11++) {
				int var12 = this.scene.getGroundDecorationBitset(this.currentLevel, var10, var11);
				if (var12 != 0) {
					var12 = var12 >> 14 & 0x7FFF;
					int var13 = LocType.get(var12).mapfunction;
					if (var13 >= 0) {
						int var14 = var10;
						int var15 = var11;
						if (var13 != 22 && var13 != 29 && var13 != 34 && var13 != 36 && var13 != 46 && var13 != 47 && var13 != 48) {
							byte var16 = 104;
							byte var17 = 104;
							int[][] var18 = this.levelCollisionMap[this.currentLevel].flags;
							for (int var19 = 0; var19 < 10; var19++) {
								int var20 = (int) (Math.random() * 4.0D);
								if (var20 == 0 && var14 > 0 && var14 > var10 - 3 && (var18[var14 - 1][var15] & 0x280108) == 0) {
									var14--;
								}
								if (var20 == 1 && var14 < var16 - 1 && var14 < var10 + 3 && (var18[var14 + 1][var15] & 0x280180) == 0) {
									var14++;
								}
								if (var20 == 2 && var15 > 0 && var15 > var11 - 3 && (var18[var14][var15 - 1] & 0x280102) == 0) {
									var15--;
								}
								if (var20 == 3 && var15 < var17 - 1 && var15 < var11 + 3 && (var18[var14][var15 + 1] & 0x280120) == 0) {
									var15++;
								}
							}
						}
						this.activeMapFunctions[this.activeMapFunctionCount] = this.imageMapfunction[var13];
						this.activeMapFunctionX[this.activeMapFunctionCount] = var14;
						this.activeMapFunctionZ[this.activeMapFunctionCount] = var15;
						this.activeMapFunctionCount++;
					}
				}
			}
		}
	}

	@ObfuscatedName("client.a(IIIIII)V")
	public final void drawMinimapLoc(int arg0, int arg1, int arg2, int arg3, int arg4, int arg5) {
		int var7 = this.scene.getWallBitset(arg1, arg3, arg5);
		int var8;
		int var9;
		int var10;
		int var11;
		int var13;
		int var14;
		if (var7 != 0) {
			var8 = this.scene.getInfo(arg1, arg3, arg5, var7);
			var9 = var8 >> 6 & 0x3;
			var10 = var8 & 0x1F;
			var11 = arg2;
			if (var7 > 0) {
				var11 = arg4;
			}
			int[] var12 = this.imageMinimap.pixels;
			var13 = arg3 * 4 + 24624 + (103 - arg5) * 512 * 4;
			var14 = var7 >> 14 & 0x7FFF;
			LocType var15 = LocType.get(var14);
			if (var15.mapscene == -1) {
				if (var10 == 0 || var10 == 2) {
					if (var9 == 0) {
						var12[var13] = var11;
						var12[var13 + 512] = var11;
						var12[var13 + 1024] = var11;
						var12[var13 + 1536] = var11;
					} else if (var9 == 1) {
						var12[var13] = var11;
						var12[var13 + 1] = var11;
						var12[var13 + 2] = var11;
						var12[var13 + 3] = var11;
					} else if (var9 == 2) {
						var12[var13 + 3] = var11;
						var12[var13 + 3 + 512] = var11;
						var12[var13 + 3 + 1024] = var11;
						var12[var13 + 3 + 1536] = var11;
					} else if (var9 == 3) {
						var12[var13 + 1536] = var11;
						var12[var13 + 1536 + 1] = var11;
						var12[var13 + 1536 + 2] = var11;
						var12[var13 + 1536 + 3] = var11;
					}
				}
				if (var10 == 3) {
					if (var9 == 0) {
						var12[var13] = var11;
					} else if (var9 == 1) {
						var12[var13 + 3] = var11;
					} else if (var9 == 2) {
						var12[var13 + 3 + 1536] = var11;
					} else if (var9 == 3) {
						var12[var13 + 1536] = var11;
					}
				}
				if (var10 == 2) {
					if (var9 == 3) {
						var12[var13] = var11;
						var12[var13 + 512] = var11;
						var12[var13 + 1024] = var11;
						var12[var13 + 1536] = var11;
					} else if (var9 == 0) {
						var12[var13] = var11;
						var12[var13 + 1] = var11;
						var12[var13 + 2] = var11;
						var12[var13 + 3] = var11;
					} else if (var9 == 1) {
						var12[var13 + 3] = var11;
						var12[var13 + 3 + 512] = var11;
						var12[var13 + 3 + 1024] = var11;
						var12[var13 + 3 + 1536] = var11;
					} else if (var9 == 2) {
						var12[var13 + 1536] = var11;
						var12[var13 + 1536 + 1] = var11;
						var12[var13 + 1536 + 2] = var11;
						var12[var13 + 1536 + 3] = var11;
					}
				}
			} else {
				Pix8 var16 = this.imageMapscene[var15.mapscene];
				if (var16 != null) {
					int var17 = (var15.width * 4 - var16.width) / 2;
					int var18 = (var15.length * 4 - var16.height) / 2;
					var16.draw((104 - arg5 - var15.length) * 4 + 48 + var18, arg3 * 4 + 48 + var17, false);
				}
			}
		}
		var7 = this.scene.getLocBitset(arg1, arg3, arg5);
		int var19 = 56 / arg0;
		if (var7 != 0) {
			var8 = this.scene.getInfo(arg1, arg3, arg5, var7);
			var9 = var8 >> 6 & 0x3;
			var10 = var8 & 0x1F;
			var11 = var7 >> 14 & 0x7FFF;
			LocType var22 = LocType.get(var11);
			int var26;
			if (var22.mapscene != -1) {
				Pix8 var24 = this.imageMapscene[var22.mapscene];
				if (var24 != null) {
					var14 = (var22.width * 4 - var24.width) / 2;
					var26 = (var22.length * 4 - var24.height) / 2;
					var24.draw((104 - arg5 - var22.length) * 4 + 48 + var26, arg3 * 4 + 48 + var14, false);
				}
			} else if (var10 == 9) {
				var13 = 15658734;
				if (var7 > 0) {
					var13 = 15597568;
				}
				int[] var25 = this.imageMinimap.pixels;
				var26 = arg3 * 4 + 24624 + (103 - arg5) * 512 * 4;
				if (var9 == 0 || var9 == 2) {
					var25[var26 + 1536] = var13;
					var25[var26 + 1024 + 1] = var13;
					var25[var26 + 512 + 2] = var13;
					var25[var26 + 3] = var13;
				} else {
					var25[var26] = var13;
					var25[var26 + 512 + 1] = var13;
					var25[var26 + 1024 + 2] = var13;
					var25[var26 + 1536 + 3] = var13;
				}
			}
		}
		var7 = this.scene.getGroundDecorationBitset(arg1, arg3, arg5);
		if (var7 == 0) {
			return;
		}
		var8 = var7 >> 14 & 0x7FFF;
		LocType var20 = LocType.get(var8);
		if (var20.mapscene == -1) {
			return;
		}
		Pix8 var21 = this.imageMapscene[var20.mapscene];
		if (var21 != null) {
			var11 = (var20.width * 4 - var21.width) / 2;
			int var23 = (var20.length * 4 - var21.height) / 2;
			var21.draw((104 - arg5 - var20.length) * 4 + 48 + var23, arg3 * 4 + 48 + var11, false);
			return;
		}
	}

	@ObfuscatedName("client.a(Lkb;IZ)V")
	public final void getNpcPos(Packet arg0, int arg1, boolean arg2) {
		this.entityRemovalCount = 0;
		this.entityUpdateCount = 0;
		this.getNpcPosOldVis(-542, arg0, arg1);
		this.getNpcPosNewVis(false, arg0, arg1);
		this.getNpcPosExtended(arg0, arg1, this.field1436);
		if (arg2) {
			field1323 = 338;
		}
		int var5;
		for (int var4 = 0; var4 < this.entityRemovalCount; var4++) {
			var5 = this.entityRemovalIds[var4];
			if (this.npcs[var5].cycle != loopCycle) {
				this.npcs[var5].type = null;
				this.npcs[var5] = null;
			}
		}
		if (arg0.pos != arg1) {
			signlink.reporterror(this.username + " size mismatch in getnpcpos - pos:" + arg0.pos + " psize:" + arg1);
			throw new RuntimeException("eek");
		}
		for (var5 = 0; var5 < this.npcCount; var5++) {
			if (this.npcs[this.npcIds[var5]] == null) {
				signlink.reporterror(this.username + " null entry in npc list - pos:" + var5 + " size:" + this.npcCount);
				throw new RuntimeException("eek");
			}
		}
	}

	@ObfuscatedName("client.a(Ljava/lang/Runnable;I)V")
	public final void startThread(Runnable arg0, int arg1) {
		if (signlink.mainapp == null) {
			super.startThread(arg0, arg1);
		} else {
			signlink.startthread(arg0, arg1);
		}
	}

	@ObfuscatedName("client.n(I)V")
	public final void loadTitleImages(int arg0) {
		this.imageTitlebox = new Pix8(this.archiveTitle, "titlebox", 0);
		this.imageTitlebutton = new Pix8(this.archiveTitle, "titlebutton", 0);
		if (arg0 != 0) {
			this.packetType = this.in.g1();
		}
		this.imageRunes = new Pix8[12];
		for (int var2 = 0; var2 < 12; var2++) {
			this.imageRunes[var2] = new Pix8(this.archiveTitle, "runes", var2);
		}
		this.imageFlamesLeft = new Pix32(128, 265);
		this.imageFlamesRight = new Pix32(128, 265);
		for (int var3 = 0; var3 < 33920; var3++) {
			this.imageFlamesLeft.pixels[var3] = this.imageTitle0.pixels[var3];
		}
		for (int var4 = 0; var4 < 33920; var4++) {
			this.imageFlamesRight.pixels[var4] = this.imageTitle1.pixels[var4];
		}
		this.flameGradient0 = new int[256];
		for (int var5 = 0; var5 < 64; var5++) {
			this.flameGradient0[var5] = var5 * 262144;
		}
		for (int var6 = 0; var6 < 64; var6++) {
			this.flameGradient0[var6 + 64] = var6 * 1024 + 16711680;
		}
		for (int var7 = 0; var7 < 64; var7++) {
			this.flameGradient0[var7 + 128] = var7 * 4 + 16776960;
		}
		for (int var8 = 0; var8 < 64; var8++) {
			this.flameGradient0[var8 + 192] = 16777215;
		}
		this.flameGradient1 = new int[256];
		for (int var9 = 0; var9 < 64; var9++) {
			this.flameGradient1[var9] = var9 * 1024;
		}
		for (int var10 = 0; var10 < 64; var10++) {
			this.flameGradient1[var10 + 64] = var10 * 4 + 65280;
		}
		for (int var11 = 0; var11 < 64; var11++) {
			this.flameGradient1[var11 + 128] = var11 * 262144 + 65535;
		}
		for (int var12 = 0; var12 < 64; var12++) {
			this.flameGradient1[var12 + 192] = 16777215;
		}
		this.flameGradient2 = new int[256];
		for (int var13 = 0; var13 < 64; var13++) {
			this.flameGradient2[var13] = var13 * 4;
		}
		for (int var14 = 0; var14 < 64; var14++) {
			this.flameGradient2[var14 + 64] = var14 * 262144 + 255;
		}
		for (int var15 = 0; var15 < 64; var15++) {
			this.flameGradient2[var15 + 128] = var15 * 1024 + 16711935;
		}
		for (int var16 = 0; var16 < 64; var16++) {
			this.flameGradient2[var16 + 192] = 16777215;
		}
		this.flameGradient = new int[256];
		this.flameBuffer0 = new int[32768];
		this.flameBuffer1 = new int[32768];
		this.updateFlameBuffer(578, null);
		this.flameBuffer3 = new int[32768];
		this.flameBuffer2 = new int[32768];
		this.drawProgress(true, "Connecting to fileserver", 10);
		if (!this.flameActive) {
			this.flamesThread = true;
			this.flameActive = true;
			this.startThread(this, 2);
		}
	}

	@ObfuscatedName("client.a(ILkb;I)V")
	public final void getPlayerOldVis(int arg0, Packet arg1, int arg2) {
		int var4 = arg1.gBit(9, 8);
		int var5;
		if (arg2 != 0) {
			for (var5 = 1; var5 > 0; var5++) {
			}
		}
		if (var4 < this.playerCount) {
			for (var5 = var4; var5 < this.playerCount; var5++) {
				this.entityRemovalIds[this.entityRemovalCount++] = this.playerIds[var5];
			}
		}
		if (var4 > this.playerCount) {
			signlink.reporterror(this.username + " Too many players");
			throw new RuntimeException("eek");
		}
		this.playerCount = 0;
		for (var5 = 0; var5 < var4; var5++) {
			int var6 = this.playerIds[var5];
			PlayerEntity var7 = this.players[var6];
			int var8 = arg1.gBit(9, 1);
			if (var8 == 0) {
				this.playerIds[this.playerCount++] = var6;
				var7.cycle = loopCycle;
			} else {
				int var9 = arg1.gBit(9, 2);
				if (var9 == 0) {
					this.playerIds[this.playerCount++] = var6;
					var7.cycle = loopCycle;
					this.entityUpdateIds[this.entityUpdateCount++] = var6;
				} else {
					int var10;
					int var11;
					if (var9 == 1) {
						this.playerIds[this.playerCount++] = var6;
						var7.cycle = loopCycle;
						var10 = arg1.gBit(9, 3);
						var7.moveAlongRoute(false, var10, (byte) 6);
						var11 = arg1.gBit(9, 1);
						if (var11 == 1) {
							this.entityUpdateIds[this.entityUpdateCount++] = var6;
						}
					} else if (var9 == 2) {
						this.playerIds[this.playerCount++] = var6;
						var7.cycle = loopCycle;
						var10 = arg1.gBit(9, 3);
						var7.moveAlongRoute(true, var10, (byte) 6);
						var11 = arg1.gBit(9, 3);
						var7.moveAlongRoute(true, var11, (byte) 6);
						int var12 = arg1.gBit(9, 1);
						if (var12 == 1) {
							this.entityUpdateIds[this.entityUpdateCount++] = var6;
						}
					} else if (var9 == 3) {
						this.entityRemovalIds[this.entityRemovalCount++] = var6;
					}
				}
			}
		}
	}

	@ObfuscatedName("client.b(IIIIII)V")
	public final void drawScrollbar(int arg0, int arg1, int arg2, int arg3, int arg4, int arg5) {
		this.imageScrollbar0.draw(arg2, arg1, false);
		this.imageScrollbar1.draw(arg2 + arg5 - 16, arg1, false);
		Pix2D.fillRect(arg2 + 16, arg1, this.SCROLLBAR_TRACK, (byte) 93, 16, arg5 - 32);
		int var7 = (arg5 - 32) * arg5 / arg4;
		if (var7 < 8) {
			var7 = 8;
		}
		int var8 = (arg5 - 32 - var7) * arg3 / (arg4 - arg5);
		if (arg0 <= 0) {
			this.packetType = -1;
		}
		Pix2D.fillRect(arg2 + 16 + var8, arg1, this.SCROLLBAR_GRIP_FOREGROUND, (byte) 93, 16, var7);
		Pix2D.vline(this.SCROLLBAR_GRIP_HIGHLIGHT, this.field1092, arg2 + 16 + var8, var7, arg1);
		Pix2D.vline(this.SCROLLBAR_GRIP_HIGHLIGHT, this.field1092, arg2 + 16 + var8, var7, arg1 + 1);
		Pix2D.hline(this.SCROLLBAR_GRIP_HIGHLIGHT, 0, arg2 + 16 + var8, 16, arg1);
		Pix2D.hline(this.SCROLLBAR_GRIP_HIGHLIGHT, 0, arg2 + 17 + var8, 16, arg1);
		Pix2D.vline(this.SCROLLBAR_GRIP_LOWLIGHT, this.field1092, arg2 + 16 + var8, var7, arg1 + 15);
		Pix2D.vline(this.SCROLLBAR_GRIP_LOWLIGHT, this.field1092, arg2 + 17 + var8, var7 - 1, arg1 + 14);
		Pix2D.hline(this.SCROLLBAR_GRIP_LOWLIGHT, 0, arg2 + 15 + var8 + var7, 16, arg1);
		Pix2D.hline(this.SCROLLBAR_GRIP_LOWLIGHT, 0, arg2 + 14 + var8 + var7, 15, arg1 + 1);
	}

	@ObfuscatedName("client.g(B)V")
	public final void validateCharacterDesign(byte arg0) {
		this.updateDesignModel = true;
		if (arg0 != -6) {
			this.levelObjStacks = null;
		}
		for (int var2 = 0; var2 < 7; var2++) {
			this.designIdentikits[var2] = -1;
			for (int var3 = 0; var3 < IdkType.count; var3++) {
				if (!IdkType.instances[var3].disable && IdkType.instances[var3].type == var2 + (this.designGenderMale ? 0 : 7)) {
					this.designIdentikits[var2] = var3;
					break;
				}
			}
		}
	}

	@ObfuscatedName("client.a([BIIZ)V")
	public final void saveMidi(byte[] arg0, int arg1, int arg2, boolean arg3) {
		if (arg1 <= 0) {
			this.field1214 = !this.field1214;
		}
		signlink.midifade = arg3 ? 1 : 0;
		signlink.midisave(arg0, arg2);
	}

	@ObfuscatedName("client.h(Z)V")
	public final void pushNpcs(boolean arg0) {
		for (int var2 = 0; var2 < this.npcCount; var2++) {
			NpcEntity var3 = this.npcs[this.npcIds[var2]];
			int var4 = (this.npcIds[var2] << 14) + 536870912;
			if (var3 != null && var3.isVisible(false)) {
				int var5 = var3.x >> 7;
				int var6 = var3.z >> 7;
				if (var5 >= 0 && var5 < 104 && var6 >= 0 && var6 < 104) {
					if (var3.size == 1 && (var3.x & 0x7F) == 64 && (var3.z & 0x7F) == 64) {
						if (this.tileLastOccupiedCycle[var5][var6] == this.sceneCycle) {
							continue;
						}
						this.tileLastOccupiedCycle[var5][var6] = this.sceneCycle;
					}
					this.scene.addTemporary(-44713, var3.z, (var3.size - 1) * 64 + 60, var3.yaw, var3.x, var4, var3.seqStretches, null, var3, this.getHeightmapY(this.currentLevel, var3.x, (byte) 5, var3.z), this.currentLevel);
				}
			}
		}
		if (arg0) {
			this.packetType = -1;
		}
	}

	@ObfuscatedName("client.a(IIZ)V")
	public final void setMidiVolume(int arg0, int arg1, boolean arg2) {
		signlink.midivol = arg1;
		this.packetSize += arg0;
		if (arg2) {
			signlink.midi = "voladjust";
		}
	}

	@ObfuscatedName("client.o(I)V")
	public final void drawTitleScreen(int arg0) {
		this.loadTitle((byte) 99);
		this.imageTitle4.bind((byte) 62);
		if (arg0 < 4 || arg0 > 4) {
			return;
		}
		this.imageTitlebox.draw(0, 0, false);
		short var2 = 360;
		short var3 = 200;
		int var4;
		int var5;
		int var6;
		if (this.titleScreenState == 0) {
			var4 = var3 / 2 - 20;
			this.fontBold12.drawStringTaggableCenter(var2 / 2, 16776960, true, var4, "Welcome to RuneScape", 0);
			int var7 = var4 + 30;
			var5 = var2 / 2 - 80;
			var6 = var3 / 2 + 20;
			this.imageTitlebutton.draw(var6 - 20, var5 - 73, false);
			this.fontBold12.drawStringTaggableCenter(var5, 16777215, true, var6 + 5, "New user", 0);
			int var8 = var2 / 2 + 80;
			this.imageTitlebutton.draw(var6 - 20, var8 - 73, false);
			this.fontBold12.drawStringTaggableCenter(var8, 16777215, true, var6 + 5, "Existing User", 0);
		}
		if (this.titleScreenState == 2) {
			var4 = var3 / 2 - 40;
			if (this.loginMessage0.length() > 0) {
				this.fontBold12.drawStringTaggableCenter(var2 / 2, 16776960, true, var4 - 15, this.loginMessage0, 0);
				this.fontBold12.drawStringTaggableCenter(var2 / 2, 16776960, true, var4, this.loginMessage1, 0);
				var4 += 30;
			} else {
				this.fontBold12.drawStringTaggableCenter(var2 / 2, 16776960, true, var4 - 7, this.loginMessage1, 0);
				var4 += 30;
			}
			this.fontBold12.drawStringTaggable(var2 / 2 - 90, 6, var4, "Username: " + this.username + (this.titleLoginField == 0 & loopCycle % 40 < 20 ? "@yel@|" : ""), true, 16777215);
			var4 += 15;
			this.fontBold12.drawStringTaggable(var2 / 2 - 88, 6, var4, "Password: " + JString.toAsterisks(7, this.password) + (this.titleLoginField == 1 & loopCycle % 40 < 20 ? "@yel@|" : ""), true, 16777215);
			var4 += 15;
			var5 = var2 / 2 - 80;
			var6 = var3 / 2 + 50;
			this.imageTitlebutton.draw(var6 - 20, var5 - 73, false);
			this.fontBold12.drawStringTaggableCenter(var5, 16777215, true, var6 + 5, "Login", 0);
			var5 = var2 / 2 + 80;
			this.imageTitlebutton.draw(var6 - 20, var5 - 73, false);
			this.fontBold12.drawStringTaggableCenter(var5, 16777215, true, var6 + 5, "Cancel", 0);
		}
		if (this.titleScreenState == 3) {
			this.fontBold12.drawStringTaggableCenter(var2 / 2, 16776960, true, var3 / 2 - 60, "Create a free account", 0);
			var4 = var3 / 2 - 35;
			this.fontBold12.drawStringTaggableCenter(var2 / 2, 16777215, true, var4, "To create a new account you need to", 0);
			var4 += 15;
			this.fontBold12.drawStringTaggableCenter(var2 / 2, 16777215, true, var4, "go back to the main RuneScape webpage", 0);
			var4 += 15;
			this.fontBold12.drawStringTaggableCenter(var2 / 2, 16777215, true, var4, "and choose the red 'create account'", 0);
			var4 += 15;
			this.fontBold12.drawStringTaggableCenter(var2 / 2, 16777215, true, var4, "button at the top right of that page.", 0);
			var4 += 15;
			var5 = var2 / 2;
			var6 = var3 / 2 + 50;
			this.imageTitlebutton.draw(var6 - 20, var5 - 73, false);
			this.fontBold12.drawStringTaggableCenter(var5, 16777215, true, var6 + 5, "Cancel", 0);
		}
		this.imageTitle4.draw(186, super.graphics, 214, 5193);
		if (!this.redrawBackground) {
			return;
		}
		this.redrawBackground = false;
		this.imageTitle2.draw(0, super.graphics, 128, 5193);
		this.imageTitle3.draw(386, super.graphics, 214, 5193);
		this.imageTitle5.draw(265, super.graphics, 0, 5193);
		this.imageTitle6.draw(265, super.graphics, 574, 5193);
		this.imageTitle7.draw(186, super.graphics, 128, 5193);
		this.imageTitle8.draw(186, super.graphics, 574, 5193);
	}

	@ObfuscatedName("client.p(I)V")
	public final void prepareGameScreen(int arg0) {
		if (this.areaChatback != null) {
			return;
		}
		this.unloadTitle(true);
		super.drawArea = null;
		this.imageTitle2 = null;
		this.imageTitle3 = null;
		this.imageTitle4 = null;
		this.imageTitle0 = null;
		if (arg0 != -7185) {
			return;
		}
		this.imageTitle1 = null;
		this.imageTitle5 = null;
		this.imageTitle6 = null;
		this.imageTitle7 = null;
		this.imageTitle8 = null;
		this.areaChatback = new PixMap(this.getBaseComponent(field1481), 479, 299, 96);
		this.areaMapback = new PixMap(this.getBaseComponent(field1481), 168, 299, 160);
		Pix2D.clear(this.field1508);
		this.imageMapback.draw(0, 0, false);
		this.areaSidebar = new PixMap(this.getBaseComponent(field1481), 190, 299, 261);
		this.areaViewport = new PixMap(this.getBaseComponent(field1481), 512, 299, 334);
		Pix2D.clear(this.field1508);
		this.areaBackbase1 = new PixMap(this.getBaseComponent(field1481), 501, 299, 61);
		this.areaBackbase2 = new PixMap(this.getBaseComponent(field1481), 288, 299, 40);
		this.areaBackhmid1 = new PixMap(this.getBaseComponent(field1481), 269, 299, 66);
		this.redrawBackground = true;
	}

	@ObfuscatedName("client.a(IILkb;)V")
	public final void getPlayerNewVis(int arg0, int arg1, Packet arg2) {
		int var4;
		if (arg0 < 2 || arg0 > 2) {
			for (var4 = 1; var4 > 0; var4++) {
			}
		}
		while (arg2.bitPos + 10 < arg1 * 8) {
			var4 = arg2.gBit(9, 11);
			if (var4 == 2047) {
				break;
			}
			if (this.players[var4] == null) {
				this.players[var4] = new PlayerEntity();
				if (this.playerAppearanceBuffer[var4] != null) {
					this.players[var4].read(false, this.playerAppearanceBuffer[var4]);
				}
			}
			this.playerIds[this.playerCount++] = var4;
			PlayerEntity var5 = this.players[var4];
			var5.cycle = loopCycle;
			int var6 = arg2.gBit(9, 5);
			if (var6 > 15) {
				var6 -= 32;
			}
			int var7 = arg2.gBit(9, 5);
			if (var7 > 15) {
				var7 -= 32;
			}
			int var8 = arg2.gBit(9, 1);
			var5.teleport(false, var8 == 1, this.localPlayer.pathTileX[0] + var6, this.localPlayer.pathTileZ[0] + var7);
			int var9 = arg2.gBit(9, 1);
			if (var9 == 1) {
				this.entityUpdateIds[this.entityUpdateCount++] = var4;
			}
		}
		arg2.accessBytes(this.field1178);
	}

	@ObfuscatedName("client.q(I)V")
	public final void logout(int arg0) {
		try {
			if (this.stream != null) {
				this.stream.close();
			}
		} catch (Exception var3) {
		}
		this.stream = null;
		this.ingame = false;
		this.titleScreenState = 0;
		this.username = "";
		this.password = "";
		InputTracking.setDisabled((byte) 65);
		this.clearCaches(this.field1208);
		if (arg0 >= 0) {
			return;
		}
		this.scene.reset(742);
		for (int var2 = 0; var2 < 4; var2++) {
			this.levelCollisionMap[var2].reset((byte) 74);
		}
		System.gc();
		this.stopMidi(0);
		this.currentMidi = null;
		this.nextMusicDelay = 0;
	}

	@ObfuscatedName("client.a(IIILhc;I)V")
	public final void drawInterface(int arg0, int arg1, int arg2, Component arg3, int arg4) {
		if (arg3.type != 0 || arg3.childId == null || arg3.hide && this.viewportHoveredInterfaceIndex != arg3.id && this.sidebarHoveredInterfaceIndex != arg3.id && this.chatHoveredInterfaceIndex != arg3.id) {
			return;
		}
		int var6 = Pix2D.boundLeft;
		int var7 = Pix2D.boundTop;
		int var8 = Pix2D.boundRight;
		int var9 = Pix2D.boundBottom;
		if (arg2 != 38682) {
			this.packetType = -1;
		}
		Pix2D.setClipping(arg0 + arg3.height, arg0, arg1 + arg3.width, 789, arg1);
		int var10 = arg3.childId.length;
		for (int var11 = 0; var11 < var10; var11++) {
			int var12 = arg3.childX[var11] + arg1;
			int var13 = arg3.childY[var11] + arg0 - arg4;
			Component var14 = Component.instances[arg3.childId[var11]];
			int var25 = var12 + var14.x;
			int var26 = var13 + var14.y;
			if (var14.clientCode > 0) {
				this.updateInterfaceContent(0, var14);
			}
			if (var14.type == 0) {
				if (var14.scrollPosition > var14.scroll - var14.height) {
					var14.scrollPosition = var14.scroll - var14.height;
				}
				if (var14.scrollPosition < 0) {
					var14.scrollPosition = 0;
				}
				this.drawInterface(var26, var25, 38682, var14, var14.scrollPosition);
				if (var14.scroll > var14.height) {
					this.drawScrollbar(this.field1168, var25 + var14.width, var26, var14.scrollPosition, var14.scroll, var14.height);
				}
			} else if (var14.type != 1) {
				int var16;
				int var17;
				int var18;
				int var21;
				int var22;
				int var27;
				int var32;
				int var33;
				if (var14.type == 2) {
					var27 = 0;
					for (var16 = 0; var16 < var14.height; var16++) {
						for (var17 = 0; var17 < var14.width; var17++) {
							var18 = var25 + var17 * (var14.marginX + 32);
							var32 = var26 + var16 * (var14.marginY + 32);
							if (var27 < 20) {
								var18 += var14.invSlotOffsetX[var27];
								var32 += var14.invSlotOffsetY[var27];
							}
							if (var14.invSlotObjId[var27] > 0) {
								var33 = 0;
								var21 = 0;
								var22 = var14.invSlotObjId[var27] - 1;
								if (var18 >= -32 && var18 <= 512 && var32 >= -32 && var32 <= 334 || this.objDragArea != 0 && this.objDragSlot == var27) {
									Pix32 var23 = ObjType.getIcon(var22, 24638, var14.invSlotObjCount[var27]);
									if (this.objDragArea != 0 && this.objDragSlot == var27 && this.objDragInterfaceId == var14.id) {
										var33 = super.mouseX - this.objGrabX;
										var21 = super.mouseY - this.objGrabY;
										if (var33 < 5 && var33 > -5) {
											var33 = 0;
										}
										if (var21 < 5 && var21 > -5) {
											var21 = 0;
										}
										if (this.objDragCycles < 5) {
											var33 = 0;
											var21 = 0;
										}
										var23.drawAlpha(128, var18 + var33, var32 + var21, (byte) -26);
									} else if (this.selectedArea != 0 && this.selectedItem == var27 && this.selectedInterface == var14.id) {
										var23.drawAlpha(128, var18, var32, (byte) -26);
									} else {
										var23.draw(var32, var18, false);
									}
									if (var23.cropW == 33 || var14.invSlotObjCount[var27] != 1) {
										int var24 = var14.invSlotObjCount[var27];
										this.fontPlain11.drawString(var18 + 1 + var33, var32 + 10 + var21, false, 0, formatObjCount(true, var24));
										this.fontPlain11.drawString(var18 + var33, var32 + 9 + var21, false, 16776960, formatObjCount(true, var24));
									}
								}
							} else if (var14.invSlotSprite != null && var27 < 20) {
								Pix32 var36 = var14.invSlotSprite[var27];
								if (var36 != null) {
									var36.draw(var32, var18, false);
								}
							}
							var27++;
						}
					}
				} else if (var14.type != 3) {
					PixFont var15;
					if (var14.type == 4) {
						var15 = var14.font;
						var16 = var14.colour;
						String var29 = var14.text;
						if ((this.chatHoveredInterfaceIndex == var14.id || this.sidebarHoveredInterfaceIndex == var14.id || this.viewportHoveredInterfaceIndex == var14.id) && var14.overColour != 0) {
							var16 = var14.overColour;
						}
						if (this.executeInterfaceScript(var14, 65)) {
							var16 = var14.activeColour;
							if (var14.activeText.length() > 0) {
								var29 = var14.activeText;
							}
						}
						if (var14.buttonType == 6 && this.pressedContinueOption) {
							var29 = "Please wait...";
							var16 = var14.colour;
						}
						var32 = var26 + var15.height;
						while (var29.length() > 0) {
							if (var29.indexOf("%") != -1) {
								label260: while (true) {
									var33 = var29.indexOf("%1");
									if (var33 == -1) {
										while (true) {
											var33 = var29.indexOf("%2");
											if (var33 == -1) {
												while (true) {
													var33 = var29.indexOf("%3");
													if (var33 == -1) {
														while (true) {
															var33 = var29.indexOf("%4");
															if (var33 == -1) {
																while (true) {
																	var33 = var29.indexOf("%5");
																	if (var33 == -1) {
																		break label260;
																	}
																	var29 = var29.substring(0, var33) + this.getIntString(this.executeClientscript1(var14, false, 4), -676) + var29.substring(var33 + 2);
																}
															}
															var29 = var29.substring(0, var33) + this.getIntString(this.executeClientscript1(var14, false, 3), -676) + var29.substring(var33 + 2);
														}
													}
													var29 = var29.substring(0, var33) + this.getIntString(this.executeClientscript1(var14, false, 2), -676) + var29.substring(var33 + 2);
												}
											}
											var29 = var29.substring(0, var33) + this.getIntString(this.executeClientscript1(var14, false, 1), -676) + var29.substring(var33 + 2);
										}
									}
									var29 = var29.substring(0, var33) + this.getIntString(this.executeClientscript1(var14, false, 0), -676) + var29.substring(var33 + 2);
								}
							}
							var33 = var29.indexOf("\\n");
							String var30;
							if (var33 == -1) {
								var30 = var29;
								var29 = "";
							} else {
								var30 = var29.substring(0, var33);
								var29 = var29.substring(var33 + 2);
							}
							if (var14.center) {
								var15.drawStringTaggableCenter(var25 + var14.width / 2, var16, var14.shadowed, var32, var30, 0);
							} else {
								var15.drawStringTaggable(var25, 6, var32, var30, var14.shadowed, var16);
							}
							var32 += var15.height;
						}
					} else if (var14.type == 5) {
						Pix32 var28;
						if (this.executeInterfaceScript(var14, 65)) {
							var28 = var14.activeGraphic;
						} else {
							var28 = var14.graphic;
						}
						if (var28 != null) {
							var28.draw(var26, var25, false);
						}
					} else if (var14.type == 6) {
						var27 = Pix3D.centerW3D;
						var16 = Pix3D.centerH3D;
						Pix3D.centerW3D = var25 + var14.width / 2;
						Pix3D.centerH3D = var26 + var14.height / 2;
						var17 = Pix3D.sinTable[var14.xan] * var14.zoom >> 16;
						var18 = Pix3D.cosTable[var14.xan] * var14.zoom >> 16;
						boolean var31 = this.executeInterfaceScript(var14, 65);
						if (var31) {
							var33 = var14.activeAnim;
						} else {
							var33 = var14.anim;
						}
						Model var34;
						if (var33 == -1) {
							var34 = var14.getModel(-1, -1, var31);
						} else {
							SeqType var35 = SeqType.instances[var33];
							var34 = var14.getModel(var35.frames[var14.seqFrame], var35.iframes[var14.seqFrame], var31);
						}
						if (var34 != null) {
							var34.drawSimple(0, var14.yan, 0, var14.xan, 0, var17, var18);
						}
						Pix3D.centerW3D = var27;
						Pix3D.centerH3D = var16;
					} else if (var14.type == 7) {
						var15 = var14.font;
						var16 = 0;
						for (var17 = 0; var17 < var14.height; var17++) {
							for (var18 = 0; var18 < var14.width; var18++) {
								if (var14.invSlotObjId[var16] > 0) {
									ObjType var19 = ObjType.get(var14.invSlotObjId[var16] - 1);
									String var20 = var19.name;
									if (var19.stackable || var14.invSlotObjCount[var16] != 1) {
										var20 = var20 + " x" + formatObjCountTagged(var14.invSlotObjCount[var16], 0);
									}
									var21 = var25 + var18 * (var14.marginX + 115);
									var22 = var26 + var17 * (var14.marginY + 12);
									if (var14.center) {
										var15.drawStringTaggableCenter(var21 + var14.width / 2, var14.colour, var14.shadowed, var22, var20, 0);
									} else {
										var15.drawStringTaggable(var21, 6, var22, var20, var14.shadowed, var14.colour);
									}
								}
								var16++;
							}
						}
					}
				} else if (var14.fill) {
					Pix2D.fillRect(var26, var25, var14.colour, (byte) 93, var14.width, var14.height);
				} else {
					Pix2D.drawRect(3, var25, var14.colour, var14.height, var26, var14.width);
				}
			}
		}
		Pix2D.setClipping(var9, var7, var8, 789, var6);
	}

	@ObfuscatedName("client.a(ZILkb;)V")
	public final void getPlayerExtended(boolean arg0, int arg1, Packet arg2) {
		for (int var4 = 0; var4 < this.entityUpdateCount; var4++) {
			int var5 = this.entityUpdateIds[var4];
			PlayerEntity var6 = this.players[var5];
			int var7 = arg2.g1();
			if ((var7 & 0x80) == 128) {
				var7 += arg2.g1() << 8;
			}
			this.getPlayerExtended(true, var5, var7, arg2, var6);
		}
		this.ingame &= arg0;
	}

	@ObfuscatedName("client.d(II)V")
	public final void updateVarp(int arg0, int arg1) {
		int var6 = 19 / arg1;
		int var3 = VarpType.instances[arg0].clientcode;
		if (var3 == 0) {
			return;
		}
		int var4 = this.varps[arg0];
		if (var3 == 1) {
			if (var4 == 1) {
				Pix3D.setBrightness(true, 0.9D);
			}
			if (var4 == 2) {
				Pix3D.setBrightness(true, 0.8D);
			}
			if (var4 == 3) {
				Pix3D.setBrightness(true, 0.7D);
			}
			if (var4 == 4) {
				Pix3D.setBrightness(true, 0.6D);
			}
			ObjType.iconCache.clear();
			this.redrawBackground = true;
		}
		if (var3 == 3) {
			boolean var5 = this.midiActive;
			if (var4 == 0) {
				this.setMidiVolume(0, 0, this.midiActive);
				this.midiActive = true;
			}
			if (var4 == 1) {
				this.setMidiVolume(0, -400, this.midiActive);
				this.midiActive = true;
			}
			if (var4 == 2) {
				this.setMidiVolume(0, -800, this.midiActive);
				this.midiActive = true;
			}
			if (var4 == 3) {
				this.setMidiVolume(0, -1200, this.midiActive);
				this.midiActive = true;
			}
			if (var4 == 4) {
				this.midiActive = false;
			}
			if (this.midiActive != var5) {
				if (this.midiActive) {
					this.setMidi(false, this.midiCrc, this.currentMidi, this.midiSize);
				} else {
					this.stopMidi(0);
				}
				this.nextMusicDelay = 0;
			}
		}
		if (var3 == 4) {
			if (var4 == 0) {
				this.waveEnabled = true;
				this.setWaveVolume(0, 0);
			}
			if (var4 == 1) {
				this.waveEnabled = true;
				this.setWaveVolume(-400, 0);
			}
			if (var4 == 2) {
				this.waveEnabled = true;
				this.setWaveVolume(-800, 0);
			}
			if (var4 == 3) {
				this.waveEnabled = true;
				this.setWaveVolume(-1200, 0);
			}
			if (var4 == 4) {
				this.waveEnabled = false;
			}
		}
		if (var3 == 5) {
			this.mouseButtonsOption = var4;
		}
		if (var3 == 6) {
			this.chatEffects = var4;
		}
		if (var3 == 8) {
			this.splitPrivateChat = var4;
			this.redrawChatback = true;
		}
	}

	@ObfuscatedName("client.i(Z)V")
	public final void updateNpcs(boolean arg0) {
		this.ingame &= arg0;
		for (int var2 = 0; var2 < this.npcCount; var2++) {
			int var3 = this.npcIds[var2];
			NpcEntity var4 = this.npcs[var3];
			if (var4 != null) {
				this.updateEntity(var4, (byte) -128, var4.type.size);
			}
		}
	}

	@ObfuscatedName("client.a(Lx;BI)V")
	public final void updateEntity(PathingEntity arg0, byte arg1, int arg2) {
		if (arg0.x < 128 || arg0.z < 128 || arg0.x >= 13184 || arg0.z >= 13184) {
			arg0.primarySeqId = -1;
			arg0.spotanimId = -1;
			arg0.forceMoveEndCycle = 0;
			arg0.forceMoveStartCycle = 0;
			arg0.x = arg0.pathTileX[0] * 128 + arg0.size * 64;
			arg0.z = arg0.pathTileZ[0] * 128 + arg0.size * 64;
			arg0.pathLength = 0;
		}
		if (arg0 == this.localPlayer && (arg0.x < 1536 || arg0.z < 1536 || arg0.x >= 11776 || arg0.z >= 11776)) {
			arg0.primarySeqId = -1;
			arg0.spotanimId = -1;
			arg0.forceMoveEndCycle = 0;
			arg0.forceMoveStartCycle = 0;
			arg0.x = arg0.pathTileX[0] * 128 + arg0.size * 64;
			arg0.z = arg0.pathTileZ[0] * 128 + arg0.size * 64;
			arg0.pathLength = 0;
		}
		if (arg0.forceMoveEndCycle > loopCycle) {
			this.updateForceMovement(arg0, -25115);
		} else if (arg0.forceMoveStartCycle >= loopCycle) {
			this.startForceMovement(arg0, 0);
		} else {
			this.updateMovement(598, arg0);
		}
		this.updateFacingDirection(arg0, (byte) 117);
		if (arg1 != -128) {
			this.out.p1(11);
		}
		this.updateSequences(this.field1152, arg0);
	}

	@ObfuscatedName("client.a(Lx;I)V")
	public final void updateForceMovement(PathingEntity arg0, int arg1) {
		int var3 = arg0.forceMoveEndCycle - loopCycle;
		int var4 = arg0.forceMoveStartSceneTileX * 128 + arg0.size * 64;
		int var5 = arg0.forceMoveStartSceneTileZ * 128 + arg0.size * 64;
		arg0.x += (var4 - arg0.x) / var3;
		if (arg1 != -25115) {
			this.field1098 = this.randomIn.takeNextValue();
		}
		arg0.z += (var5 - arg0.z) / var3;
		arg0.seqTrigger = 0;
		if (arg0.forceMoveFaceDirection == 0) {
			arg0.dstYaw = 1024;
		}
		if (arg0.forceMoveFaceDirection == 1) {
			arg0.dstYaw = 1536;
		}
		if (arg0.forceMoveFaceDirection == 2) {
			arg0.dstYaw = 0;
		}
		if (arg0.forceMoveFaceDirection == 3) {
			arg0.dstYaw = 512;
		}
	}

	@ObfuscatedName("client.b(Lx;I)V")
	public final void startForceMovement(PathingEntity arg0, int arg1) {
		this.packetSize += arg1;
		if (arg0.forceMoveStartCycle == loopCycle || arg0.primarySeqId == -1 || arg0.primarySeqDelay != 0 || arg0.primarySeqCycle + 1 > SeqType.instances[arg0.primarySeqId].delay[arg0.primarySeqFrame]) {
			int var3 = arg0.forceMoveStartCycle - arg0.forceMoveEndCycle;
			int var4 = loopCycle - arg0.forceMoveEndCycle;
			int var5 = arg0.forceMoveStartSceneTileX * 128 + arg0.size * 64;
			int var6 = arg0.forceMoveStartSceneTileZ * 128 + arg0.size * 64;
			int var7 = arg0.forceMoveEndSceneTileX * 128 + arg0.size * 64;
			int var8 = arg0.forceMoveEndSceneTileZ * 128 + arg0.size * 64;
			arg0.x = (var5 * (var3 - var4) + var7 * var4) / var3;
			arg0.z = (var6 * (var3 - var4) + var8 * var4) / var3;
		}
		arg0.seqTrigger = 0;
		if (arg0.forceMoveFaceDirection == 0) {
			arg0.dstYaw = 1024;
		}
		if (arg0.forceMoveFaceDirection == 1) {
			arg0.dstYaw = 1536;
		}
		if (arg0.forceMoveFaceDirection == 2) {
			arg0.dstYaw = 0;
		}
		if (arg0.forceMoveFaceDirection == 3) {
			arg0.dstYaw = 512;
		}
		arg0.yaw = arg0.dstYaw;
	}

	@ObfuscatedName("client.a(ILx;)V")
	public final void updateMovement(int arg0, PathingEntity arg1) {
		arg1.secondarySeqId = arg1.seqStandId;
		int var10 = 56 / arg0;
		if (arg1.pathLength == 0) {
			arg1.seqTrigger = 0;
			return;
		}
		if (arg1.primarySeqId != -1 && arg1.primarySeqDelay == 0) {
			SeqType var3 = SeqType.instances[arg1.primarySeqId];
			if (var3.walkmerge == null) {
				arg1.seqTrigger++;
				return;
			}
		}
		int var11 = arg1.x;
		int var4 = arg1.z;
		int var5 = arg1.pathTileX[arg1.pathLength - 1] * 128 + arg1.size * 64;
		int var6 = arg1.pathTileZ[arg1.pathLength - 1] * 128 + arg1.size * 64;
		if (var5 - var11 > 256 || var5 - var11 < -256 || var6 - var4 > 256 || var6 - var4 < -256) {
			arg1.x = var5;
			arg1.z = var6;
			return;
		}
		if (var11 < var5) {
			if (var4 < var6) {
				arg1.dstYaw = 1280;
			} else if (var4 > var6) {
				arg1.dstYaw = 1792;
			} else {
				arg1.dstYaw = 1536;
			}
		} else if (var11 > var5) {
			if (var4 < var6) {
				arg1.dstYaw = 768;
			} else if (var4 > var6) {
				arg1.dstYaw = 256;
			} else {
				arg1.dstYaw = 512;
			}
		} else if (var4 < var6) {
			arg1.dstYaw = 1024;
		} else {
			arg1.dstYaw = 0;
		}
		int var7 = arg1.dstYaw - arg1.yaw & 0x7FF;
		if (var7 > 1024) {
			var7 -= 2048;
		}
		int var8 = arg1.seqTurnAroundId;
		if (var7 >= -256 && var7 <= 256) {
			var8 = arg1.seqWalkId;
		} else if (var7 >= 256 && var7 < 768) {
			var8 = arg1.seqTurnRightId;
		} else if (var7 >= -768 && var7 <= -256) {
			var8 = arg1.seqTurnLeftId;
		}
		if (var8 == -1) {
			var8 = arg1.seqWalkId;
		}
		arg1.secondarySeqId = var8;
		int var9 = 4;
		if (arg1.yaw != arg1.dstYaw && arg1.targetId == -1) {
			var9 = 2;
		}
		if (arg1.pathLength > 2) {
			var9 = 6;
		}
		if (arg1.pathLength > 3) {
			var9 = 8;
		}
		if (arg1.seqTrigger > 0 && arg1.pathLength > 1) {
			var9 = 8;
			arg1.seqTrigger--;
		}
		if (arg1.pathRunning[arg1.pathLength - 1]) {
			var9 <<= 0x1;
		}
		if (var9 >= 8 && arg1.secondarySeqId == arg1.seqWalkId && arg1.seqRunId != -1) {
			arg1.secondarySeqId = arg1.seqRunId;
		}
		if (var11 < var5) {
			arg1.x += var9;
			if (arg1.x > var5) {
				arg1.x = var5;
			}
		} else if (var11 > var5) {
			arg1.x -= var9;
			if (arg1.x < var5) {
				arg1.x = var5;
			}
		}
		if (var4 < var6) {
			arg1.z += var9;
			if (arg1.z > var6) {
				arg1.z = var6;
			}
		} else if (var4 > var6) {
			arg1.z -= var9;
			if (arg1.z < var6) {
				arg1.z = var6;
			}
		}
		if (arg1.x == var5 && arg1.z == var6) {
			arg1.pathLength--;
		}
	}

	@ObfuscatedName("client.a(Lx;B)V")
	public final void updateFacingDirection(PathingEntity arg0, byte arg1) {
		if (arg1 != 117) {
			this.out.p1(89);
		}
		int var4;
		int var5;
		if (arg0.targetId != -1 && arg0.targetId < 32768) {
			NpcEntity var3 = this.npcs[arg0.targetId];
			if (var3 != null) {
				var4 = arg0.x - var3.x;
				var5 = arg0.z - var3.z;
				if (var4 != 0 || var5 != 0) {
					arg0.dstYaw = (int) (Math.atan2((double) var4, (double) var5) * 325.949D) & 0x7FF;
				}
			}
		}
		int var7;
		if (arg0.targetId >= 32768) {
			var7 = arg0.targetId - 32768;
			if (var7 == this.localPid) {
				var7 = this.LOCAL_PLAYER_INDEX;
			}
			PlayerEntity var8 = this.players[var7];
			if (var8 != null) {
				var5 = arg0.x - var8.x;
				int var6 = arg0.z - var8.z;
				if (var5 != 0 || var6 != 0) {
					arg0.dstYaw = (int) (Math.atan2((double) var5, (double) var6) * 325.949D) & 0x7FF;
				}
			}
		}
		if ((arg0.targetTileX != 0 || arg0.targetTileZ != 0) && (arg0.pathLength == 0 || arg0.seqTrigger > 0)) {
			var7 = arg0.x - (arg0.targetTileX - this.sceneBaseTileX - this.sceneBaseTileX) * 64;
			var4 = arg0.z - (arg0.targetTileZ - this.sceneBaseTileZ - this.sceneBaseTileZ) * 64;
			if (var7 != 0 || var4 != 0) {
				arg0.dstYaw = (int) (Math.atan2((double) var7, (double) var4) * 325.949D) & 0x7FF;
			}
			arg0.targetTileX = 0;
			arg0.targetTileZ = 0;
		}
		var7 = arg0.dstYaw - arg0.yaw & 0x7FF;
		if (var7 == 0) {
			return;
		}
		if (var7 < 32 || var7 > 2016) {
			arg0.yaw = arg0.dstYaw;
		} else if (var7 > 1024) {
			arg0.yaw -= 32;
		} else {
			arg0.yaw += 32;
		}
		arg0.yaw &= 0x7FF;
		if (arg0.secondarySeqId != arg0.seqStandId || arg0.yaw == arg0.dstYaw) {
			return;
		}
		if (arg0.seqTurnId != -1) {
			arg0.secondarySeqId = arg0.seqTurnId;
			return;
		}
		arg0.secondarySeqId = arg0.seqWalkId;
		return;
	}

	@ObfuscatedName("client.a(ZLx;)V")
	public final void updateSequences(boolean arg0, PathingEntity arg1) {
		if (!arg0) {
			return;
		}
		arg1.seqStretches = false;
		SeqType var3;
		if (arg1.secondarySeqId != -1) {
			var3 = SeqType.instances[arg1.secondarySeqId];
			arg1.secondarySeqCycle++;
			if (arg1.secondarySeqFrame < var3.frameCount && arg1.secondarySeqCycle > var3.delay[arg1.secondarySeqFrame]) {
				arg1.secondarySeqCycle = 0;
				arg1.secondarySeqFrame++;
			}
			if (arg1.secondarySeqFrame >= var3.frameCount) {
				arg1.secondarySeqCycle = 0;
				arg1.secondarySeqFrame = 0;
			}
		}
		if (arg1.primarySeqId != -1 && arg1.primarySeqDelay == 0) {
			var3 = SeqType.instances[arg1.primarySeqId];
			arg1.primarySeqCycle++;
			while (arg1.primarySeqFrame < var3.frameCount && arg1.primarySeqCycle > var3.delay[arg1.primarySeqFrame]) {
				arg1.primarySeqCycle -= var3.delay[arg1.primarySeqFrame];
				arg1.primarySeqFrame++;
			}
			if (arg1.primarySeqFrame >= var3.frameCount) {
				arg1.primarySeqFrame -= var3.replayoff;
				arg1.primarySeqLoop++;
				if (arg1.primarySeqLoop >= var3.replaycount) {
					arg1.primarySeqId = -1;
				}
				if (arg1.primarySeqFrame < 0 || arg1.primarySeqFrame >= var3.frameCount) {
					arg1.primarySeqId = -1;
				}
			}
			arg1.seqStretches = var3.stretches;
		}
		if (arg1.primarySeqDelay > 0) {
			arg1.primarySeqDelay--;
		}
		if (arg1.spotanimId == -1 || loopCycle < arg1.spotanimLastCycle) {
			return;
		}
		if (arg1.spotanimFrame < 0) {
			arg1.spotanimFrame = 0;
		}
		var3 = SpotAnimType.instances[arg1.spotanimId].seq;
		arg1.spotanimCycle++;
		while (arg1.spotanimFrame < var3.frameCount && arg1.spotanimCycle > var3.delay[arg1.spotanimFrame]) {
			arg1.spotanimCycle -= var3.delay[arg1.spotanimFrame];
			arg1.spotanimFrame++;
		}
		if (arg1.spotanimFrame >= var3.frameCount) {
			if (arg1.spotanimFrame < 0 || arg1.spotanimFrame >= var3.frameCount) {
				arg1.spotanimId = -1;
			}
			return;
		}
	}

	@ObfuscatedName("client.r(I)V")
	public final void drawGame(int arg0) {
		if (arg0 != 29510) {
			this.field1497 = 411;
		}
		if (this.redrawBackground) {
			this.redrawBackground = false;
			this.areaBackleft1.draw(11, super.graphics, 0, 5193);
			this.areaBackleft2.draw(375, super.graphics, 0, 5193);
			this.areaBackright1.draw(5, super.graphics, 729, 5193);
			this.areaBackright2.draw(231, super.graphics, 752, 5193);
			this.areaBacktop1.draw(0, super.graphics, 0, 5193);
			this.areaBacktop2.draw(0, super.graphics, 561, 5193);
			this.areaBackvmid1.draw(11, super.graphics, 520, 5193);
			this.areaBackvmid2.draw(231, super.graphics, 520, 5193);
			this.areaBackvmid3.draw(375, super.graphics, 501, 5193);
			this.areaBackhmid2.draw(345, super.graphics, 0, 5193);
			this.redrawSidebar = true;
			this.redrawChatback = true;
			this.redrawSideicons = true;
			this.redrawPrivacySettings = true;
			if (this.sceneState != 2) {
				this.areaViewport.draw(11, super.graphics, 8, 5193);
				this.areaMapback.draw(5, super.graphics, 561, 5193);
			}
		}
		if (this.sceneState == 2) {
			this.drawScene(0);
		}
		if (this.menuVisible && this.menuArea == 1) {
			this.redrawSidebar = true;
		}
		boolean var2;
		if (this.sidebarInterfaceId != -1) {
			var2 = this.updateInterfaceAnimation(this.sidebarInterfaceId, this.sceneDelta, 623);
			if (var2) {
				this.redrawSidebar = true;
			}
		}
		if (this.selectedArea == 2) {
			this.redrawSidebar = true;
		}
		if (this.objDragArea == 2) {
			this.redrawSidebar = true;
		}
		if (this.redrawSidebar) {
			this.drawSidebar((byte) 2);
			this.redrawSidebar = false;
		}
		if (this.chatInterfaceId == -1) {
			this.chatInterface.scrollPosition = this.chatScrollHeight - this.chatScrollOffset - 77;
			if (super.mouseX > 453 && super.mouseX < 565 && super.mouseY > 350) {
				this.handleScrollInput(super.mouseX - 22, 0, super.mouseY - 375, this.chatScrollHeight, 77, false, 463, 0, this.chatInterface);
			}
			int var3 = this.chatScrollHeight - 77 - this.chatInterface.scrollPosition;
			if (var3 < 0) {
				var3 = 0;
			}
			if (var3 > this.chatScrollHeight - 77) {
				var3 = this.chatScrollHeight - 77;
			}
			if (this.chatScrollOffset != var3) {
				this.chatScrollOffset = var3;
				this.redrawChatback = true;
			}
		}
		if (this.chatInterfaceId != -1) {
			var2 = this.updateInterfaceAnimation(this.chatInterfaceId, this.sceneDelta, 623);
			if (var2) {
				this.redrawChatback = true;
			}
		}
		if (this.selectedArea == 3) {
			this.redrawChatback = true;
		}
		if (this.objDragArea == 3) {
			this.redrawChatback = true;
		}
		if (this.modalMessage != null) {
			this.redrawChatback = true;
		}
		if (this.menuVisible && this.menuArea == 2) {
			this.redrawChatback = true;
		}
		if (this.redrawChatback) {
			this.drawChatback(false);
			this.redrawChatback = false;
		}
		if (this.sceneState == 2) {
			this.drawMinimap(this.field1300);
			this.areaMapback.draw(5, super.graphics, 561, 5193);
		}
		if (this.flashingTab != -1) {
			this.redrawSideicons = true;
		}
		if (this.redrawSideicons) {
			if (this.flashingTab != -1 && this.flashingTab == this.selectedTab) {
				this.flashingTab = -1;
				this.out.p1isaac((byte) -34, 175);
				this.out.p1(this.selectedTab);
			}
			this.redrawSideicons = false;
			this.areaBackhmid1.bind((byte) 62);
			this.imageBackhmid1.draw(0, 0, false);
			if (this.sidebarInterfaceId == -1) {
				if (this.tabInterfaceId[this.selectedTab] != -1) {
					if (this.selectedTab == 0) {
						this.imageRedstone1.draw(30, 29, false);
					}
					if (this.selectedTab == 1) {
						this.imageRedstone2.draw(29, 59, false);
					}
					if (this.selectedTab == 2) {
						this.imageRedstone2.draw(29, 87, false);
					}
					if (this.selectedTab == 3) {
						this.imageRedstone3.draw(29, 115, false);
					}
					if (this.selectedTab == 4) {
						this.imageRedstone2h.draw(29, 156, false);
					}
					if (this.selectedTab == 5) {
						this.imageRedstone2h.draw(29, 184, false);
					}
					if (this.selectedTab == 6) {
						this.imageRedstone1h.draw(30, 212, false);
					}
				}
				if (this.tabInterfaceId[0] != -1 && (this.flashingTab != 0 || loopCycle % 20 < 10)) {
					this.imageSideicons[0].draw(34, 35, false);
				}
				if (this.tabInterfaceId[1] != -1 && (this.flashingTab != 1 || loopCycle % 20 < 10)) {
					this.imageSideicons[1].draw(32, 59, false);
				}
				if (this.tabInterfaceId[2] != -1 && (this.flashingTab != 2 || loopCycle % 20 < 10)) {
					this.imageSideicons[2].draw(32, 86, false);
				}
				if (this.tabInterfaceId[3] != -1 && (this.flashingTab != 3 || loopCycle % 20 < 10)) {
					this.imageSideicons[3].draw(33, 121, false);
				}
				if (this.tabInterfaceId[4] != -1 && (this.flashingTab != 4 || loopCycle % 20 < 10)) {
					this.imageSideicons[4].draw(34, 157, false);
				}
				if (this.tabInterfaceId[5] != -1 && (this.flashingTab != 5 || loopCycle % 20 < 10)) {
					this.imageSideicons[5].draw(32, 185, false);
				}
				if (this.tabInterfaceId[6] != -1 && (this.flashingTab != 6 || loopCycle % 20 < 10)) {
					this.imageSideicons[6].draw(34, 212, false);
				}
			}
			this.areaBackhmid1.draw(165, super.graphics, 520, 5193);
			this.areaBackbase2.bind((byte) 62);
			this.imageBackbase2.draw(0, 0, false);
			if (this.sidebarInterfaceId == -1) {
				if (this.tabInterfaceId[this.selectedTab] != -1) {
					if (this.selectedTab == 7) {
						this.imageRedstone1v.draw(0, 49, false);
					}
					if (this.selectedTab == 8) {
						this.imageRedstone2v.draw(0, 81, false);
					}
					if (this.selectedTab == 9) {
						this.imageRedstone2v.draw(0, 108, false);
					}
					if (this.selectedTab == 10) {
						this.imageRedstone3v.draw(1, 136, false);
					}
					if (this.selectedTab == 11) {
						this.imageRedstone2hv.draw(0, 178, false);
					}
					if (this.selectedTab == 12) {
						this.imageRedstone2hv.draw(0, 205, false);
					}
					if (this.selectedTab == 13) {
						this.imageRedstone1hv.draw(0, 233, false);
					}
				}
				if (this.tabInterfaceId[8] != -1 && (this.flashingTab != 8 || loopCycle % 20 < 10)) {
					this.imageSideicons[7].draw(2, 80, false);
				}
				if (this.tabInterfaceId[9] != -1 && (this.flashingTab != 9 || loopCycle % 20 < 10)) {
					this.imageSideicons[8].draw(3, 107, false);
				}
				if (this.tabInterfaceId[10] != -1 && (this.flashingTab != 10 || loopCycle % 20 < 10)) {
					this.imageSideicons[9].draw(4, 142, false);
				}
				if (this.tabInterfaceId[11] != -1 && (this.flashingTab != 11 || loopCycle % 20 < 10)) {
					this.imageSideicons[10].draw(2, 179, false);
				}
				if (this.tabInterfaceId[12] != -1 && (this.flashingTab != 12 || loopCycle % 20 < 10)) {
					this.imageSideicons[11].draw(2, 206, false);
				}
				if (this.tabInterfaceId[13] != -1 && (this.flashingTab != 13 || loopCycle % 20 < 10)) {
					this.imageSideicons[12].draw(2, 230, false);
				}
			}
			this.areaBackbase2.draw(492, super.graphics, 501, 5193);
			this.areaViewport.bind((byte) 62);
		}
		if (this.redrawPrivacySettings) {
			this.redrawPrivacySettings = false;
			this.areaBackbase1.bind((byte) 62);
			this.imageBackbase1.draw(0, 0, false);
			this.fontPlain12.drawStringTaggableCenter(57, 16777215, true, 33, "Public chat", 0);
			if (this.publicChatSetting == 0) {
				this.fontPlain12.drawStringTaggableCenter(57, 65280, true, 46, "On", 0);
			}
			if (this.publicChatSetting == 1) {
				this.fontPlain12.drawStringTaggableCenter(57, 16776960, true, 46, "Friends", 0);
			}
			if (this.publicChatSetting == 2) {
				this.fontPlain12.drawStringTaggableCenter(57, 16711680, true, 46, "Off", 0);
			}
			if (this.publicChatSetting == 3) {
				this.fontPlain12.drawStringTaggableCenter(57, 65535, true, 46, "Hide", 0);
			}
			this.fontPlain12.drawStringTaggableCenter(186, 16777215, true, 33, "Private chat", 0);
			if (this.privateChatSetting == 0) {
				this.fontPlain12.drawStringTaggableCenter(186, 65280, true, 46, "On", 0);
			}
			if (this.privateChatSetting == 1) {
				this.fontPlain12.drawStringTaggableCenter(186, 16776960, true, 46, "Friends", 0);
			}
			if (this.privateChatSetting == 2) {
				this.fontPlain12.drawStringTaggableCenter(186, 16711680, true, 46, "Off", 0);
			}
			this.fontPlain12.drawStringTaggableCenter(326, 16777215, true, 33, "Trade/duel", 0);
			if (this.tradeChatSetting == 0) {
				this.fontPlain12.drawStringTaggableCenter(326, 65280, true, 46, "On", 0);
			}
			if (this.tradeChatSetting == 1) {
				this.fontPlain12.drawStringTaggableCenter(326, 16776960, true, 46, "Friends", 0);
			}
			if (this.tradeChatSetting == 2) {
				this.fontPlain12.drawStringTaggableCenter(326, 16711680, true, 46, "Off", 0);
			}
			this.fontPlain12.drawStringTaggableCenter(462, 16777215, true, 38, "Report abuse", 0);
			this.areaBackbase1.draw(471, super.graphics, 0, 5193);
			this.areaViewport.bind((byte) 62);
		}
		this.sceneDelta = 0;
	}

	@ObfuscatedName("client.e(II)Z")
	public final boolean isAddFriendOption(int arg0, int arg1) {
		if (arg1 < 0) {
			return false;
		}
		int var3 = this.menuAction[arg1];
		if (arg0 <= 0) {
			throw new NullPointerException();
		}
		if (var3 >= 2000) {
			var3 -= 2000;
		}
		return var3 == 406;
	}

	@ObfuscatedName("client.f(II)V")
	public final void useMenuOption(int arg0, int arg1) {
		if (arg1 < 0) {
			return;
		}
		if (this.chatbackInputOpen) {
			this.chatbackInputOpen = false;
			this.redrawChatback = true;
		}
		int var3 = this.menuParamB[arg1];
		int var4 = this.menuParamC[arg1];
		int var5 = this.menuAction[arg1];
		int var6 = this.menuParamA[arg1];
		if (var5 >= 2000) {
			var5 -= 2000;
		}
		String var7;
		int var8;
		String var9;
		int var11;
		if (var5 == 903 || var5 == 363) {
			var7 = this.menuOption[arg1];
			var8 = var7.indexOf("@whi@");
			if (var8 != -1) {
				var7 = var7.substring(var8 + 5).trim();
				var9 = JString.formatName(0, JString.fromBase37(JString.toBase37(var7), false));
				boolean var10 = false;
				for (var11 = 0; var11 < this.playerCount; var11++) {
					PlayerEntity var12 = this.players[this.playerIds[var11]];
					if (var12 != null && var12.name != null && var12.name.equalsIgnoreCase(var9)) {
						this.tryMove(this.localPlayer.pathTileX[0], 1, false, var12.pathTileX[0], this.localPlayer.pathTileZ[0], 0, 2, 1, var12.pathTileZ[0], 0, 0, 0);
						if (var5 == 903) {
							this.out.p1isaac((byte) -34, 206);
						}
						if (var5 == 363) {
							this.out.p1isaac((byte) -34, 164);
						}
						this.out.p2(this.playerIds[var11]);
						var10 = true;
						break;
					}
				}
				if (!var10) {
					this.addMessage(0, "Unable to find " + var9, (byte) 4, "");
				}
			}
		}
		if (var5 == 450 && this.interactWithLoc(75, var3, var4, var6, true)) {
			this.out.p2(this.objInterface);
			this.out.p2(this.objSelectedSlot);
			this.out.p2(this.objSelectedInterface);
		}
		if (var5 == 405 || var5 == 38 || var5 == 422 || var5 == 478 || var5 == 347) {
			if (var5 == 478) {
				if ((var3 & 0x3) == 0) {
					oplogic5++;
				}
				if (oplogic5 >= 90) {
					this.out.p1isaac((byte) -34, 220);
				}
				this.out.p1isaac((byte) -34, 157);
			}
			if (var5 == 347) {
				this.out.p1isaac((byte) -34, 211);
			}
			if (var5 == 422) {
				this.out.p1isaac((byte) -34, 133);
			}
			if (var5 == 405) {
				oplogic3 += var6;
				if (oplogic3 >= 97) {
					this.out.p1isaac((byte) -34, 30);
					this.out.p3(14953816);
				}
				this.out.p1isaac((byte) -34, 195);
			}
			if (var5 == 38) {
				this.out.p1isaac((byte) -34, 71);
			}
			this.out.p2(var6);
			this.out.p2(var3);
			this.out.p2(var4);
			this.selectedCycle = 0;
			this.selectedInterface = var4;
			this.selectedItem = var3;
			this.selectedArea = 2;
			if (Component.instances[var4].layer == this.viewportInterfaceId) {
				this.selectedArea = 1;
			}
			if (Component.instances[var4].layer == this.chatInterfaceId) {
				this.selectedArea = 3;
			}
		}
		NpcEntity var13;
		if (var5 == 728 || var5 == 542 || var5 == 6 || var5 == 963 || var5 == 245) {
			var13 = this.npcs[var6];
			if (var13 != null) {
				this.tryMove(this.localPlayer.pathTileX[0], 1, false, var13.pathTileX[0], this.localPlayer.pathTileZ[0], 0, 2, 1, var13.pathTileZ[0], 0, 0, 0);
				this.crossX = super.mouseClickX;
				this.crossY = super.mouseClickY;
				this.crossMode = 2;
				this.crossCycle = 0;
				if (var5 == 542) {
					this.out.p1isaac((byte) -34, 8);
				}
				if (var5 == 6) {
					if ((var6 & 0x3) == 0) {
						oplogic2++;
					}
					if (oplogic2 >= 124) {
						this.out.p1isaac((byte) -34, 88);
						this.out.p4(0);
					}
					this.out.p1isaac((byte) -34, 27);
				}
				if (var5 == 963) {
					this.out.p1isaac((byte) -34, 113);
				}
				if (var5 == 728) {
					this.out.p1isaac((byte) -34, 194);
				}
				if (var5 == 245) {
					if ((var6 & 0x3) == 0) {
						oplogic4++;
					}
					if (oplogic4 >= 85) {
						this.out.p1isaac((byte) -34, 176);
						this.out.p2(39596);
					}
					this.out.p1isaac((byte) -34, 100);
				}
				this.out.p2(var6);
			}
		}
		boolean var14;
		if (var5 == 217) {
			var14 = this.tryMove(this.localPlayer.pathTileX[0], 0, false, var3, this.localPlayer.pathTileZ[0], 0, 2, 0, var4, 0, 0, 0);
			if (!var14) {
				this.tryMove(this.localPlayer.pathTileX[0], 1, false, var3, this.localPlayer.pathTileZ[0], 0, 2, 1, var4, 0, 0, 0);
			}
			this.crossX = super.mouseClickX;
			this.crossY = super.mouseClickY;
			this.crossMode = 2;
			this.crossCycle = 0;
			this.out.p1isaac((byte) -34, 239);
			this.out.p2(var3 + this.sceneBaseTileX);
			this.out.p2(var4 + this.sceneBaseTileZ);
			this.out.p2(var6);
			this.out.p2(this.objInterface);
			this.out.p2(this.objSelectedSlot);
			this.out.p2(this.objSelectedInterface);
		}
		if (var5 == 1175) {
			int var15 = var6 >> 14 & 0x7FFF;
			LocType var16 = LocType.get(var15);
			if (var16.desc == null) {
				var9 = "It's a " + var16.name + ".";
			} else {
				var9 = new String(var16.desc);
			}
			this.addMessage(0, var9, (byte) 4, "");
		}
		if (var5 == 285) {
			this.interactWithLoc(245, var3, var4, var6, true);
		}
		if (var5 == 881) {
			this.out.p1isaac((byte) -34, 130);
			this.out.p2(var6);
			this.out.p2(var3);
			this.out.p2(var4);
			this.out.p2(this.objInterface);
			this.out.p2(this.objSelectedSlot);
			this.out.p2(this.objSelectedInterface);
			this.selectedCycle = 0;
			this.selectedInterface = var4;
			this.selectedItem = var3;
			this.selectedArea = 2;
			if (Component.instances[var4].layer == this.viewportInterfaceId) {
				this.selectedArea = 1;
			}
			if (Component.instances[var4].layer == this.chatInterfaceId) {
				this.selectedArea = 3;
			}
		}
		if (var5 == 391) {
			this.out.p1isaac((byte) -34, 48);
			this.out.p2(var6);
			this.out.p2(var3);
			this.out.p2(var4);
			this.out.p2(this.activeSpellId);
			this.selectedCycle = 0;
			this.selectedInterface = var4;
			this.selectedItem = var3;
			this.selectedArea = 2;
			if (Component.instances[var4].layer == this.viewportInterfaceId) {
				this.selectedArea = 1;
			}
			if (Component.instances[var4].layer == this.chatInterfaceId) {
				this.selectedArea = 3;
			}
		}
		if (var5 == 660) {
			if (this.menuVisible) {
				this.scene.click(4, var4 - 11, var3 - 8);
			} else {
				this.scene.click(4, super.mouseClickY - 11, super.mouseClickX - 8);
			}
		}
		if (var5 == 188) {
			this.objSelected = 1;
			this.objSelectedSlot = var3;
			this.objSelectedInterface = var4;
			this.objInterface = var6;
			this.objSelectedName = ObjType.get(var6).name;
			this.spellSelected = 0;
			return;
		}
		if (var5 == 44 && !this.pressedContinueOption) {
			this.out.p1isaac((byte) -34, 235);
			this.out.p2(var4);
			this.pressedContinueOption = true;
		}
		ObjType var17;
		String var18;
		if (var5 == 1773) {
			var17 = ObjType.get(var6);
			if (var4 >= 100000) {
				var18 = var4 + " x " + var17.name;
			} else if (var17.desc == null) {
				var18 = "It's a " + var17.name + ".";
			} else {
				var18 = new String(var17.desc);
			}
			this.addMessage(0, var18, (byte) 4, "");
		}
		if (var5 == 900) {
			var13 = this.npcs[var6];
			if (var13 != null) {
				this.tryMove(this.localPlayer.pathTileX[0], 1, false, var13.pathTileX[0], this.localPlayer.pathTileZ[0], 0, 2, 1, var13.pathTileZ[0], 0, 0, 0);
				this.crossX = super.mouseClickX;
				this.crossY = super.mouseClickY;
				this.crossMode = 2;
				this.crossCycle = 0;
				this.out.p1isaac((byte) -34, 202);
				this.out.p2(var6);
				this.out.p2(this.objInterface);
				this.out.p2(this.objSelectedSlot);
				this.out.p2(this.objSelectedInterface);
			}
		}
		PlayerEntity var19;
		if (var5 == 1373 || var5 == 1544 || var5 == 151 || var5 == 1101) {
			var19 = this.players[var6];
			if (var19 != null) {
				this.tryMove(this.localPlayer.pathTileX[0], 1, false, var19.pathTileX[0], this.localPlayer.pathTileZ[0], 0, 2, 1, var19.pathTileZ[0], 0, 0, 0);
				this.crossX = super.mouseClickX;
				this.crossY = super.mouseClickY;
				this.crossMode = 2;
				this.crossCycle = 0;
				if (var5 == 1101) {
					this.out.p1isaac((byte) -34, 164);
				}
				if (var5 == 151) {
					oplogic8++;
					if (oplogic8 >= 90) {
						this.out.p1isaac((byte) -34, 2);
						this.out.p2(31114);
					}
					this.out.p1isaac((byte) -34, 53);
				}
				if (var5 == 1373) {
					this.out.p1isaac((byte) -34, 206);
				}
				if (var5 == 1544) {
					this.out.p1isaac((byte) -34, 185);
				}
				this.out.p2(var6);
			}
		}
		if (var5 == 265) {
			var13 = this.npcs[var6];
			if (var13 != null) {
				this.tryMove(this.localPlayer.pathTileX[0], 1, false, var13.pathTileX[0], this.localPlayer.pathTileZ[0], 0, 2, 1, var13.pathTileZ[0], 0, 0, 0);
				this.crossX = super.mouseClickX;
				this.crossY = super.mouseClickY;
				this.crossMode = 2;
				this.crossCycle = 0;
				this.out.p1isaac((byte) -34, 134);
				this.out.p2(var6);
				this.out.p2(this.activeSpellId);
			}
		}
		long var20;
		if (var5 == 679) {
			var7 = this.menuOption[arg1];
			var8 = var7.indexOf("@whi@");
			if (var8 != -1) {
				var20 = JString.toBase37(var7.substring(var8 + 5).trim());
				var11 = -1;
				for (int var24 = 0; var24 < this.friendCount; var24++) {
					if (this.friendName37[var24] == var20) {
						var11 = var24;
						break;
					}
				}
				if (var11 != -1 && this.friendWorld[var11] > 0) {
					this.redrawChatback = true;
					this.chatbackInputOpen = false;
					this.showSocialInput = true;
					this.socialInput = "";
					this.socialAction = 3;
					this.socialName37 = this.friendName37[var11];
					this.socialMessage = "Enter message to send to " + this.friendName[var11];
				}
			}
		}
		if (var5 == 55 && this.interactWithLoc(9, var3, var4, var6, true)) {
			this.out.p2(this.activeSpellId);
		}
		if (var5 == 224 || var5 == 993 || var5 == 99 || var5 == 746 || var5 == 877) {
			var14 = this.tryMove(this.localPlayer.pathTileX[0], 0, false, var3, this.localPlayer.pathTileZ[0], 0, 2, 0, var4, 0, 0, 0);
			if (!var14) {
				this.tryMove(this.localPlayer.pathTileX[0], 1, false, var3, this.localPlayer.pathTileZ[0], 0, 2, 1, var4, 0, 0, 0);
			}
			this.crossX = super.mouseClickX;
			this.crossY = super.mouseClickY;
			this.crossMode = 2;
			this.crossCycle = 0;
			if (var5 == 224) {
				this.out.p1isaac((byte) -34, 140);
			}
			if (var5 == 746) {
				this.out.p1isaac((byte) -34, 178);
			}
			if (var5 == 877) {
				this.out.p1isaac((byte) -34, 247);
			}
			if (var5 == 99) {
				this.out.p1isaac((byte) -34, 200);
			}
			if (var5 == 993) {
				this.out.p1isaac((byte) -34, 40);
			}
			this.out.p2(var3 + this.sceneBaseTileX);
			this.out.p2(var4 + this.sceneBaseTileZ);
			this.out.p2(var6);
		}
		if (var5 == 1607) {
			var13 = this.npcs[var6];
			if (var13 != null) {
				if (var13.type.desc == null) {
					var18 = "It's a " + var13.type.name + ".";
				} else {
					var18 = new String(var13.type.desc);
				}
				this.addMessage(0, var18, (byte) 4, "");
			}
		}
		if (var5 == 504) {
			this.interactWithLoc(172, var3, var4, var6, true);
		}
		Component var22;
		if (var5 == 930) {
			var22 = Component.instances[var4];
			this.spellSelected = 1;
			this.activeSpellId = var4;
			this.activeSpellFlags = var22.actionTarget;
			this.objSelected = 0;
			var18 = var22.actionVerb;
			if (var18.indexOf(" ") != -1) {
				var18 = var18.substring(0, var18.indexOf(" "));
			}
			var9 = var22.actionVerb;
			if (var9.indexOf(" ") != -1) {
				var9 = var9.substring(var9.indexOf(" ") + 1);
			}
			this.spellCaption = var18 + " " + var22.action + " " + var9;
			if (this.activeSpellFlags == 16) {
				this.redrawSidebar = true;
				this.selectedTab = 3;
				this.redrawSideicons = true;
			}
			return;
		}
		if (var5 == 951) {
			var22 = Component.instances[var4];
			boolean var23 = true;
			if (var22.clientCode > 0) {
				var23 = this.handleInterfaceAction(false, var22);
			}
			if (var23) {
				this.out.p1isaac((byte) -34, 155);
				this.out.p2(var4);
			}
		}
		if (var5 == 602 || var5 == 596 || var5 == 22 || var5 == 892 || var5 == 415) {
			if (var5 == 22) {
				this.out.p1isaac((byte) -34, 212);
			}
			if (var5 == 415) {
				if ((var4 & 0x3) == 0) {
					oplogic7++;
				}
				if (oplogic7 >= 55) {
					this.out.p1isaac((byte) -34, 17);
					this.out.p4(0);
				}
				this.out.p1isaac((byte) -34, 6);
			}
			if (var5 == 602) {
				this.out.p1isaac((byte) -34, 31);
			}
			if (var5 == 892) {
				if ((var3 & 0x3) == 0) {
					oplogic9++;
				}
				if (oplogic9 >= 130) {
					this.out.p1isaac((byte) -34, 238);
					this.out.p1(177);
				}
				this.out.p1isaac((byte) -34, 38);
			}
			if (var5 == 596) {
				this.out.p1isaac((byte) -34, 59);
			}
			this.out.p2(var6);
			this.out.p2(var3);
			this.out.p2(var4);
			this.selectedCycle = 0;
			this.selectedInterface = var4;
			this.selectedItem = var3;
			this.selectedArea = 2;
			if (Component.instances[var4].layer == this.viewportInterfaceId) {
				this.selectedArea = 1;
			}
			if (Component.instances[var4].layer == this.chatInterfaceId) {
				this.selectedArea = 3;
			}
		}
		if (var5 == 581) {
			if ((var6 & 0x3) == 0) {
				oplogic1++;
			}
			if (oplogic1 >= 99) {
				this.out.p1isaac((byte) -34, 7);
				this.out.p4(0);
			}
			this.interactWithLoc(97, var3, var4, var6, true);
		}
		if (var5 == 965) {
			var14 = this.tryMove(this.localPlayer.pathTileX[0], 0, false, var3, this.localPlayer.pathTileZ[0], 0, 2, 0, var4, 0, 0, 0);
			if (!var14) {
				this.tryMove(this.localPlayer.pathTileX[0], 1, false, var3, this.localPlayer.pathTileZ[0], 0, 2, 1, var4, 0, 0, 0);
			}
			this.crossX = super.mouseClickX;
			this.crossY = super.mouseClickY;
			this.crossMode = 2;
			this.crossCycle = 0;
			this.out.p1isaac((byte) -34, 138);
			this.out.p2(var3 + this.sceneBaseTileX);
			this.out.p2(var4 + this.sceneBaseTileZ);
			this.out.p2(var6);
			this.out.p2(this.activeSpellId);
		}
		if (var5 == 1501) {
			oplogic6 += this.sceneBaseTileZ;
			if (oplogic6 >= 92) {
				this.out.p1isaac((byte) -34, 66);
				this.out.p4(0);
			}
			this.interactWithLoc(116, var3, var4, var6, true);
		}
		if (var5 == 364) {
			this.interactWithLoc(96, var3, var4, var6, true);
		}
		if (var5 == 1102) {
			var17 = ObjType.get(var6);
			if (var17.desc == null) {
				var18 = "It's a " + var17.name + ".";
			} else {
				var18 = new String(var17.desc);
			}
			this.addMessage(0, var18, (byte) 4, "");
		}
		if (var5 == 960) {
			this.out.p1isaac((byte) -34, 155);
			this.out.p2(var4);
			var22 = Component.instances[var4];
			if (var22.scripts != null && var22.scripts[0][0] == 5) {
				var8 = var22.scripts[0][1];
				if (this.varps[var8] != var22.scriptOperand[0]) {
					this.varps[var8] = var22.scriptOperand[0];
					this.updateVarp(var8, 49);
					this.redrawSidebar = true;
				}
			}
		}
		if (var5 == 34) {
			var7 = this.menuOption[arg1];
			var8 = var7.indexOf("@whi@");
			if (var8 != -1) {
				this.closeInterfaces((byte) -60);
				this.reportAbuseInput = var7.substring(var8 + 5).trim();
				this.reportAbuseMuteOption = false;
				for (int var21 = 0; var21 < Component.instances.length; var21++) {
					if (Component.instances[var21] != null && Component.instances[var21].clientCode == 600) {
						this.reportAbuseInterfaceID = this.viewportInterfaceId = Component.instances[var21].layer;
						break;
					}
				}
			}
		}
		if (var5 == 947) {
			this.closeInterfaces((byte) -60);
		}
		if (var5 == 367) {
			var19 = this.players[var6];
			if (var19 != null) {
				this.tryMove(this.localPlayer.pathTileX[0], 1, false, var19.pathTileX[0], this.localPlayer.pathTileZ[0], 0, 2, 1, var19.pathTileZ[0], 0, 0, 0);
				this.crossX = super.mouseClickX;
				this.crossY = super.mouseClickY;
				this.crossMode = 2;
				this.crossCycle = 0;
				this.out.p1isaac((byte) -34, 248);
				this.out.p2(var6);
				this.out.p2(this.objInterface);
				this.out.p2(this.objSelectedSlot);
				this.out.p2(this.objSelectedInterface);
			}
		}
		if (var5 == 465) {
			this.out.p1isaac((byte) -34, 155);
			this.out.p2(var4);
			var22 = Component.instances[var4];
			if (var22.scripts != null && var22.scripts[0][0] == 5) {
				var8 = var22.scripts[0][1];
				this.varps[var8] = 1 - this.varps[var8];
				this.updateVarp(var8, 49);
				this.redrawSidebar = true;
			}
		}
		if (var5 == 406 || var5 == 436 || var5 == 557 || var5 == 556) {
			var7 = this.menuOption[arg1];
			var8 = var7.indexOf("@whi@");
			if (var8 != -1) {
				var20 = JString.toBase37(var7.substring(var8 + 5).trim());
				if (var5 == 406) {
					this.addFriend(var20, -460);
				}
				if (var5 == 436) {
					this.addIgnore(var20, (byte) 3);
				}
				if (var5 == 557) {
					this.removeFriend(43808, var20);
				}
				if (var5 == 556) {
					this.removeIgnore(1, var20);
				}
			}
		}
		if (var5 == 651) {
			var19 = this.players[var6];
			if (var19 != null) {
				this.tryMove(this.localPlayer.pathTileX[0], 1, false, var19.pathTileX[0], this.localPlayer.pathTileZ[0], 0, 2, 1, var19.pathTileZ[0], 0, 0, 0);
				this.crossX = super.mouseClickX;
				this.crossY = super.mouseClickY;
				this.crossMode = 2;
				this.crossCycle = 0;
				this.out.p1isaac((byte) -34, 177);
				this.out.p2(var6);
				this.out.p2(this.activeSpellId);
			}
		}
		this.objSelected = 0;
		if (arg0 != 6412) {
			this.load();
		}
		this.spellSelected = 0;
	}

	@ObfuscatedName("client.b(IZI)Ljava/lang/String;")
	public static final String getCombatLevelColorTag(int arg0, boolean arg1, int arg2) {
		if (arg1) {
			throw new NullPointerException();
		}
		int var3 = arg0 - arg2;
		if (var3 < -9) {
			return "@red@";
		} else if (var3 < -6) {
			return "@or3@";
		} else if (var3 < -3) {
			return "@or2@";
		} else if (var3 < 0) {
			return "@or1@";
		} else if (var3 > 9) {
			return "@gre@";
		} else if (var3 > 6) {
			return "@gr3@";
		} else if (var3 > 3) {
			return "@gr2@";
		} else if (var3 > 0) {
			return "@gr1@";
		} else {
			return "@yel@";
		}
	}

	@ObfuscatedName("client.s(I)Ljava/lang/String;")
	public final String getHost(int arg0) {
		if (arg0 != -7437) {
			this.out.p1(216);
		}
		if (signlink.mainapp == null) {
			return super.frame == null ? super.getDocumentBase().getHost().toLowerCase() : "runescape.com";
		} else {
			return signlink.mainapp.getDocumentBase().getHost().toLowerCase();
		}
	}

	@ObfuscatedName("client.t(I)V")
	public final void drawMenu(int arg0) {
		int var2 = this.menuX;
		int var3 = this.menuY;
		int var4 = this.menuWidth;
		int var5 = this.menuHeight;
		int var6 = 6116423;
		Pix2D.fillRect(var3, var2, var6, (byte) 93, var4, var5);
		Pix2D.fillRect(var3 + 1, var2 + 1, 0, (byte) 93, var4 - 2, 16);
		Pix2D.drawRect(3, var2 + 1, 0, var5 - 19, var3 + 18, var4 - 2);
		this.fontBold12.drawString(var2 + 3, var3 + 14, false, var6, "Choose Option");
		int var7 = super.mouseX;
		int var8 = super.mouseY;
		if (this.menuArea == 0) {
			var7 -= 8;
			var8 -= 11;
		}
		if (this.menuArea == 1) {
			var7 -= 562;
			var8 -= 231;
		}
		if (this.menuArea == 2) {
			var7 -= 22;
			var8 -= 375;
		}
		for (int var9 = 0; var9 < this.menuSize; var9++) {
			int var10 = var3 + 31 + (this.menuSize - 1 - var9) * 15;
			int var11 = 16777215;
			if (var7 > var2 && var7 < var2 + var4 && var8 > var10 - 13 && var8 < var10 + 3) {
				var11 = 16776960;
			}
			this.fontBold12.drawStringTaggable(var2 + 3, 6, var10, this.menuOption[var9], true, var11);
		}
		if (arg0 >= 0) {
			this.packetType = this.in.g1();
		}
	}

	@ObfuscatedName("client.c(III)V")
	public final void handlePrivateChatInput(int arg0, int arg1, int arg2) {
		if (this.splitPrivateChat == 0) {
			return;
		}
		int var4 = 0;
		if (this.systemUpdateTimer != 0) {
			var4 = 1;
		}
		for (int var5 = 0; var5 < 100; var5++) {
			if (this.messageText[var5] != null) {
				int var6 = this.messageType[var5];
				if ((var6 == 3 || var6 == 7) && (var6 == 7 || this.privateChatSetting == 0 || this.privateChatSetting == 1 && this.isFriend(-20, this.messageSender[var5]))) {
					int var7 = 329 - var4 * 13;
					if (super.mouseX > 8 && super.mouseX < 520 && arg2 - 11 > var7 - 10 && arg2 - 11 <= var7 + 3) {
						if (this.rights) {
							this.menuOption[this.menuSize] = "Report abuse @whi@" + this.messageSender[var5];
							this.menuAction[this.menuSize] = 2034;
							this.menuSize++;
						}
						this.menuOption[this.menuSize] = "Add ignore @whi@" + this.messageSender[var5];
						this.menuAction[this.menuSize] = 2436;
						this.menuSize++;
						this.menuOption[this.menuSize] = "Add friend @whi@" + this.messageSender[var5];
						this.menuAction[this.menuSize] = 2406;
						this.menuSize++;
					}
					var4++;
					if (var4 >= 5) {
						return;
					}
				}
				if ((var6 == 5 || var6 == 6) && this.privateChatSetting < 2) {
					var4++;
					if (var4 >= 5) {
						return;
					}
				}
			}
		}
		if (arg1 != 27078) {
			this.field1152 = !this.field1152;
		}
	}

	@ObfuscatedName("client.a(ILhc;)V")
	public final void updateInterfaceContent(int arg0, Component arg1) {
		int var3 = arg1.clientCode;
		if (arg0 != 0) {
			field1323 = -68;
		}
		if (var3 >= 1 && var3 <= 100) {
			var3--;
			if (var3 >= this.friendCount) {
				arg1.text = "";
				arg1.buttonType = 0;
			} else {
				arg1.text = this.friendName[var3];
				arg1.buttonType = 1;
			}
		} else if (var3 >= 101 && var3 <= 200) {
			var3 -= 101;
			if (var3 >= this.friendCount) {
				arg1.text = "";
				arg1.buttonType = 0;
			} else {
				if (this.friendWorld[var3] == 0) {
					arg1.text = "@red@Offline";
				} else if (this.friendWorld[var3] == nodeId) {
					arg1.text = "@gre@World-" + (this.friendWorld[var3] - 9);
				} else {
					arg1.text = "@yel@World-" + (this.friendWorld[var3] - 9);
				}
				arg1.buttonType = 1;
			}
		} else if (var3 == 203) {
			arg1.scroll = this.friendCount * 15 + 20;
			if (arg1.scroll <= arg1.height) {
				arg1.scroll = arg1.height + 1;
			}
		} else if (var3 >= 401 && var3 <= 500) {
			var3 -= 401;
			if (var3 >= this.ignoreCount) {
				arg1.text = "";
				arg1.buttonType = 0;
			} else {
				arg1.text = JString.formatName(0, JString.fromBase37(this.ignoreName37[var3], false));
				arg1.buttonType = 1;
			}
		} else if (var3 == 503) {
			arg1.scroll = this.ignoreCount * 15 + 20;
			if (arg1.scroll <= arg1.height) {
				arg1.scroll = arg1.height + 1;
			}
		} else if (var3 == 327) {
			arg1.xan = 150;
			arg1.yan = (int) (Math.sin((double) loopCycle / 40.0D) * 256.0D) & 0x7FF;
			if (this.updateDesignModel) {
				this.updateDesignModel = false;
				Model[] var9 = new Model[7];
				int var5 = 0;
				for (int var6 = 0; var6 < 7; var6++) {
					int var7 = this.designIdentikits[var6];
					if (var7 >= 0) {
						var9[var5++] = IdkType.instances[var7].getModel();
					}
				}
				Model var10 = new Model(0, var9, var5);
				for (int var8 = 0; var8 < 5; var8++) {
					if (this.designColors[var8] != 0) {
						var10.recolor(field1307[var8][0], field1307[var8][this.designColors[var8]]);
						if (var8 == 1) {
							var10.recolor(field1438[0], field1438[this.designColors[var8]]);
						}
					}
				}
				var10.createLabelReferences(4);
				var10.applyTransform(-16599, SeqType.instances[this.localPlayer.seqStandId].frames[0]);
				var10.calculateNormals(64, 850, -30, -50, -30, true);
				arg1.model = var10;
			}
		} else if (var3 == 324) {
			if (this.genderButtonImage0 == null) {
				this.genderButtonImage0 = arg1.graphic;
				this.genderButtonImage1 = arg1.activeGraphic;
			}
			if (this.designGenderMale) {
				arg1.graphic = this.genderButtonImage1;
			} else {
				arg1.graphic = this.genderButtonImage0;
			}
		} else if (var3 == 325) {
			if (this.genderButtonImage0 == null) {
				this.genderButtonImage0 = arg1.graphic;
				this.genderButtonImage1 = arg1.activeGraphic;
			}
			if (this.designGenderMale) {
				arg1.graphic = this.genderButtonImage0;
			} else {
				arg1.graphic = this.genderButtonImage1;
			}
		} else if (var3 == 600) {
			arg1.text = this.reportAbuseInput;
			if (loopCycle % 20 < 10) {
				arg1.text = arg1.text + "|";
			} else {
				arg1.text = arg1.text + " ";
			}
		} else {
			if (var3 == 613) {
				if (!this.rights) {
					arg1.text = "";
				} else if (this.reportAbuseMuteOption) {
					arg1.colour = 16711680;
					arg1.text = "Moderator option: Mute player for 48 hours: <ON>";
				} else {
					arg1.colour = 16777215;
					arg1.text = "Moderator option: Mute player for 48 hours: <OFF>";
				}
			}
			String var4;
			if (var3 == 650 || var3 == 655) {
				if (this.lastAddress == 0) {
					arg1.text = "";
				} else {
					if (this.daysSinceLastLogin == 0) {
						var4 = "earlier today";
					} else if (this.daysSinceLastLogin == 1) {
						var4 = "yesterday";
					} else {
						var4 = this.daysSinceLastLogin + " days ago";
					}
					arg1.text = "You last logged in " + var4 + " from: " + signlink.dns;
				}
			}
			if (var3 == 651) {
				if (this.unreadMessages == 0) {
					arg1.text = "0 unread messages";
					arg1.colour = 16776960;
				}
				if (this.unreadMessages == 1) {
					arg1.text = "1 unread message";
					arg1.colour = 65280;
				}
				if (this.unreadMessages > 1) {
					arg1.text = this.unreadMessages + " unread messages";
					arg1.colour = 65280;
				}
			}
			if (var3 == 652) {
				if (this.daysSinceRecoveriesChanged == 201) {
					arg1.text = "";
				} else if (this.daysSinceRecoveriesChanged == 200) {
					arg1.text = "You have not yet set any password recovery questions.";
				} else {
					if (this.daysSinceRecoveriesChanged == 0) {
						var4 = "Earlier today";
					} else if (this.daysSinceRecoveriesChanged == 1) {
						var4 = "Yesterday";
					} else {
						var4 = this.daysSinceRecoveriesChanged + " days ago";
					}
					arg1.text = var4 + " you changed your recovery questions";
				}
			}
			if (var3 == 653) {
				if (this.daysSinceRecoveriesChanged == 201) {
					arg1.text = "";
				} else if (this.daysSinceRecoveriesChanged == 200) {
					arg1.text = "We strongly recommend you do so now to secure your account.";
				} else {
					arg1.text = "If you do not remember making this change then cancel it immediately";
				}
			}
			if (var3 == 654) {
				if (this.daysSinceRecoveriesChanged == 201) {
					arg1.text = "";
				} else if (this.daysSinceRecoveriesChanged == 200) {
					arg1.text = "Do this from the 'account management' area on our front webpage";
				} else {
					arg1.text = "Do this from the 'account management' area on our front webpage";
				}
			}
		}
	}

	@ObfuscatedName("client.a([BII)Z")
	public final boolean saveWave(byte[] arg0, int arg1, int arg2) {
		if (arg2 != 0) {
			this.packetType = -1;
		}
		return arg0 == null ? true : signlink.wavesave(arg0, arg1);
	}

	@ObfuscatedName("client.u(I)Z")
	public final boolean replayWave(int arg0) {
		if (arg0 <= 0) {
			this.out.p1(77);
		}
		return signlink.wavereplay();
	}

	@ObfuscatedName("client.g(II)V")
	public final void setWaveVolume(int arg0, int arg1) {
		if (arg1 != 0) {
			this.levelObjStacks = null;
		}
		signlink.wavevol = arg0;
	}

	@ObfuscatedName("client.a(ZLkb;I)V")
	public final void getNpcPosNewVis(boolean arg0, Packet arg1, int arg2) {
		if (arg0) {
			this.load();
		}
		while (arg1.bitPos + 21 < arg2 * 8) {
			int var4 = arg1.gBit(9, 13);
			if (var4 == 8191) {
				break;
			}
			if (this.npcs[var4] == null) {
				this.npcs[var4] = new NpcEntity();
			}
			NpcEntity var5 = this.npcs[var4];
			this.npcIds[this.npcCount++] = var4;
			var5.cycle = loopCycle;
			var5.type = NpcType.get(arg1.gBit(9, 11));
			var5.size = var5.type.size;
			var5.seqWalkId = var5.type.walkanim;
			var5.seqTurnAroundId = var5.type.walkanim_b;
			var5.seqTurnLeftId = var5.type.walkanim_r;
			var5.seqTurnRightId = var5.type.walkanim_l;
			var5.seqStandId = var5.type.readyanim;
			int var6 = arg1.gBit(9, 5);
			if (var6 > 15) {
				var6 -= 32;
			}
			int var7 = arg1.gBit(9, 5);
			if (var7 > 15) {
				var7 -= 32;
			}
			var5.teleport(false, false, this.localPlayer.pathTileX[0] + var6, this.localPlayer.pathTileZ[0] + var7);
			int var8 = arg1.gBit(9, 1);
			if (var8 == 1) {
				this.entityUpdateIds[this.entityUpdateCount++] = var4;
			}
		}
		arg1.accessBytes(this.field1178);
	}

	@ObfuscatedName("client.a(ZLhc;)Z")
	public final boolean handleInterfaceAction(boolean arg0, Component arg1) {
		int var3 = arg1.clientCode;
		if (arg0) {
			this.load();
		}
		if (var3 == 201) {
			this.redrawChatback = true;
			this.chatbackInputOpen = false;
			this.showSocialInput = true;
			this.socialInput = "";
			this.socialAction = 1;
			this.socialMessage = "Enter name of friend to add to list";
		}
		if (var3 == 202) {
			this.redrawChatback = true;
			this.chatbackInputOpen = false;
			this.showSocialInput = true;
			this.socialInput = "";
			this.socialAction = 2;
			this.socialMessage = "Enter name of friend to delete from list";
		}
		if (var3 == 205) {
			this.idleTimeout = 250;
			return true;
		}
		if (var3 == 501) {
			this.redrawChatback = true;
			this.chatbackInputOpen = false;
			this.showSocialInput = true;
			this.socialInput = "";
			this.socialAction = 4;
			this.socialMessage = "Enter name of player to add to list";
		}
		if (var3 == 502) {
			this.redrawChatback = true;
			this.chatbackInputOpen = false;
			this.showSocialInput = true;
			this.socialInput = "";
			this.socialAction = 5;
			this.socialMessage = "Enter name of player to delete from list";
		}
		int var4;
		int var5;
		int var6;
		if (var3 >= 300 && var3 <= 313) {
			var4 = (var3 - 300) / 2;
			var5 = var3 & 0x1;
			var6 = this.designIdentikits[var4];
			if (var6 != -1) {
				while (true) {
					if (var5 == 0) {
						var6--;
						if (var6 < 0) {
							var6 = IdkType.count - 1;
						}
					}
					if (var5 == 1) {
						var6++;
						if (var6 >= IdkType.count) {
							var6 = 0;
						}
					}
					if (!IdkType.instances[var6].disable && IdkType.instances[var6].type == var4 + (this.designGenderMale ? 0 : 7)) {
						this.designIdentikits[var4] = var6;
						this.updateDesignModel = true;
						break;
					}
				}
			}
		}
		if (var3 >= 314 && var3 <= 323) {
			var4 = (var3 - 314) / 2;
			var5 = var3 & 0x1;
			var6 = this.designColors[var4];
			if (var5 == 0) {
				var6--;
				if (var6 < 0) {
					var6 = field1307[var4].length - 1;
				}
			}
			if (var5 == 1) {
				var6++;
				if (var6 >= field1307[var4].length) {
					var6 = 0;
				}
			}
			this.designColors[var4] = var6;
			this.updateDesignModel = true;
		}
		if (var3 == 324 && !this.designGenderMale) {
			this.designGenderMale = true;
			this.validateCharacterDesign((byte) -6);
		}
		if (var3 == 325 && this.designGenderMale) {
			this.designGenderMale = false;
			this.validateCharacterDesign((byte) -6);
		}
		if (var3 == 326) {
			this.out.p1isaac((byte) -34, 52);
			this.out.p1(this.designGenderMale ? 0 : 1);
			for (var4 = 0; var4 < 7; var4++) {
				this.out.p1(this.designIdentikits[var4]);
			}
			for (var5 = 0; var5 < 5; var5++) {
				this.out.p1(this.designColors[var5]);
			}
			return true;
		}
		if (var3 == 613) {
			this.reportAbuseMuteOption = !this.reportAbuseMuteOption;
		}
		if (var3 >= 601 && var3 <= 612) {
			this.closeInterfaces((byte) -60);
			if (this.reportAbuseInput.length() > 0) {
				this.out.p1isaac((byte) -34, 190);
				this.out.p8(true, JString.toBase37(this.reportAbuseInput));
				this.out.p1(var3 - 601);
				this.out.p1(this.reportAbuseMuteOption ? 1 : 0);
			}
		}
		return false;
	}

	@ObfuscatedName("client.a()V")
	public final void load() {
		if (signlink.sunjava) {
			super.mindel = 5;
		}
		if (!lowMemory) {
			this.startMidiThread = true;
			this.midiThreadActive = true;
			this.startThread(this, 2);
			this.setMidi(false, 12345678, "scape_main", 40000);
		}
		if (started) {
			this.errorStarted = true;
			return;
		}
		started = true;
		boolean var1 = false;
		String var2 = this.getHost(-7437);
		if (var2.endsWith("jagex.com")) {
			var1 = true;
		}
		if (var2.endsWith("runescape.com")) {
			var1 = true;
		}
		if (var2.endsWith("192.168.1.2")) {
			var1 = true;
		}
		if (var2.endsWith("192.168.1.249")) {
			var1 = true;
		}
		if (var2.endsWith("192.168.1.252")) {
			var1 = true;
		}
		if (var2.endsWith("192.168.1.253")) {
			var1 = true;
		}
		if (var2.endsWith("192.168.1.254")) {
			var1 = true;
		}
		if (var2.endsWith("127.0.0.1")) {
			var1 = true;
		}
		if (!var1) {
			this.errorHost = true;
			return;
		}
		try {
			int var3 = 5;
			this.archiveChecksum[8] = 0;
			while (this.archiveChecksum[8] == 0) {
				this.drawProgress(true, "Connecting to fileserver", 10);
				try {
					DataInputStream var35 = this.openUrl("crc" + (int) (Math.random() * 9.9999999E7D));
					Packet var5 = new Packet(363, new byte[36]);
					var35.readFully(var5.data, 0, 36);
					for (int var6 = 0; var6 < 9; var6++) {
						this.archiveChecksum[var6] = var5.g4();
					}
					var35.close();
				} catch (IOException var33) {
					for (int var4 = var3; var4 > 0; var4--) {
						this.drawProgress(true, "Error loading - Will retry in " + var4 + " secs.", 10);
						try {
							Thread.sleep(1000L);
						} catch (Exception var28) {
						}
					}
					var3 *= 2;
					if (var3 > 60) {
						var3 = 60;
					}
				}
			}
			this.archiveTitle = this.loadArchive("title screen", this.archiveChecksum[1], "title", 10, 0);
			this.fontPlain11 = new PixFont(this.archiveTitle, "p11", 530);
			this.fontPlain12 = new PixFont(this.archiveTitle, "p12", 530);
			this.fontBold12 = new PixFont(this.archiveTitle, "b12", 530);
			this.fontQuill8 = new PixFont(this.archiveTitle, "q8", 530);
			this.loadTitleBackground((byte) 5);
			this.loadTitleImages(0);
			Jagfile var36 = this.loadArchive("config", this.archiveChecksum[2], "config", 15, 0);
			Jagfile var37 = this.loadArchive("interface", this.archiveChecksum[3], "interface", 20, 0);
			Jagfile var38 = this.loadArchive("2d graphics", this.archiveChecksum[4], "media", 30, 0);
			Jagfile var7 = this.loadArchive("3d graphics", this.archiveChecksum[5], "models", 40, 0);
			Jagfile var8 = this.loadArchive("textures", this.archiveChecksum[6], "textures", 60, 0);
			Jagfile var9 = this.loadArchive("chat system", this.archiveChecksum[7], "wordenc", 65, 0);
			Jagfile var10 = this.loadArchive("sound effects", this.archiveChecksum[8], "sounds", 70, 0);
			this.levelTileFlags = new byte[4][104][104];
			this.levelHeightmap = new int[4][105][105];
			this.scene = new World3D(415, this.levelHeightmap, 104, 4, 104);
			for (int var11 = 0; var11 < 4; var11++) {
				this.levelCollisionMap[var11] = new CollisionMap(104, -708, 104);
			}
			this.imageMinimap = new Pix32(512, 512);
			this.drawProgress(true, "Unpacking media", 75);
			this.imageInvback = new Pix8(var38, "invback", 0);
			this.imageChatback = new Pix8(var38, "chatback", 0);
			this.imageMapback = new Pix8(var38, "mapback", 0);
			this.imageBackbase1 = new Pix8(var38, "backbase1", 0);
			this.imageBackbase2 = new Pix8(var38, "backbase2", 0);
			this.imageBackhmid1 = new Pix8(var38, "backhmid1", 0);
			for (int var12 = 0; var12 < 13; var12++) {
				this.imageSideicons[var12] = new Pix8(var38, "sideicons", var12);
			}
			this.imageCompass = new Pix32(var38, "compass", 0);
			int var13;
			try {
				for (var13 = 0; var13 < 50; var13++) {
					this.imageMapscene[var13] = new Pix8(var38, "mapscene", var13);
				}
			} catch (Exception var32) {
			}
			try {
				for (var13 = 0; var13 < 50; var13++) {
					this.imageMapfunction[var13] = new Pix32(var38, "mapfunction", var13);
				}
			} catch (Exception var31) {
			}
			try {
				for (var13 = 0; var13 < 20; var13++) {
					this.imageHitmarks[var13] = new Pix32(var38, "hitmarks", var13);
				}
			} catch (Exception var30) {
			}
			try {
				for (var13 = 0; var13 < 20; var13++) {
					this.imageHeadicons[var13] = new Pix32(var38, "headicons", var13);
				}
			} catch (Exception var29) {
			}
			this.imageMapflag = new Pix32(var38, "mapflag", 0);
			for (var13 = 0; var13 < 8; var13++) {
				this.imageCrosses[var13] = new Pix32(var38, "cross", var13);
			}
			this.imageMapdot0 = new Pix32(var38, "mapdots", 0);
			this.imageMapdot1 = new Pix32(var38, "mapdots", 1);
			this.imageMapdot2 = new Pix32(var38, "mapdots", 2);
			this.imageMapdot3 = new Pix32(var38, "mapdots", 3);
			this.imageScrollbar0 = new Pix8(var38, "scrollbar", 0);
			this.imageScrollbar1 = new Pix8(var38, "scrollbar", 1);
			this.imageRedstone1 = new Pix8(var38, "redstone1", 0);
			this.imageRedstone2 = new Pix8(var38, "redstone2", 0);
			this.imageRedstone3 = new Pix8(var38, "redstone3", 0);
			this.imageRedstone1h = new Pix8(var38, "redstone1", 0);
			this.imageRedstone1h.flipHorizontally(-725);
			this.imageRedstone2h = new Pix8(var38, "redstone2", 0);
			this.imageRedstone2h.flipHorizontally(-725);
			this.imageRedstone1v = new Pix8(var38, "redstone1", 0);
			this.imageRedstone1v.flipVertically((byte) -74);
			this.imageRedstone2v = new Pix8(var38, "redstone2", 0);
			this.imageRedstone2v.flipVertically((byte) -74);
			this.imageRedstone3v = new Pix8(var38, "redstone3", 0);
			this.imageRedstone3v.flipVertically((byte) -74);
			this.imageRedstone1hv = new Pix8(var38, "redstone1", 0);
			this.imageRedstone1hv.flipHorizontally(-725);
			this.imageRedstone1hv.flipVertically((byte) -74);
			this.imageRedstone2hv = new Pix8(var38, "redstone2", 0);
			this.imageRedstone2hv.flipHorizontally(-725);
			this.imageRedstone2hv.flipVertically((byte) -74);
			Pix32 var14 = new Pix32(var38, "backleft1", 0);
			this.areaBackleft1 = new PixMap(this.getBaseComponent(field1481), var14.width, 299, var14.height);
			var14.blitOpaque(34676, 0, 0);
			Pix32 var39 = new Pix32(var38, "backleft2", 0);
			this.areaBackleft2 = new PixMap(this.getBaseComponent(field1481), var39.width, 299, var39.height);
			var39.blitOpaque(34676, 0, 0);
			Pix32 var40 = new Pix32(var38, "backright1", 0);
			this.areaBackright1 = new PixMap(this.getBaseComponent(field1481), var40.width, 299, var40.height);
			var40.blitOpaque(34676, 0, 0);
			Pix32 var41 = new Pix32(var38, "backright2", 0);
			this.areaBackright2 = new PixMap(this.getBaseComponent(field1481), var41.width, 299, var41.height);
			var41.blitOpaque(34676, 0, 0);
			Pix32 var42 = new Pix32(var38, "backtop1", 0);
			this.areaBacktop1 = new PixMap(this.getBaseComponent(field1481), var42.width, 299, var42.height);
			var42.blitOpaque(34676, 0, 0);
			Pix32 var43 = new Pix32(var38, "backtop2", 0);
			this.areaBacktop2 = new PixMap(this.getBaseComponent(field1481), var43.width, 299, var43.height);
			var43.blitOpaque(34676, 0, 0);
			Pix32 var44 = new Pix32(var38, "backvmid1", 0);
			this.areaBackvmid1 = new PixMap(this.getBaseComponent(field1481), var44.width, 299, var44.height);
			var44.blitOpaque(34676, 0, 0);
			Pix32 var45 = new Pix32(var38, "backvmid2", 0);
			this.areaBackvmid2 = new PixMap(this.getBaseComponent(field1481), var45.width, 299, var45.height);
			var45.blitOpaque(34676, 0, 0);
			Pix32 var46 = new Pix32(var38, "backvmid3", 0);
			this.areaBackvmid3 = new PixMap(this.getBaseComponent(field1481), var46.width, 299, var46.height);
			var46.blitOpaque(34676, 0, 0);
			Pix32 var47 = new Pix32(var38, "backhmid2", 0);
			this.areaBackhmid2 = new PixMap(this.getBaseComponent(field1481), var47.width, 299, var47.height);
			var47.blitOpaque(34676, 0, 0);
			int var15 = (int) (Math.random() * 21.0D) - 10;
			int var16 = (int) (Math.random() * 21.0D) - 10;
			int var17 = (int) (Math.random() * 21.0D) - 10;
			int var18 = (int) (Math.random() * 41.0D) - 20;
			for (int var19 = 0; var19 < 50; var19++) {
				if (this.imageMapfunction[var19] != null) {
					this.imageMapfunction[var19].translate(var15 + var18, var16 + var18, var17 + var18, true);
				}
				if (this.imageMapscene[var19] != null) {
					this.imageMapscene[var19].translate(var15 + var18, var16 + var18, var17 + var18, true);
				}
			}
			this.drawProgress(true, "Unpacking textures", 80);
			Pix3D.unpackTextures((byte) 2, var8);
			Pix3D.setBrightness(true, 0.8D);
			Pix3D.initPool(20, -20);
			this.drawProgress(true, "Unpacking models", 83);
			Model.unpack(field1323, var7);
			AnimBase.unpack(false, var7);
			AnimFrame.unpack(false, var7);
			this.drawProgress(true, "Unpacking config", 86);
			SeqType.unpack(var36, 473);
			LocType.unpack(var36);
			FloType.unpack(var36, 473);
			ObjType.unpack(var36);
			NpcType.unpack(var36);
			IdkType.unpack(var36, 473);
			SpotAnimType.unpack(var36, 473);
			VarpType.unpack(var36, 473);
			ObjType.membersWorld = members;
			if (!lowMemory) {
				this.drawProgress(true, "Unpacking sounds", 90);
				byte[] var20 = var10.read("sounds.dat", null, (byte) 2);
				Packet var21 = new Packet(363, var20);
				Wave.unpack(var21, 473);
			}
			this.drawProgress(true, "Unpacking interfaces", 92);
			PixFont[] var48 = new PixFont[] { this.fontPlain11, this.fontPlain12, this.fontBold12, this.fontQuill8 };
			Component.unpack(var38, var48, 30, var37);
			this.drawProgress(true, "Preparing game engine", 97);
			int var22;
			int var23;
			int var24;
			for (int var49 = 0; var49 < 33; var49++) {
				var22 = 999;
				var23 = 0;
				for (var24 = 0; var24 < 35; var24++) {
					if (this.imageMapback.pixels[var24 + var49 * this.imageMapback.width] == 0) {
						if (var22 == 999) {
							var22 = var24;
						}
					} else if (var22 != 999) {
						var23 = var24;
						break;
					}
				}
				this.compassMaskLineOffsets[var49] = var22;
				this.compassMaskLineLengths[var49] = var23 - var22;
			}
			int var25;
			for (var22 = 9; var22 < 160; var22++) {
				var23 = 999;
				var24 = 0;
				for (var25 = 10; var25 < 168; var25++) {
					if (this.imageMapback.pixels[var25 + var22 * this.imageMapback.width] == 0 && (var25 > 34 || var22 > 34)) {
						if (var23 == 999) {
							var23 = var25;
						}
					} else if (var23 != 999) {
						var24 = var25;
						break;
					}
				}
				this.minimapMaskLineOffsets[var22 - 9] = var23 - 21;
				this.minimapMaskLineLengths[var22 - 9] = var24 - var23;
			}
			Pix3D.init3D(96, 479, 0);
			this.areaChatbackOffsets = Pix3D.lineOffset;
			Pix3D.init3D(261, 190, 0);
			this.areaSidebarOffsets = Pix3D.lineOffset;
			Pix3D.init3D(334, 512, 0);
			this.areaViewportOffsets = Pix3D.lineOffset;
			int[] var50 = new int[9];
			for (var24 = 0; var24 < 9; var24++) {
				var25 = var24 * 32 + 128 + 15;
				int var26 = var25 * 3 + 600;
				int var27 = Pix3D.sinTable[var25];
				var50[var24] = var26 * var27 >> 16;
			}
			World3D.init(var50, 800, 512, field1236, 334, 500);
			WordFilter.unpack(var9);
		} catch (Exception var34) {
			this.errorLoading = true;
		}
	}

	@ObfuscatedName("client.v(I)V")
	public final void handleInput(int arg0) {
		if (this.objDragArea != 0) {
			return;
		}
		this.menuOption[0] = "Cancel";
		this.menuAction[0] = 1252;
		this.menuSize = 1;
		this.handlePrivateChatInput(super.mouseX, 27078, super.mouseY);
		this.lastHoveredInterfaceId = 0;
		int var6 = 12 / arg0;
		if (super.mouseX > 8 && super.mouseY > 11 && super.mouseX < 520 && super.mouseY < 345) {
			if (this.viewportInterfaceId == -1) {
				this.handleViewportOptions((byte) 2);
			} else {
				this.handleInterfaceInput(super.mouseY, super.mouseX, 11, Component.instances[this.viewportInterfaceId], 5082, 8, 0);
			}
		}
		if (this.lastHoveredInterfaceId != this.viewportHoveredInterfaceIndex) {
			this.viewportHoveredInterfaceIndex = this.lastHoveredInterfaceId;
		}
		this.lastHoveredInterfaceId = 0;
		if (super.mouseX > 562 && super.mouseY > 231 && super.mouseX < 752 && super.mouseY < 492) {
			if (this.sidebarInterfaceId != -1) {
				this.handleInterfaceInput(super.mouseY, super.mouseX, 231, Component.instances[this.sidebarInterfaceId], 5082, 562, 0);
			} else if (this.tabInterfaceId[this.selectedTab] != -1) {
				this.handleInterfaceInput(super.mouseY, super.mouseX, 231, Component.instances[this.tabInterfaceId[this.selectedTab]], 5082, 562, 0);
			}
		}
		if (this.lastHoveredInterfaceId != this.sidebarHoveredInterfaceIndex) {
			this.redrawSidebar = true;
			this.sidebarHoveredInterfaceIndex = this.lastHoveredInterfaceId;
		}
		this.lastHoveredInterfaceId = 0;
		if (super.mouseX > 22 && super.mouseY > 375 && super.mouseX < 431 && super.mouseY < 471) {
			if (this.chatInterfaceId == -1) {
				this.handleChatMouseInput(super.mouseY - 375, 0, super.mouseX - 22);
			} else {
				this.handleInterfaceInput(super.mouseY, super.mouseX, 375, Component.instances[this.chatInterfaceId], 5082, 22, 0);
			}
		}
		if (this.chatInterfaceId != -1 && this.lastHoveredInterfaceId != this.chatHoveredInterfaceIndex) {
			this.redrawChatback = true;
			this.chatHoveredInterfaceIndex = this.lastHoveredInterfaceId;
		}
		boolean var2 = false;
		while (!var2) {
			var2 = true;
			for (int var3 = 0; var3 < this.menuSize - 1; var3++) {
				if (this.menuAction[var3] < 1000 && this.menuAction[var3 + 1] > 1000) {
					String var4 = this.menuOption[var3];
					this.menuOption[var3] = this.menuOption[var3 + 1];
					this.menuOption[var3 + 1] = var4;
					int var5 = this.menuAction[var3];
					this.menuAction[var3] = this.menuAction[var3 + 1];
					this.menuAction[var3 + 1] = var5;
					int var7 = this.menuParamB[var3];
					this.menuParamB[var3] = this.menuParamB[var3 + 1];
					this.menuParamB[var3 + 1] = var7;
					int var8 = this.menuParamC[var3];
					this.menuParamC[var3] = this.menuParamC[var3 + 1];
					this.menuParamC[var3 + 1] = var8;
					int var9 = this.menuParamA[var3];
					this.menuParamA[var3] = this.menuParamA[var3 + 1];
					this.menuParamA[var3 + 1] = var9;
					var2 = false;
				}
			}
		}
	}

	@ObfuscatedName("client.h(B)V")
	public final void clearCaches(byte arg0) {
		LocType.modelCacheStatic.clear();
		LocType.modelCacheDynamic.clear();
		NpcType.modelCache.clear();
		ObjType.modelCache.clear();
		ObjType.iconCache.clear();
		PlayerEntity.modelCache.clear();
		SpotAnimType.modelCache.clear();
		if (arg0 == this.field1208) {
			boolean var2 = false;
		} else {
			this.field1512 = !this.field1512;
		}
	}

	@ObfuscatedName("client.w(I)V")
	public final void draw3DEntityElements(int arg0) {
		int var2;
		if (arg0 != this.field1121) {
			for (var2 = 1; var2 > 0; var2++) {
			}
		}
		this.drawPrivateMessages(222);
		if (this.crossMode == 1) {
			this.imageCrosses[this.crossCycle / 100].draw(this.crossY - 8 - 11, this.crossX - 8 - 8, false);
		}
		if (this.crossMode == 2) {
			this.imageCrosses[this.crossCycle / 100 + 4].draw(this.crossY - 8 - 11, this.crossX - 8 - 8, false);
		}
		if (this.viewportInterfaceId != -1) {
			this.updateInterfaceAnimation(this.viewportInterfaceId, this.sceneDelta, 623);
			this.drawInterface(0, 0, 38682, Component.instances[this.viewportInterfaceId], 0);
		}
		this.drawWildyLevel(39734);
		if (!this.menuVisible) {
			this.handleInput(26);
			this.drawTooltip(true);
		} else if (this.menuArea == 0) {
			this.drawMenu(-961);
		}
		if (this.inMultizone == 1) {
			if (this.wildernessLevel > 0 || this.worldLocationState == 1) {
				this.imageHeadicons[1].draw(258, 472, false);
			} else {
				this.imageHeadicons[1].draw(296, 472, false);
			}
		}
		if (this.wildernessLevel > 0) {
			this.imageHeadicons[0].draw(296, 472, false);
			this.fontPlain12.drawStringCenter(329, (byte) 6, 16776960, "Level: " + this.wildernessLevel, 484);
		}
		if (this.worldLocationState == 1) {
			this.imageHeadicons[6].draw(296, 472, false);
			this.fontPlain12.drawStringCenter(329, (byte) 6, 16776960, "Arena", 484);
		}
		if (this.systemUpdateTimer == 0) {
			return;
		}
		var2 = this.systemUpdateTimer / 50;
		int var3 = var2 / 60;
		var2 %= 60;
		if (var2 < 10) {
			this.fontPlain12.drawString(4, 329, false, 16776960, "System update in: " + var3 + ":0" + var2);
		} else {
			this.fontPlain12.drawString(4, 329, false, 16776960, "System update in: " + var3 + ":" + var2);
		}
	}

	@ObfuscatedName("client.x(I)V")
	public final void updateOrbitCamera(int arg0) {
		int var2 = this.localPlayer.x + this.cameraAnticheatOffsetX;
		int var3 = this.localPlayer.z + this.cameraAnticheatOffsetZ;
		if (this.orbitCameraX - var2 < -500 || this.orbitCameraX - var2 > 500 || this.orbitCameraZ - var3 < -500 || this.orbitCameraZ - var3 > 500) {
			this.orbitCameraX = var2;
			this.orbitCameraZ = var3;
		}
		if (this.orbitCameraX != var2) {
			this.orbitCameraX += (var2 - this.orbitCameraX) / 16;
		}
		if (this.orbitCameraZ != var3) {
			this.orbitCameraZ += (var3 - this.orbitCameraZ) / 16;
		}
		if (super.actionKey[1] == 1) {
			this.orbitCameraYawVelocity += (-24 - this.orbitCameraYawVelocity) / 2;
		} else if (super.actionKey[2] == 1) {
			this.orbitCameraYawVelocity += (24 - this.orbitCameraYawVelocity) / 2;
		} else {
			this.orbitCameraYawVelocity /= 2;
		}
		if (super.actionKey[3] == 1) {
			this.orbitCameraPitchVelocity += (12 - this.orbitCameraPitchVelocity) / 2;
		} else if (super.actionKey[4] == 1) {
			this.orbitCameraPitchVelocity += (-12 - this.orbitCameraPitchVelocity) / 2;
		} else {
			this.orbitCameraPitchVelocity /= 2;
		}
		this.orbitCameraYaw = this.orbitCameraYaw + this.orbitCameraYawVelocity / 2 & 0x7FF;
		this.packetSize += arg0;
		this.orbitCameraPitch += this.orbitCameraPitchVelocity / 2;
		if (this.orbitCameraPitch < 128) {
			this.orbitCameraPitch = 128;
		}
		if (this.orbitCameraPitch > 383) {
			this.orbitCameraPitch = 383;
		}
		int var4 = this.orbitCameraX >> 7;
		int var5 = this.orbitCameraZ >> 7;
		int var6 = this.getHeightmapY(this.currentLevel, this.orbitCameraX, (byte) 5, this.orbitCameraZ);
		int var7 = 0;
		int var8;
		if (var4 > 3 && var5 > 3 && var4 < 100 && var5 < 100) {
			for (var8 = var4 - 4; var8 <= var4 + 4; var8++) {
				for (int var9 = var5 - 4; var9 <= var5 + 4; var9++) {
					int var10 = this.currentLevel;
					if (var10 < 3 && (this.levelTileFlags[1][var8][var9] & 0x2) == 2) {
						var10++;
					}
					int var11 = var6 - this.levelHeightmap[var10][var8][var9];
					if (var11 > var7) {
						var7 = var11;
					}
				}
			}
		}
		var8 = var7 * 192;
		if (var8 > 98048) {
			var8 = 98048;
		}
		if (var8 < 32768) {
			var8 = 32768;
		}
		if (var8 > this.cameraPitchClamp) {
			this.cameraPitchClamp += (var8 - this.cameraPitchClamp) / 24;
		} else if (var8 < this.cameraPitchClamp) {
			this.cameraPitchClamp += (var8 - this.cameraPitchClamp) / 80;
		}
	}

	@ObfuscatedName("client.i(B)V")
	public final void pushProjectiles(byte arg0) {
		if (arg0 != -26) {
			this.packetType = -1;
		}
		for (ProjectileEntity var2 = (ProjectileEntity) this.projectiles.head(); var2 != null; var2 = (ProjectileEntity) this.projectiles.next(551)) {
			if (var2.level != this.currentLevel || loopCycle > var2.lastCycle) {
				var2.unlink();
			} else if (loopCycle >= var2.startCycle) {
				if (var2.target > 0) {
					NpcEntity var3 = this.npcs[var2.target - 1];
					if (var3 != null) {
						var2.updateVelocity(this.getHeightmapY(var2.level, var3.x, (byte) 5, var3.z) - var2.offsetY, var3.z, var3.x, -855, loopCycle);
					}
				}
				if (var2.target < 0) {
					int var4 = -var2.target - 1;
					PlayerEntity var5;
					if (var4 == this.localPid) {
						var5 = this.localPlayer;
					} else {
						var5 = this.players[var4];
					}
					if (var5 != null) {
						var2.updateVelocity(this.getHeightmapY(var2.level, var5.x, (byte) 5, var5.z) - var2.offsetY, var5.z, var5.x, -855, loopCycle);
					}
				}
				var2.update((byte) -30, this.sceneDelta);
				this.scene.addTemporary(-44713, (int) var2.z, 60, var2.yaw, (int) var2.x, -1, false, null, var2, (int) var2.y, this.currentLevel);
			}
		}
	}

	@ObfuscatedName("client.c(I)V")
	public final void refresh(int arg0) {
		if (arg0 != 3) {
			field1235 = !field1235;
		}
		this.redrawBackground = true;
	}

	@ObfuscatedName("client.a(IILhb;I)V")
	public final void drawOnMinimap(int arg0, int arg1, Pix32 arg2, int arg3) {
		int var5 = this.orbitCameraYaw + this.minimapAnticheatAngle & 0x7FF;
		int var6 = arg3 * arg3 + arg0 * arg0;
		if (arg1 != 4) {
			this.field1225 = !this.field1225;
		}
		if (var6 > 6400) {
			return;
		}
		int var7 = Model.sin[var5];
		int var8 = Model.cos[var5];
		int var11 = var7 * 256 / (this.minimapZoom + 256);
		int var12 = var8 * 256 / (this.minimapZoom + 256);
		int var9 = arg0 * var11 + arg3 * var12 >> 16;
		int var10 = arg0 * var12 - arg3 * var11 >> 16;
		if (var6 > 2500) {
			arg2.drawMasked(this.imageMapback, 83 - var10 - arg2.cropH / 2, var9 + 94 - arg2.cropW / 2, (byte) -15);
		} else {
			arg2.draw(83 - var10 - arg2.cropH / 2, var9 + 94 - arg2.cropW / 2, false);
		}
	}

	@ObfuscatedName("client.a(IIII)I")
	public final int mix(int arg0, int arg1, int arg2, int arg3) {
		int var5 = 256 - arg1;
		if (arg3 <= 0) {
			field1235 = !field1235;
		}
		return ((arg0 & 0xFF00FF) * var5 + (arg2 & 0xFF00FF) * arg1 & 0xFF00FF00) + ((arg0 & 0xFF00) * var5 + (arg2 & 0xFF00) * arg1 & 0xFF0000) >> 8;
	}

	@ObfuscatedName("client.h(II)Ljava/lang/String;")
	public final String getIntString(int arg0, int arg1) {
		if (arg1 >= 0) {
			this.field1497 = -430;
		}
		return arg0 < 999999999 ? String.valueOf(arg0) : "*";
	}

	@ObfuscatedName("client.a(IZLx;)V")
	public final void projectFromGround(int arg0, boolean arg1, PathingEntity arg2) {
		this.projectFromGround(arg2.z, arg2.x, this.field1470, arg0);
		if (arg1) {
			this.packetType = -1;
		}
	}

	@ObfuscatedName("client.b(IIII)V")
	public final void projectFromGround(int arg0, int arg1, int arg2, int arg3) {
		if (arg1 < 128 || arg0 < 128 || arg1 > 13056 || arg0 > 13056) {
			this.projectX = -1;
			this.projectY = -1;
			return;
		}
		int var5 = this.getHeightmapY(this.currentLevel, arg1, (byte) 5, arg0) - arg3;
		int var13 = arg1 - this.cameraX;
		int var14 = var5 - this.cameraY;
		int var11 = arg0 - this.cameraZ;
		int var6 = Model.sin[this.cameraPitch];
		int var7 = Model.cos[this.cameraPitch];
		int var8 = Model.sin[this.cameraYaw];
		int var9 = Model.cos[this.cameraYaw];
		int var10 = var11 * var8 + var13 * var9 >> 16;
		int var12 = var11 * var9 - var13 * var8 >> 16;
		if (arg2 >= 0) {
			this.out.p1(131);
		}
		var13 = var10;
		var10 = var14 * var7 - var12 * var6 >> 16;
		var11 = var14 * var6 + var12 * var7 >> 16;
		if (var11 >= 50) {
			this.projectX = Pix3D.centerW3D + (var13 << 9) / var11;
			this.projectY = Pix3D.centerH3D + (var10 << 9) / var11;
		} else {
			this.projectX = -1;
			this.projectY = -1;
		}
	}

	@ObfuscatedName("client.a(IIIIZ)Z")
	public final boolean interactWithLoc(int arg0, int arg1, int arg2, int arg3, boolean arg4) {
		int var6 = arg3 >> 14 & 0x7FFF;
		int var7 = this.scene.getInfo(this.currentLevel, arg1, arg2, arg3);
		if (var7 == -1) {
			return false;
		}
		int var8 = var7 & 0x1F;
		int var9 = var7 >> 6 & 0x3;
		if (var8 == 10 || var8 == 11 || var8 == 22) {
			LocType var10 = LocType.get(var6);
			int var11;
			int var12;
			if (var9 == 0 || var9 == 2) {
				var11 = var10.width;
				var12 = var10.length;
			} else {
				var11 = var10.length;
				var12 = var10.width;
			}
			int var13 = var10.forceapproach;
			if (var9 != 0) {
				var13 = (var13 << var9 & 0xF) + (var13 >> 4 - var9);
			}
			this.tryMove(this.localPlayer.pathTileX[0], var11, false, arg1, this.localPlayer.pathTileZ[0], 0, 2, var12, arg2, 0, 0, var13);
		} else {
			this.tryMove(this.localPlayer.pathTileX[0], 0, false, arg1, this.localPlayer.pathTileZ[0], 0, 2, 0, arg2, var9, var8 + 1, 0);
		}
		this.crossX = super.mouseClickX;
		this.crossY = super.mouseClickY;
		this.crossMode = 2;
		this.crossCycle = 0;
		this.out.p1isaac((byte) -34, arg0);
		this.out.p2(arg1 + this.sceneBaseTileX);
		this.out.p2(arg2 + this.sceneBaseTileZ);
		this.out.p2(var6);
		if (!arg4) {
			throw new NullPointerException();
		}
		return true;
	}

	@ObfuscatedName("client.y(I)V")
	public final void showContextMenu(int arg0) {
		int var2 = this.fontBold12.stringWidth(false, "Choose Option");
		int var4;
		for (int var3 = 0; var3 < this.menuSize; var3++) {
			var4 = this.fontBold12.stringWidth(false, this.menuOption[var3]);
			if (var4 > var2) {
				var2 = var4;
			}
		}
		var2 += 8;
		var4 = this.menuSize * 15 + 21;
		if (arg0 >= 0) {
			return;
		}
		int var5;
		int var6;
		if (super.mouseClickX > 8 && super.mouseClickY > 11 && super.mouseClickX < 520 && super.mouseClickY < 345) {
			var5 = super.mouseClickX - 8 - var2 / 2;
			if (var5 + var2 > 512) {
				var5 = 512 - var2;
			}
			if (var5 < 0) {
				var5 = 0;
			}
			var6 = super.mouseClickY - 11;
			if (var6 + var4 > 334) {
				var6 = 334 - var4;
			}
			if (var6 < 0) {
				var6 = 0;
			}
			this.menuVisible = true;
			this.menuArea = 0;
			this.menuX = var5;
			this.menuY = var6;
			this.menuWidth = var2;
			this.menuHeight = this.menuSize * 15 + 22;
		}
		if (super.mouseClickX > 562 && super.mouseClickY > 231 && super.mouseClickX < 752 && super.mouseClickY < 492) {
			var5 = super.mouseClickX - 562 - var2 / 2;
			if (var5 < 0) {
				var5 = 0;
			} else if (var5 + var2 > 190) {
				var5 = 190 - var2;
			}
			var6 = super.mouseClickY - 231;
			if (var6 < 0) {
				var6 = 0;
			} else if (var6 + var4 > 261) {
				var6 = 261 - var4;
			}
			this.menuVisible = true;
			this.menuArea = 1;
			this.menuX = var5;
			this.menuY = var6;
			this.menuWidth = var2;
			this.menuHeight = this.menuSize * 15 + 22;
		}
		if (super.mouseClickX <= 22 || super.mouseClickY <= 375 || super.mouseClickX >= 501 || super.mouseClickY >= 471) {
			return;
		}
		var5 = super.mouseClickX - 22 - var2 / 2;
		if (var5 < 0) {
			var5 = 0;
		} else if (var5 + var2 > 479) {
			var5 = 479 - var2;
		}
		var6 = super.mouseClickY - 375;
		if (var6 < 0) {
			var6 = 0;
		} else if (var6 + var4 > 96) {
			var6 = 96 - var4;
		}
		this.menuVisible = true;
		this.menuArea = 2;
		this.menuX = var5;
		this.menuY = var6;
		this.menuWidth = var2;
		this.menuHeight = this.menuSize * 15 + 22;
	}

	@ObfuscatedName("client.a(Ljava/lang/String;)Ljava/io/DataInputStream;")
	public final DataInputStream openUrl(String arg0) throws IOException {
		return signlink.mainapp == null ? new DataInputStream((new URL(this.getCodeBase(), arg0)).openStream()) : signlink.openurl(arg0);
	}

	@ObfuscatedName("client.j(B)V")
	public final void loadTitle(byte arg0) {
		if (this.imageTitle2 != null) {
			return;
		}
		super.drawArea = null;
		this.areaChatback = null;
		this.areaMapback = null;
		this.areaSidebar = null;
		this.areaViewport = null;
		this.areaBackbase1 = null;
		this.areaBackbase2 = null;
		this.areaBackhmid1 = null;
		this.imageTitle0 = new PixMap(this.getBaseComponent(field1481), 128, 299, 265);
		Pix2D.clear(this.field1508);
		this.imageTitle1 = new PixMap(this.getBaseComponent(field1481), 128, 299, 265);
		Pix2D.clear(this.field1508);
		this.imageTitle2 = new PixMap(this.getBaseComponent(field1481), 533, 299, 186);
		Pix2D.clear(this.field1508);
		this.imageTitle3 = new PixMap(this.getBaseComponent(field1481), 360, 299, 146);
		Pix2D.clear(this.field1508);
		this.imageTitle4 = new PixMap(this.getBaseComponent(field1481), 360, 299, 200);
		Pix2D.clear(this.field1508);
		this.imageTitle5 = new PixMap(this.getBaseComponent(field1481), 214, 299, 267);
		Pix2D.clear(this.field1508);
		this.imageTitle6 = new PixMap(this.getBaseComponent(field1481), 215, 299, 267);
		Pix2D.clear(this.field1508);
		this.imageTitle7 = new PixMap(this.getBaseComponent(field1481), 86, 299, 79);
		if (arg0 != this.field1231) {
			this.out.p1(73);
		}
		Pix2D.clear(this.field1508);
		this.imageTitle8 = new PixMap(this.getBaseComponent(field1481), 87, 299, 79);
		Pix2D.clear(this.field1508);
		if (this.archiveTitle != null) {
			this.loadTitleBackground((byte) 5);
			this.loadTitleImages(0);
		}
		this.redrawBackground = true;
	}

	@ObfuscatedName("client.z(I)V")
	public final void runFlames(int arg0) {
		this.flameThread = true;
		try {
			long var2 = System.currentTimeMillis();
			int var4 = 0;
			int var5 = 20;
			while (this.flameActive) {
				this.updateFlames(true);
				this.updateFlames(true);
				this.drawFlames(true);
				var4++;
				if (var4 > 10) {
					long var6 = System.currentTimeMillis();
					int var8 = (int) (var6 - var2) / 10 - var5;
					var5 = 40 - var8;
					if (var5 < 5) {
						var5 = 5;
					}
					var4 = 0;
					var2 = var6;
				}
				try {
					Thread.sleep((long) var5);
				} catch (Exception var9) {
				}
			}
		} catch (Exception var10) {
		}
		this.flameThread = false;
		if (arg0 != -33833) {
			field1235 = !field1235;
		}
	}

	public final void run() {
		if (this.flamesThread) {
			this.runFlames(-33833);
		} else if (this.startMidiThread) {
			this.runMidi(true);
		} else {
			super.run();
		}
	}

	@ObfuscatedName("client.a(IIIIIZIILhc;)V")
	public final void handleScrollInput(int arg0, int arg1, int arg2, int arg3, int arg4, boolean arg5, int arg6, int arg7, Component arg8) {
		if (this.scrollGrabbed) {
			this.scrollInputPadding = 32;
		} else {
			this.scrollInputPadding = 0;
		}
		this.scrollGrabbed = false;
		this.packetSize += arg1;
		if (arg0 >= arg6 && arg0 < arg6 + 16 && arg2 >= arg7 && arg2 < arg7 + 16) {
			arg8.scrollPosition -= this.dragCycles * 4;
			if (arg5) {
				this.redrawSidebar = true;
				return;
			}
		} else if (arg0 >= arg6 && arg0 < arg6 + 16 && arg2 >= arg7 + arg4 - 16 && arg2 < arg7 + arg4) {
			arg8.scrollPosition += this.dragCycles * 4;
			if (arg5) {
				this.redrawSidebar = true;
				return;
			}
		} else if (arg0 >= arg6 - this.scrollInputPadding && arg0 < arg6 + 16 + this.scrollInputPadding && arg2 >= arg7 + 16 && arg2 < arg7 + arg4 - 16 && this.dragCycles > 0) {
			int var10 = (arg4 - 32) * arg4 / arg3;
			if (var10 < 8) {
				var10 = 8;
			}
			int var11 = arg2 - arg7 - 16 - var10 / 2;
			int var12 = arg4 - 32 - var10;
			arg8.scrollPosition = (arg3 - arg4) * var11 / var12;
			if (arg5) {
				this.redrawSidebar = true;
			}
			this.scrollGrabbed = true;
		} else {
			return;
		}
	}

	@ObfuscatedName("client.a(Ljava/lang/String;Ljava/lang/String;Z)V")
	public final void login(String arg0, String arg1, boolean arg2) {
		signlink.errorname = arg0;
		try {
			if (!arg2) {
				this.loginMessage0 = "";
				this.loginMessage1 = "Connecting to server...";
				this.drawTitleScreen(4);
			}
			this.stream = new ClientStream(this, (byte) 2, this.openSocket(portOffset + 43594));
			this.stream.read(this.in.data, 0, 8);
			this.in.pos = 0;
			this.serverSeed = this.in.g8(603);
			int[] var4 = new int[] { (int) (Math.random() * 9.9999999E7D), (int) (Math.random() * 9.9999999E7D), (int) (this.serverSeed >> 32), (int) this.serverSeed };
			this.out.pos = 0;
			this.out.p1(10);
			this.out.p4(var4[0]);
			this.out.p4(var4[1]);
			this.out.p4(var4[2]);
			this.out.p4(var4[3]);
			this.out.p4(signlink.uid);
			this.out.pjstr(arg0);
			this.out.pjstr(arg1);
			this.out.rsaenc(RSA_MODULUS, RSA_EXPONENT, this.field1098);
			this.login.pos = 0;
			if (arg2) {
				this.login.p1(18);
			} else {
				this.login.p1(16);
			}
			this.login.p1(this.out.pos + 36 + 1 + 1);
			this.login.p1(225);
			this.login.p1(lowMemory ? 1 : 0);
			for (int var5 = 0; var5 < 9; var5++) {
				this.login.p4(this.archiveChecksum[var5]);
			}
			this.login.pdata(this.out.data, this.out.pos, 0, (byte) -106);
			this.out.random = new Isaac((byte) 1, var4);
			for (int var6 = 0; var6 < 4; var6++) {
				var4[var6] += 50;
			}
			this.randomIn = new Isaac((byte) 1, var4);
			this.stream.write(this.login.data, this.login.pos, true, 0);
			int var7 = this.stream.read();
			if (var7 == 1) {
				try {
					Thread.sleep(2000L);
				} catch (Exception var14) {
				}
				this.login(arg0, arg1, arg2);
				return;
			}
			if (var7 == 2 || var7 == 18) {
				if (var7 == 18) {
					this.rights = true;
				} else {
					this.rights = false;
				}
				InputTracking.setDisabled((byte) 65);
				this.ingame = true;
				this.out.pos = 0;
				this.in.pos = 0;
				this.packetType = -1;
				this.lastPacketType0 = -1;
				this.lastPacketType1 = -1;
				this.lastPacketType2 = -1;
				this.packetSize = 0;
				this.idleNetCycles = 0;
				this.systemUpdateTimer = 0;
				this.idleTimeout = 0;
				this.hintType = 0;
				this.menuSize = 0;
				this.menuVisible = false;
				super.idleCycles = 0;
				for (int var8 = 0; var8 < 100; var8++) {
					this.messageText[var8] = null;
				}
				this.objSelected = 0;
				this.spellSelected = 0;
				this.sceneState = 0;
				this.waveCount = 0;
				this.cameraAnticheatOffsetX = (int) (Math.random() * 100.0D) - 50;
				this.cameraAnticheatOffsetZ = (int) (Math.random() * 110.0D) - 55;
				this.cameraAnticheatAngle = (int) (Math.random() * 80.0D) - 40;
				this.minimapAnticheatAngle = (int) (Math.random() * 120.0D) - 60;
				this.minimapZoom = (int) (Math.random() * 30.0D) - 20;
				this.orbitCameraYaw = (int) (Math.random() * 20.0D) - 10 & 0x7FF;
				this.minimapLevel = -1;
				this.flagSceneTileX = 0;
				this.flagSceneTileZ = 0;
				this.playerCount = 0;
				this.npcCount = 0;
				for (int var9 = 0; var9 < this.MAX_PLAYER_COUNT; var9++) {
					this.players[var9] = null;
					this.playerAppearanceBuffer[var9] = null;
				}
				for (int var10 = 0; var10 < 8192; var10++) {
					this.npcs[var10] = null;
				}
				this.localPlayer = this.players[this.LOCAL_PLAYER_INDEX] = new PlayerEntity();
				this.projectiles.clear();
				this.spotanims.clear();
				this.mergedLocations.clear();
				int var12;
				for (int var11 = 0; var11 < 4; var11++) {
					for (var12 = 0; var12 < 104; var12++) {
						for (int var13 = 0; var13 < 104; var13++) {
							this.levelObjStacks[var11][var12][var13] = null;
						}
					}
				}
				this.spawnedLocations = new LinkList(0);
				this.friendCount = 0;
				this.stickyChatInterfaceId = -1;
				this.chatInterfaceId = -1;
				this.viewportInterfaceId = -1;
				this.sidebarInterfaceId = -1;
				this.pressedContinueOption = false;
				this.selectedTab = 3;
				this.chatbackInputOpen = false;
				this.menuVisible = false;
				this.showSocialInput = false;
				this.modalMessage = null;
				this.inMultizone = 0;
				this.flashingTab = -1;
				this.designGenderMale = true;
				this.validateCharacterDesign((byte) -6);
				for (var12 = 0; var12 < 5; var12++) {
					this.designColors[var12] = 0;
				}
				oplogic1 = 0;
				oplogic2 = 0;
				oplogic3 = 0;
				oplogic4 = 0;
				oplogic5 = 0;
				oplogic6 = 0;
				oplogic7 = 0;
				oplogic8 = 0;
				oplogic9 = 0;
				field1382 = 0;
				this.prepareGameScreen(-7185);
				return;
			}
			if (var7 == 3) {
				this.loginMessage0 = "";
				this.loginMessage1 = "Invalid username or password.";
				return;
			}
			if (var7 == 4) {
				this.loginMessage0 = "Your account has been disabled.";
				this.loginMessage1 = "Please check your message-centre for details.";
				return;
			}
			if (var7 == 5) {
				this.loginMessage0 = "Your account is already logged in.";
				this.loginMessage1 = "Try again in 60 secs...";
				return;
			}
			if (var7 == 6) {
				this.loginMessage0 = "RuneScape has been updated!";
				this.loginMessage1 = "Please reload this page.";
				return;
			}
			if (var7 == 7) {
				this.loginMessage0 = "This world is full.";
				this.loginMessage1 = "Please use a different world.";
				return;
			}
			if (var7 == 8) {
				this.loginMessage0 = "Unable to connect.";
				this.loginMessage1 = "Login server offline.";
				return;
			}
			if (var7 == 9) {
				this.loginMessage0 = "Login limit exceeded.";
				this.loginMessage1 = "Too many connections from your address.";
				return;
			}
			if (var7 == 10) {
				this.loginMessage0 = "Unable to connect.";
				this.loginMessage1 = "Bad session id.";
				return;
			}
			if (var7 == 11) {
				this.loginMessage1 = "Login server rejected session.";
				this.loginMessage1 = "Please try again.";
				return;
			}
			if (var7 == 12) {
				this.loginMessage0 = "You need a members account to login to this world.";
				this.loginMessage1 = "Please subscribe, or use a different world.";
				return;
			}
			if (var7 == 13) {
				this.loginMessage0 = "Could not complete login.";
				this.loginMessage1 = "Please try using a different world.";
				return;
			}
			if (var7 == 14) {
				this.loginMessage0 = "The server is being updated.";
				this.loginMessage1 = "Please wait 1 minute and try again.";
				return;
			}
			if (var7 == 15) {
				this.ingame = true;
				this.out.pos = 0;
				this.in.pos = 0;
				this.packetType = -1;
				this.lastPacketType0 = -1;
				this.lastPacketType1 = -1;
				this.lastPacketType2 = -1;
				this.packetSize = 0;
				this.idleNetCycles = 0;
				this.systemUpdateTimer = 0;
				this.menuSize = 0;
				this.menuVisible = false;
				return;
			}
			if (var7 == 16) {
				this.loginMessage0 = "Login attempts exceeded.";
				this.loginMessage1 = "Please wait 1 minute and try again.";
				return;
			}
			if (var7 == 17) {
				this.loginMessage0 = "You are standing in a members-only area.";
				this.loginMessage1 = "To play on this world move to a free area first";
				return;
			}
		} catch (IOException var15) {
			this.loginMessage0 = "";
			this.loginMessage1 = "Error connecting to server.";
		}
	}

	@ObfuscatedName("client.a(IIIIIIII)V")
	public final void addLoc(int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7) {
		if (arg6 != -27819) {
			this.load();
		}
		if (arg1 < 1 || arg2 < 1 || arg1 > 102 || arg2 > 102) {
			return;
		}
		if (lowMemory && arg7 != this.currentLevel) {
			return;
		}
		int var9 = 0;
		boolean var10 = true;
		boolean var11 = false;
		boolean var12 = false;
		if (arg3 == 0) {
			var9 = this.scene.getWallBitset(arg7, arg1, arg2);
		}
		if (arg3 == 1) {
			var9 = this.scene.getWallDecorationBitset(arg7, arg2, 3, arg1);
		}
		if (arg3 == 2) {
			var9 = this.scene.getLocBitset(arg7, arg1, arg2);
		}
		if (arg3 == 3) {
			var9 = this.scene.getGroundDecorationBitset(arg7, arg1, arg2);
		}
		int var13;
		if (var9 != 0) {
			var13 = this.scene.getInfo(arg7, arg1, arg2, var9);
			int var15 = var9 >> 14 & 0x7FFF;
			int var16 = var13 & 0x1F;
			int var17 = var13 >> 6;
			LocType var14;
			if (arg3 == 0) {
				this.scene.removeWall(arg1, arg7, arg2, 1);
				var14 = LocType.get(var15);
				if (var14.blockwalk) {
					this.levelCollisionMap[arg7].delWall(var14.blockrange, var17, arg1, arg2, 323, var16);
				}
			}
			if (arg3 == 1) {
				this.scene.removeWallDecoration(arg7, arg2, this.field1442, arg1);
			}
			if (arg3 == 2) {
				this.scene.removeLoc(arg1, arg2, -54, arg7);
				var14 = LocType.get(var15);
				if (arg1 + var14.width > 103 || arg2 + var14.width > 103 || arg1 + var14.length > 103 || arg2 + var14.length > 103) {
					return;
				}
				if (var14.blockwalk) {
					this.levelCollisionMap[arg7].delLoc(arg2, arg1, var17, var14.width, true, var14.blockrange, var14.length);
				}
			}
			if (arg3 == 3) {
				this.scene.removeGroundDecoration(arg7, this.field1387, arg1, arg2);
				var14 = LocType.get(var15);
				if (var14.blockwalk && var14.active) {
					this.levelCollisionMap[arg7].removeBlocked(arg2, arg1, 0);
				}
			}
		}
		if (arg4 < 0) {
			return;
		}
		var13 = arg7;
		if (arg7 < 3 && (this.levelTileFlags[1][arg1][arg2] & 0x2) == 2) {
			var13 = arg7 + 1;
		}
		World.addLoc(arg1, this.locList, this.levelCollisionMap[arg7], arg2, arg0, this.levelHeightmap, 0, arg7, arg4, arg5, this.scene, var13);
		return;
	}

	@ObfuscatedName("client.a(JI)V")
	public final void addFriend(long arg0, int arg1) {
		if (arg0 == 0L) {
			return;
		}
		if (this.friendCount >= 100) {
			this.addMessage(0, "Your friends list is full. Max of 100 hit", (byte) 4, "");
			return;
		}
		String var4 = JString.formatName(0, JString.fromBase37(arg0, false));
		for (int var5 = 0; var5 < this.friendCount; var5++) {
			if (this.friendName37[var5] == arg0) {
				this.addMessage(0, var4 + " is already on your friend list", (byte) 4, "");
				return;
			}
		}
		for (int var6 = 0; var6 < this.ignoreCount; var6++) {
			if (this.ignoreName37[var6] == arg0) {
				this.addMessage(0, "Please remove " + var4 + " from your ignore list first", (byte) 4, "");
				return;
			}
		}
		if (var4.equals(this.localPlayer.name)) {
			return;
		}
		this.friendName[this.friendCount] = var4;
		this.friendName37[this.friendCount] = arg0;
		this.friendWorld[this.friendCount] = 0;
		this.friendCount++;
		if (arg1 >= 0) {
			this.field1404 = this.randomIn.takeNextValue();
		}
		this.redrawSidebar = true;
		this.out.p1isaac((byte) -34, 118);
		this.out.p8(true, arg0);
	}

	@ObfuscatedName("client.a(B)V")
	public final void unload(byte arg0) {
		signlink.reporterror = false;
		try {
			if (this.stream != null) {
				this.stream.close();
			}
		} catch (Exception var2) {
		}
		this.stream = null;
		this.stopMidi(0);
		this.midiThreadActive = false;
		this.out = null;
		this.login = null;
		this.in = null;
		this.sceneMapIndex = null;
		this.sceneMapLandData = null;
		this.sceneMapLocData = null;
		this.levelHeightmap = null;
		this.levelTileFlags = null;
		this.scene = null;
		this.levelCollisionMap = null;
		this.bfsDirection = null;
		this.bfsCost = null;
		this.bfsStepX = null;
		this.bfsStepZ = null;
		this.textureBuffer = null;
		this.areaSidebar = null;
		this.areaMapback = null;
		this.areaViewport = null;
		this.areaChatback = null;
		this.areaBackbase1 = null;
		this.areaBackbase2 = null;
		this.areaBackhmid1 = null;
		this.areaBackleft1 = null;
		this.areaBackleft2 = null;
		this.areaBackright1 = null;
		this.areaBackright2 = null;
		this.areaBacktop1 = null;
		this.areaBacktop2 = null;
		this.areaBackvmid1 = null;
		this.areaBackvmid2 = null;
		this.areaBackvmid3 = null;
		this.areaBackhmid2 = null;
		this.imageInvback = null;
		this.imageMapback = null;
		this.imageChatback = null;
		this.imageBackbase1 = null;
		this.imageBackbase2 = null;
		this.imageBackhmid1 = null;
		this.imageSideicons = null;
		this.imageRedstone1 = null;
		this.imageRedstone2 = null;
		this.imageRedstone3 = null;
		this.imageRedstone1h = null;
		this.imageRedstone2h = null;
		this.imageRedstone1v = null;
		this.imageRedstone2v = null;
		this.imageRedstone3v = null;
		this.imageRedstone1hv = null;
		this.imageRedstone2hv = null;
		this.imageCompass = null;
		this.imageHitmarks = null;
		this.imageHeadicons = null;
		this.imageCrosses = null;
		this.imageMapdot0 = null;
		this.imageMapdot1 = null;
		this.imageMapdot2 = null;
		this.imageMapdot3 = null;
		this.imageMapscene = null;
		if (arg0 != -28) {
			this.load();
		}
		this.imageMapfunction = null;
		this.tileLastOccupiedCycle = null;
		this.players = null;
		this.playerIds = null;
		this.entityUpdateIds = null;
		this.playerAppearanceBuffer = null;
		this.entityRemovalIds = null;
		this.npcs = null;
		this.npcIds = null;
		this.levelObjStacks = null;
		this.spawnedLocations = null;
		this.mergedLocations = null;
		this.projectiles = null;
		this.spotanims = null;
		this.locList = null;
		this.menuParamB = null;
		this.menuParamC = null;
		this.menuAction = null;
		this.menuParamA = null;
		this.menuOption = null;
		this.varps = null;
		this.activeMapFunctionX = null;
		this.activeMapFunctionZ = null;
		this.activeMapFunctions = null;
		this.imageMinimap = null;
		this.friendName = null;
		this.friendName37 = null;
		this.friendWorld = null;
		this.imageTitle0 = null;
		this.imageTitle1 = null;
		this.imageTitle2 = null;
		this.imageTitle3 = null;
		this.imageTitle4 = null;
		this.imageTitle5 = null;
		this.imageTitle6 = null;
		this.imageTitle7 = null;
		this.imageTitle8 = null;
		this.unloadTitle(true);
		LocType.unload(true);
		NpcType.unload(true);
		ObjType.unload(true);
		FloType.instances = null;
		IdkType.instances = null;
		Component.instances = null;
		class61.instances = null;
		SeqType.instances = null;
		SpotAnimType.instances = null;
		SpotAnimType.modelCache = null;
		VarpType.instances = null;
		super.drawArea = null;
		PlayerEntity.modelCache = null;
		Pix3D.unload(true);
		World3D.unload(true);
		Model.unload(true);
		AnimBase.instances = null;
		AnimFrame.instances = null;
		System.gc();
	}

	@ObfuscatedName("client.A(I)Ljava/net/Socket;")
	public final Socket openSocket(int arg0) throws IOException {
		return signlink.mainapp == null ? new Socket(InetAddress.getByName(this.getCodeBase().getHost()), arg0) : signlink.opensocket(arg0);
	}

	@ObfuscatedName("client.a(ZIILz;I)V")
	public final void addPlayerOptions(boolean arg0, int arg1, int arg2, PlayerEntity arg3, int arg4) {
		if (arg0) {
			this.field1387 = this.randomIn.takeNextValue();
		}
		if (arg3 == this.localPlayer || this.menuSize >= 400) {
			return;
		}
		String var6 = arg3.name + getCombatLevelColorTag(this.localPlayer.combatLevel, false, arg3.combatLevel) + " (level-" + arg3.combatLevel + ")";
		if (this.objSelected == 1) {
			this.menuOption[this.menuSize] = "Use " + this.objSelectedName + " with @whi@" + var6;
			this.menuAction[this.menuSize] = 367;
			this.menuParamA[this.menuSize] = arg2;
			this.menuParamB[this.menuSize] = arg4;
			this.menuParamC[this.menuSize] = arg1;
			this.menuSize++;
		} else if (this.spellSelected != 1) {
			this.menuOption[this.menuSize] = "Follow @whi@" + var6;
			this.menuAction[this.menuSize] = 1544;
			this.menuParamA[this.menuSize] = arg2;
			this.menuParamB[this.menuSize] = arg4;
			this.menuParamC[this.menuSize] = arg1;
			this.menuSize++;
			if (this.overrideChat == 0) {
				this.menuOption[this.menuSize] = "Trade with @whi@" + var6;
				this.menuAction[this.menuSize] = 1373;
				this.menuParamA[this.menuSize] = arg2;
				this.menuParamB[this.menuSize] = arg4;
				this.menuParamC[this.menuSize] = arg1;
				this.menuSize++;
			}
			if (this.wildernessLevel > 0) {
				this.menuOption[this.menuSize] = "Attack @whi@" + var6;
				if (this.localPlayer.combatLevel >= arg3.combatLevel) {
					this.menuAction[this.menuSize] = 151;
				} else {
					this.menuAction[this.menuSize] = 2151;
				}
				this.menuParamA[this.menuSize] = arg2;
				this.menuParamB[this.menuSize] = arg4;
				this.menuParamC[this.menuSize] = arg1;
				this.menuSize++;
			}
			if (this.worldLocationState == 1) {
				this.menuOption[this.menuSize] = "Fight @whi@" + var6;
				this.menuAction[this.menuSize] = 151;
				this.menuParamA[this.menuSize] = arg2;
				this.menuParamB[this.menuSize] = arg4;
				this.menuParamC[this.menuSize] = arg1;
				this.menuSize++;
			}
			if (this.worldLocationState == 2) {
				this.menuOption[this.menuSize] = "Duel-with @whi@" + var6;
				this.menuAction[this.menuSize] = 1101;
				this.menuParamA[this.menuSize] = arg2;
				this.menuParamB[this.menuSize] = arg4;
				this.menuParamC[this.menuSize] = arg1;
				this.menuSize++;
			}
		} else if ((this.activeSpellFlags & 0x8) == 8) {
			this.menuOption[this.menuSize] = this.spellCaption + " @whi@" + var6;
			this.menuAction[this.menuSize] = 651;
			this.menuParamA[this.menuSize] = arg2;
			this.menuParamB[this.menuSize] = arg4;
			this.menuParamC[this.menuSize] = arg1;
			this.menuSize++;
		}
		for (int var7 = 0; var7 < this.menuSize; var7++) {
			if (this.menuAction[var7] == 660) {
				this.menuOption[var7] = "Walk here @whi@" + var6;
				return;
			}
		}
	}

	@ObfuscatedName("client.j(Z)V")
	public final void updateGame(boolean arg0) {
		if (this.systemUpdateTimer > 1) {
			this.systemUpdateTimer--;
		}
		if (this.idleTimeout > 0) {
			this.idleTimeout--;
		}
		for (int var2 = 0; var2 < 5 && this.read(false); var2++) {
		}
		if (!arg0) {
			this.packetType = this.in.g1();
		}
		if (!this.ingame) {
			return;
		}
		int var12;
		int var10002;
		for (int var3 = 0; var3 < this.waveCount; var3++) {
			if (this.waveDelay[var3] <= 0) {
				boolean var4 = false;
				try {
					if (this.waveIds[var3] != this.lastWaveId || this.waveLoops[var3] != this.lastWaveLoops) {
						Packet var5 = Wave.generate((byte) -16, this.waveLoops[var3], this.waveIds[var3]);
						if (System.currentTimeMillis() + (long) (var5.pos / 22) > this.lastWaveStartTime + (long) (this.lastWaveLength / 22)) {
							this.lastWaveLength = var5.pos;
							this.lastWaveStartTime = System.currentTimeMillis();
							if (this.saveWave(var5.data, var5.pos, 0)) {
								this.lastWaveId = this.waveIds[var3];
								this.lastWaveLoops = this.waveLoops[var3];
							} else {
								var4 = true;
							}
						}
					} else if (!this.replayWave(this.field1143)) {
						var4 = true;
					}
				} catch (Exception var10) {
				}
				if (var4 && this.waveDelay[var3] != -5) {
					this.waveDelay[var3] = -5;
				} else {
					this.waveCount--;
					for (var12 = var3; var12 < this.waveCount; var12++) {
						this.waveIds[var12] = this.waveIds[var12 + 1];
						this.waveLoops[var12] = this.waveLoops[var12 + 1];
						this.waveDelay[var12] = this.waveDelay[var12 + 1];
					}
					var3--;
				}
			} else {
				var10002 = this.waveDelay[var3]--;
			}
		}
		if (this.nextMusicDelay > 0) {
			this.nextMusicDelay -= 20;
			if (this.nextMusicDelay < 0) {
				this.nextMusicDelay = 0;
			}
			if (this.nextMusicDelay == 0 && this.midiActive && !lowMemory) {
				this.setMidi(false, this.midiCrc, this.currentMidi, this.midiSize);
			}
		}
		Packet var11 = InputTracking.flush(-809);
		if (var11 != null) {
			this.out.p1isaac((byte) -34, 81);
			this.out.p2(var11.pos);
			this.out.pdata(var11.data, var11.pos, 0, (byte) -106);
			var11.release((byte) 8);
		}
		this.idleNetCycles++;
		if (this.idleNetCycles > 750) {
			this.tryReconnect(false);
		}
		this.updatePlayers(true);
		this.updateNpcs(true);
		this.updateEntityChats(this.field1487);
		this.updateMergeLocs(this.field1136);
		if ((super.actionKey[1] == 1 || super.actionKey[2] == 1 || super.actionKey[3] == 1 || super.actionKey[4] == 1) && this.cameraMovedWrite++ > 5) {
			this.cameraMovedWrite = 0;
			this.out.p1isaac((byte) -34, 189);
			this.out.p2(this.orbitCameraPitch);
			this.out.p2(this.orbitCameraYaw);
			this.out.p1(this.minimapAnticheatAngle);
			this.out.p1(this.minimapZoom);
		}
		this.sceneDelta++;
		if (this.crossMode != 0) {
			this.crossCycle += 20;
			if (this.crossCycle >= 400) {
				this.crossMode = 0;
			}
		}
		if (this.selectedArea != 0) {
			this.selectedCycle++;
			if (this.selectedCycle >= 15) {
				if (this.selectedArea == 2) {
					this.redrawSidebar = true;
				}
				if (this.selectedArea == 3) {
					this.redrawChatback = true;
				}
				this.selectedArea = 0;
			}
		}
		int var6;
		if (this.objDragArea != 0) {
			this.objDragCycles++;
			if (super.mouseX > this.objGrabX + 5 || super.mouseX < this.objGrabX - 5 || super.mouseY > this.objGrabY + 5 || super.mouseY < this.objGrabY - 5) {
				this.objGrabThreshold = true;
			}
			if (super.mouseButton == 0) {
				if (this.objDragArea == 2) {
					this.redrawSidebar = true;
				}
				if (this.objDragArea == 3) {
					this.redrawChatback = true;
				}
				this.objDragArea = 0;
				if (this.objGrabThreshold && this.objDragCycles >= 5) {
					this.hoveredSlotParentId = -1;
					this.handleInput(26);
					if (this.hoveredSlotParentId == this.objDragInterfaceId && this.hoveredSlot != this.objDragSlot) {
						Component var13 = Component.instances[this.objDragInterfaceId];
						var6 = var13.invSlotObjId[this.hoveredSlot];
						var13.invSlotObjId[this.hoveredSlot] = var13.invSlotObjId[this.objDragSlot];
						var13.invSlotObjId[this.objDragSlot] = var6;
						int var14 = var13.invSlotObjCount[this.hoveredSlot];
						var13.invSlotObjCount[this.hoveredSlot] = var13.invSlotObjCount[this.objDragSlot];
						var13.invSlotObjCount[this.objDragSlot] = var14;
						this.out.p1isaac((byte) -34, 159);
						this.out.p2(this.objDragInterfaceId);
						this.out.p2(this.objDragSlot);
						this.out.p2(this.hoveredSlot);
					}
				} else if ((this.mouseButtonsOption == 1 || this.isAddFriendOption(145, this.menuSize - 1)) && this.menuSize > 2) {
					this.showContextMenu(-386);
				} else if (this.menuSize > 0) {
					this.useMenuOption(6412, this.menuSize - 1);
				}
				this.selectedCycle = 10;
				super.mouseClickButton = 0;
			}
		}
		cyclelogic3++;
		if (cyclelogic3 > 127) {
			cyclelogic3 = 0;
			this.out.p1isaac((byte) -34, 215);
			this.out.p3(4991788);
		}
		if (World3D.clickTileX != -1) {
			var12 = World3D.clickTileX;
			var6 = World3D.clickTileZ;
			boolean var7 = this.tryMove(this.localPlayer.pathTileX[0], 0, true, var12, this.localPlayer.pathTileZ[0], 0, 0, 0, var6, 0, 0, 0);
			World3D.clickTileX = -1;
			if (var7) {
				this.crossX = super.mouseClickX;
				this.crossY = super.mouseClickY;
				this.crossMode = 1;
				this.crossCycle = 0;
			}
		}
		if (super.mouseClickButton == 1 && this.modalMessage != null) {
			this.modalMessage = null;
			this.redrawChatback = true;
			super.mouseClickButton = 0;
		}
		this.handleMouseInput((byte) 3);
		this.handleMinimapInput((byte) 7);
		this.handleTabInput(this.field1497);
		this.handleChatSettingsInput(0);
		if (super.mouseButton == 1 || super.mouseClickButton == 1) {
			this.dragCycles++;
		}
		if (this.sceneState == 2) {
			this.updateOrbitCamera(0);
		}
		if (this.sceneState == 2 && this.cutscene) {
			this.applyCutscene(this.field1216);
		}
		for (var12 = 0; var12 < 5; var12++) {
			var10002 = this.cameraModifierCycle[var12]++;
		}
		this.handleInputKey(7);
		super.idleCycles++;
		if (super.idleCycles > 4500) {
			this.idleTimeout = 250;
			super.idleCycles -= 500;
			this.out.p1isaac((byte) -34, 70);
		}
		this.cameraOffsetCycle++;
		if (this.cameraOffsetCycle > 500) {
			this.cameraOffsetCycle = 0;
			var6 = (int) (Math.random() * 8.0D);
			if ((var6 & 0x1) == 1) {
				this.cameraAnticheatOffsetX += this.cameraOffsetXModifier;
			}
			if ((var6 & 0x2) == 2) {
				this.cameraAnticheatOffsetZ += this.cameraOffsetZModifier;
			}
			if ((var6 & 0x4) == 4) {
				this.cameraAnticheatAngle += this.cameraOffsetYawModifier;
			}
		}
		if (this.cameraAnticheatOffsetX < -50) {
			this.cameraOffsetXModifier = 2;
		}
		if (this.cameraAnticheatOffsetX > 50) {
			this.cameraOffsetXModifier = -2;
		}
		if (this.cameraAnticheatOffsetZ < -55) {
			this.cameraOffsetZModifier = 2;
		}
		if (this.cameraAnticheatOffsetZ > 55) {
			this.cameraOffsetZModifier = -2;
		}
		if (this.cameraAnticheatAngle < -40) {
			this.cameraOffsetYawModifier = 1;
		}
		if (this.cameraAnticheatAngle > 40) {
			this.cameraOffsetYawModifier = -1;
		}
		this.minimapOffsetCycle++;
		if (this.minimapOffsetCycle > 500) {
			this.minimapOffsetCycle = 0;
			var6 = (int) (Math.random() * 8.0D);
			if ((var6 & 0x1) == 1) {
				this.minimapAnticheatAngle += this.minimapAngleModifier;
			}
			if ((var6 & 0x2) == 2) {
				this.minimapZoom += this.minimapZoomModifier;
			}
		}
		if (this.minimapAnticheatAngle < -60) {
			this.minimapAngleModifier = 2;
		}
		if (this.minimapAnticheatAngle > 60) {
			this.minimapAngleModifier = -2;
		}
		if (this.minimapZoom < -20) {
			this.minimapZoomModifier = 1;
		}
		if (this.minimapZoom > 10) {
			this.minimapZoomModifier = -1;
		}
		cyclelogic4++;
		if (cyclelogic4 > 110) {
			cyclelogic4 = 0;
			this.out.p1isaac((byte) -34, 236);
			this.out.p4(0);
		}
		this.heartbeatTimer++;
		if (this.heartbeatTimer > 50) {
			this.out.p1isaac((byte) -34, 108);
		}
		try {
			if (this.stream != null && this.out.pos > 0) {
				this.stream.write(this.out.data, this.out.pos, true, 0);
				this.out.pos = 0;
				this.heartbeatTimer = 0;
			}
		} catch (IOException var8) {
			this.tryReconnect(false);
		} catch (Exception var9) {
			this.logout(-780);
		}
	}

	@ObfuscatedName("client.k(Z)V")
	public final void drawTooltip(boolean arg0) {
		if (this.menuSize < 2 && this.objSelected == 0 && this.spellSelected == 0) {
			return;
		}
		String var2;
		if (this.objSelected == 1 && this.menuSize < 2) {
			var2 = "Use " + this.objSelectedName + " with...";
		} else if (this.spellSelected == 1 && this.menuSize < 2) {
			var2 = this.spellCaption + "...";
		} else {
			var2 = this.menuOption[this.menuSize - 1];
		}
		if (this.menuSize > 2) {
			var2 = var2 + "@whi@ / " + (this.menuSize - 2) + " more options";
		}
		this.fontBold12.drawStringTooltip(loopCycle / 1000, true, (byte) -121, 15, 16777215, var2, 4);
		if (!arg0) {
			this.packetType = -1;
		}
	}

	@ObfuscatedName("client.k(B)V")
	public final void pushSpotanims(byte arg0) {
		if (arg0 != this.field1131) {
			this.field1170 = -357;
		}
		for (SpotAnimEntity var2 = (SpotAnimEntity) this.spotanims.head(); var2 != null; var2 = (SpotAnimEntity) this.spotanims.next(551)) {
			if (var2.level != this.currentLevel || var2.seqComplete) {
				var2.unlink();
			} else if (loopCycle >= var2.startCycle) {
				var2.update(this.sceneDelta, 0);
				if (var2.seqComplete) {
					var2.unlink();
				} else {
					this.scene.addTemporary(-44713, var2.z, 60, 0, var2.x, -1, false, null, var2, var2.y, var2.level);
				}
			}
		}
	}

	public final URL getCodeBase() {
		if (signlink.mainapp != null) {
			return signlink.mainapp.getCodeBase();
		}
		try {
			if (super.frame != null) {
				return new URL("http://127.0.0.1:" + System.getProperty("rs2.reference.webport", "8889"));
			}
		} catch (Exception var1) {
		}
		return super.getCodeBase();
	}

	@ObfuscatedName("client.B(I)V")
	public static final void setHighMemory(int arg0) {
		World3D.lowMemory = false;
		Pix3D.lowDetail = false;
		if (arg0 == 9) {
			lowMemory = false;
			World.lowMemory = false;
		}
	}

	@ObfuscatedName("client.a(IIZIIIIIIIII)Z")
	public final boolean tryMove(int arg0, int arg1, boolean arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11) {
		byte var13 = 104;
		byte var14 = 104;
		int var16;
		for (int var15 = 0; var15 < var13; var15++) {
			for (var16 = 0; var16 < var14; var16++) {
				this.bfsDirection[var15][var16] = 0;
				this.bfsCost[var15][var16] = 99999999;
			}
		}
		var16 = arg0;
		int var17 = arg4;
		this.bfsDirection[arg0][arg4] = 99;
		this.bfsCost[arg0][arg4] = 0;
		byte var18 = 0;
		int var19 = 0;
		this.bfsStepX[var18] = arg0;
		int var28 = var18 + 1;
		this.bfsStepZ[var18] = arg4;
		boolean var20 = false;
		int var21 = this.bfsStepX.length;
		int[][] var22 = this.levelCollisionMap[this.currentLevel].flags;
		int var23;
		while (var19 != var28) {
			var16 = this.bfsStepX[var19];
			var17 = this.bfsStepZ[var19];
			var19 = (var19 + 1) % var21;
			if (var16 == arg3 && var17 == arg8) {
				var20 = true;
				break;
			}
			if (arg10 != 0) {
				if ((arg10 < 5 || arg10 == 10) && this.levelCollisionMap[this.currentLevel].testWall(-7517, arg9, arg8, arg10 - 1, var17, arg3, var16)) {
					var20 = true;
					break;
				}
				if (arg10 < 10 && this.levelCollisionMap[this.currentLevel].testWDecor(arg9, arg10 - 1, this.field1404, var16, arg3, var17, arg8)) {
					var20 = true;
					break;
				}
			}
			if (arg1 != 0 && arg7 != 0 && this.levelCollisionMap[this.currentLevel].testLoc(var17, arg7, var16, arg3, arg11, arg8, arg1, 168)) {
				var20 = true;
				break;
			}
			var23 = this.bfsCost[var16][var17] + 1;
			if (var16 > 0 && this.bfsDirection[var16 - 1][var17] == 0 && (var22[var16 - 1][var17] & 0x280108) == 0) {
				this.bfsStepX[var28] = var16 - 1;
				this.bfsStepZ[var28] = var17;
				var28 = (var28 + 1) % var21;
				this.bfsDirection[var16 - 1][var17] = 2;
				this.bfsCost[var16 - 1][var17] = var23;
			}
			if (var16 < var13 - 1 && this.bfsDirection[var16 + 1][var17] == 0 && (var22[var16 + 1][var17] & 0x280180) == 0) {
				this.bfsStepX[var28] = var16 + 1;
				this.bfsStepZ[var28] = var17;
				var28 = (var28 + 1) % var21;
				this.bfsDirection[var16 + 1][var17] = 8;
				this.bfsCost[var16 + 1][var17] = var23;
			}
			if (var17 > 0 && this.bfsDirection[var16][var17 - 1] == 0 && (var22[var16][var17 - 1] & 0x280102) == 0) {
				this.bfsStepX[var28] = var16;
				this.bfsStepZ[var28] = var17 - 1;
				var28 = (var28 + 1) % var21;
				this.bfsDirection[var16][var17 - 1] = 1;
				this.bfsCost[var16][var17 - 1] = var23;
			}
			if (var17 < var14 - 1 && this.bfsDirection[var16][var17 + 1] == 0 && (var22[var16][var17 + 1] & 0x280120) == 0) {
				this.bfsStepX[var28] = var16;
				this.bfsStepZ[var28] = var17 + 1;
				var28 = (var28 + 1) % var21;
				this.bfsDirection[var16][var17 + 1] = 4;
				this.bfsCost[var16][var17 + 1] = var23;
			}
			if (var16 > 0 && var17 > 0 && this.bfsDirection[var16 - 1][var17 - 1] == 0 && (var22[var16 - 1][var17 - 1] & 0x28010E) == 0 && (var22[var16 - 1][var17] & 0x280108) == 0 && (var22[var16][var17 - 1] & 0x280102) == 0) {
				this.bfsStepX[var28] = var16 - 1;
				this.bfsStepZ[var28] = var17 - 1;
				var28 = (var28 + 1) % var21;
				this.bfsDirection[var16 - 1][var17 - 1] = 3;
				this.bfsCost[var16 - 1][var17 - 1] = var23;
			}
			if (var16 < var13 - 1 && var17 > 0 && this.bfsDirection[var16 + 1][var17 - 1] == 0 && (var22[var16 + 1][var17 - 1] & 0x280183) == 0 && (var22[var16 + 1][var17] & 0x280180) == 0 && (var22[var16][var17 - 1] & 0x280102) == 0) {
				this.bfsStepX[var28] = var16 + 1;
				this.bfsStepZ[var28] = var17 - 1;
				var28 = (var28 + 1) % var21;
				this.bfsDirection[var16 + 1][var17 - 1] = 9;
				this.bfsCost[var16 + 1][var17 - 1] = var23;
			}
			if (var16 > 0 && var17 < var14 - 1 && this.bfsDirection[var16 - 1][var17 + 1] == 0 && (var22[var16 - 1][var17 + 1] & 0x280138) == 0 && (var22[var16 - 1][var17] & 0x280108) == 0 && (var22[var16][var17 + 1] & 0x280120) == 0) {
				this.bfsStepX[var28] = var16 - 1;
				this.bfsStepZ[var28] = var17 + 1;
				var28 = (var28 + 1) % var21;
				this.bfsDirection[var16 - 1][var17 + 1] = 6;
				this.bfsCost[var16 - 1][var17 + 1] = var23;
			}
			if (var16 < var13 - 1 && var17 < var14 - 1 && this.bfsDirection[var16 + 1][var17 + 1] == 0 && (var22[var16 + 1][var17 + 1] & 0x2801E0) == 0 && (var22[var16 + 1][var17] & 0x280180) == 0 && (var22[var16][var17 + 1] & 0x280120) == 0) {
				this.bfsStepX[var28] = var16 + 1;
				this.bfsStepZ[var28] = var17 + 1;
				var28 = (var28 + 1) % var21;
				this.bfsDirection[var16 + 1][var17 + 1] = 12;
				this.bfsCost[var16 + 1][var17 + 1] = var23;
			}
		}
		this.tryMoveNearest = 0;
		int var24;
		int var25;
		int var26;
		if (!var20) {
			if (arg2) {
				var23 = 100;
				for (var24 = 1; var24 < 2; var24++) {
					for (var25 = arg3 - var24; var25 <= arg3 + var24; var25++) {
						for (var26 = arg8 - var24; var26 <= arg8 + var24; var26++) {
							if (var25 >= 0 && var26 >= 0 && var25 < 104 && var26 < 104 && this.bfsCost[var25][var26] < var23) {
								var23 = this.bfsCost[var25][var26];
								var16 = var25;
								var17 = var26;
								this.tryMoveNearest = 1;
								var20 = true;
							}
						}
					}
					if (var20) {
						break;
					}
				}
			}
			if (!var20) {
				return false;
			}
		}
		byte var29 = 0;
		this.bfsStepX[var29] = var16;
		if (arg5 != 0) {
			this.packetType = this.in.g1();
		}
		var19 = var29 + 1;
		this.bfsStepZ[var29] = var17;
		var23 = var24 = this.bfsDirection[var16][var17];
		while (var16 != arg0 || var17 != arg4) {
			if (var23 != var24) {
				var24 = var23;
				this.bfsStepX[var19] = var16;
				this.bfsStepZ[var19++] = var17;
			}
			if ((var23 & 0x2) != 0) {
				var16++;
			} else if ((var23 & 0x8) != 0) {
				var16--;
			}
			if ((var23 & 0x1) != 0) {
				var17++;
			} else if ((var23 & 0x4) != 0) {
				var17--;
			}
			var23 = this.bfsDirection[var16][var17];
		}
		if (var19 > 0) {
			var21 = var19;
			if (var19 > 25) {
				var21 = 25;
			}
			var19--;
			var25 = this.bfsStepX[var19];
			var26 = this.bfsStepZ[var19];
			if (arg6 == 0) {
				this.out.p1isaac((byte) -34, 181);
				this.out.p1(var21 + var21 + 3);
			}
			if (arg6 == 1) {
				this.out.p1isaac((byte) -34, 165);
				this.out.p1(var21 + var21 + 3 + 14);
			}
			if (arg6 == 2) {
				this.out.p1isaac((byte) -34, 93);
				this.out.p1(var21 + var21 + 3);
			}
			if (super.actionKey[5] == 1) {
				this.out.p1(1);
			} else {
				this.out.p1(0);
			}
			this.out.p2(var25 + this.sceneBaseTileX);
			this.out.p2(var26 + this.sceneBaseTileZ);
			this.flagSceneTileX = this.bfsStepX[0];
			this.flagSceneTileZ = this.bfsStepZ[0];
			for (int var27 = 1; var27 < var21; var27++) {
				var19--;
				this.out.p1(this.bfsStepX[var19] - var25);
				this.out.p1(this.bfsStepZ[var19] - var26);
			}
			return true;
		} else if (arg6 == 1) {
			return false;
		} else {
			return true;
		}
	}

	@ObfuscatedName("client.a(ZI)Ljava/lang/String;")
	public static final String formatObjCount(boolean arg0, int arg1) {
		if (!arg0) {
			field1264 = 434;
		}
		if (arg1 < 100000) {
			return String.valueOf(arg1);
		} else if (arg1 < 10000000) {
			return arg1 / 1000 + "K";
		} else {
			return arg1 / 1000000 + "M";
		}
	}

	@ObfuscatedName("client.b(Lkb;II)V")
	public final void getPlayer(Packet arg0, int arg1, int arg2) {
		this.entityRemovalCount = 0;
		this.entityUpdateCount = 0;
		this.getPlayerLocal(11522, arg1, arg0);
		this.getPlayerOldVis(arg1, arg0, 0);
		this.getPlayerNewVis(2, arg1, arg0);
		this.getPlayerExtended(true, arg1, arg0);
		int var5;
		for (int var4 = 0; var4 < this.entityRemovalCount; var4++) {
			var5 = this.entityRemovalIds[var4];
			if (this.players[var5].cycle != loopCycle) {
				this.players[var5] = null;
			}
		}
		if (arg2 <= 0) {
			return;
		}
		if (arg0.pos != arg1) {
			signlink.reporterror("Error packet size mismatch in getplayer pos:" + arg0.pos + " psize:" + arg1);
			throw new RuntimeException("eek");
		}
		for (var5 = 0; var5 < this.playerCount; var5++) {
			if (this.players[this.playerIds[var5]] == null) {
				signlink.reporterror(this.username + " null entry in pl list - pos:" + var5 + " size:" + this.playerCount);
				throw new RuntimeException("eek");
			}
		}
	}

	@ObfuscatedName("client.d(III)Z")
	public final boolean updateInterfaceAnimation(int arg0, int arg1, int arg2) {
		boolean var4 = false;
		Component var5 = Component.instances[arg0];
		for (int var6 = 0; var6 < var5.childId.length && var5.childId[var6] != -1; var6++) {
			Component var7 = Component.instances[var5.childId[var6]];
			if (var7.type == 1) {
				var4 |= this.updateInterfaceAnimation(var7.id, arg1, 623);
			}
			if (var7.type == 6 && (var7.anim != -1 || var7.activeAnim != -1)) {
				boolean var8 = this.executeInterfaceScript(var7, 65);
				int var9;
				if (var8) {
					var9 = var7.activeAnim;
				} else {
					var9 = var7.anim;
				}
				if (var9 != -1) {
					SeqType var10 = SeqType.instances[var9];
					var7.seqCycle += arg1;
					while (var7.seqCycle > var10.delay[var7.seqFrame]) {
						var7.seqCycle -= var10.delay[var7.seqFrame] + 1;
						var7.seqFrame++;
						if (var7.seqFrame >= var10.frameCount) {
							var7.seqFrame -= var10.replayoff;
							if (var7.seqFrame < 0 || var7.seqFrame >= var10.frameCount) {
								var7.seqFrame = 0;
							}
						}
						var4 = true;
					}
				}
			}
		}
		if (arg2 <= 0) {
			throw new NullPointerException();
		}
		return var4;
	}

	@ObfuscatedName("client.a(ILjava/lang/String;BLjava/lang/String;)V")
	public final void addMessage(int arg0, String arg1, byte arg2, String arg3) {
		if (arg0 == 0 && this.stickyChatInterfaceId != -1) {
			this.modalMessage = arg1;
			super.mouseClickButton = 0;
		}
		if (this.chatInterfaceId == -1) {
			this.redrawChatback = true;
		}
		for (int var5 = 99; var5 > 0; var5--) {
			this.messageType[var5] = this.messageType[var5 - 1];
			this.messageSender[var5] = this.messageSender[var5 - 1];
			this.messageText[var5] = this.messageText[var5 - 1];
		}
		this.messageType[0] = arg0;
		this.messageSender[0] = arg3;
		if (arg2 != 4) {
			this.field1277 = !this.field1277;
		}
		this.messageText[0] = arg1;
	}

	@ObfuscatedName("client.i(II)V")
	public final void resetInterfaceAnimation(int arg0, int arg1) {
		Component var3 = Component.instances[arg1];
		for (int var4 = 0; var4 < var3.childId.length && var3.childId[var4] != -1; var4++) {
			Component var5 = Component.instances[var3.childId[var4]];
			if (var5.type == 1) {
				this.resetInterfaceAnimation(-321, var5.id);
			}
			var5.seqFrame = 0;
			var5.seqCycle = 0;
		}
		if (arg0 >= 0) {
			this.levelObjStacks = null;
		}
	}

	@ObfuscatedName("client.a(IJ)V")
	public final void removeFriend(int arg0, long arg1) {
		int var4;
		if (arg0 != 43808) {
			for (var4 = 1; var4 > 0; var4++) {
			}
		}
		if (arg1 == 0L) {
			return;
		}
		for (var4 = 0; var4 < this.friendCount; var4++) {
			if (this.friendName37[var4] == arg1) {
				this.friendCount--;
				this.redrawSidebar = true;
				for (int var5 = var4; var5 < this.friendCount; var5++) {
					this.friendName[var5] = this.friendName[var5 + 1];
					this.friendWorld[var5] = this.friendWorld[var5 + 1];
					this.friendName37[var5] = this.friendName37[var5 + 1];
				}
				this.out.p1isaac((byte) -34, 11);
				this.out.p8(true, arg1);
				return;
			}
		}
	}

	@ObfuscatedName("client.a(Lhc;I)Z")
	public final boolean executeInterfaceScript(Component arg0, int arg1) {
		if (arg0.scriptComparator == null) {
			return false;
		}
		for (int var3 = 0; var3 < arg0.scriptComparator.length; var3++) {
			int var4 = this.executeClientscript1(arg0, false, var3);
			int var5 = arg0.scriptOperand[var3];
			if (arg0.scriptComparator[var3] == 2) {
				if (var4 >= var5) {
					return false;
				}
			} else if (arg0.scriptComparator[var3] == 3) {
				if (var4 <= var5) {
					return false;
				}
			} else if (arg0.scriptComparator[var3] == 4) {
				if (var4 == var5) {
					return false;
				}
			} else if (var4 != var5) {
				return false;
			}
		}
		if (arg1 <= 0) {
			this.out.p1(82);
		}
		return true;
	}

	@ObfuscatedName("client.l(B)V")
	public final void handleMinimapInput(byte arg0) {
		if (arg0 != this.field1433) {
			this.levelObjStacks = null;
		}
		if (super.mouseClickButton != 1) {
			return;
		}
		int var2 = super.mouseClickX - 21 - 561;
		int var3 = super.mouseClickY - 9 - 5;
		if (var2 < 0 || var3 < 0 || var2 >= 146 || var3 >= 151) {
			return;
		}
		var2 -= 73;
		var3 -= 75;
		int var4 = this.orbitCameraYaw + this.minimapAnticheatAngle & 0x7FF;
		int var5 = Pix3D.sinTable[var4];
		int var6 = Pix3D.cosTable[var4];
		int var12 = var5 * (this.minimapZoom + 256) >> 8;
		int var13 = var6 * (this.minimapZoom + 256) >> 8;
		int var7 = var3 * var12 + var2 * var13 >> 11;
		int var8 = var3 * var13 - var2 * var12 >> 11;
		int var9 = this.localPlayer.x + var7 >> 7;
		int var10 = this.localPlayer.z - var8 >> 7;
		boolean var11 = this.tryMove(this.localPlayer.pathTileX[0], 0, true, var9, this.localPlayer.pathTileZ[0], 0, 1, 0, var10, 0, 0, 0);
		if (!var11) {
			return;
		}
		this.out.p1(var2);
		this.out.p1(var3);
		this.out.p2(this.orbitCameraYaw);
		this.out.p1(57);
		this.out.p1(this.minimapAnticheatAngle);
		this.out.p1(this.minimapZoom);
		this.out.p1(89);
		this.out.p2(this.localPlayer.x);
		this.out.p2(this.localPlayer.z);
		this.out.p1(this.tryMoveNearest);
		this.out.p1(63);
		return;
	}

	@ObfuscatedName("client.m(B)V")
	public final void handleMouseInput(byte arg0) {
		if (arg0 != 3) {
			this.packetType = this.in.g1();
		}
		if (this.objDragArea != 0) {
			return;
		}
		int var2 = super.mouseClickButton;
		if (this.spellSelected == 1 && super.mouseClickX >= 520 && super.mouseClickY >= 165 && super.mouseClickX <= 788 && super.mouseClickY <= 230) {
			var2 = 0;
		}
		int var3;
		int var4;
		int var5;
		if (!this.menuVisible) {
			if (var2 == 1 && this.menuSize > 0) {
				var3 = this.menuAction[this.menuSize - 1];
				if (var3 == 602 || var3 == 596 || var3 == 22 || var3 == 892 || var3 == 415 || var3 == 405 || var3 == 38 || var3 == 422 || var3 == 478 || var3 == 347 || var3 == 188) {
					var4 = this.menuParamB[this.menuSize - 1];
					var5 = this.menuParamC[this.menuSize - 1];
					Component var6 = Component.instances[var5];
					if (var6.draggable) {
						this.objGrabThreshold = false;
						this.objDragCycles = 0;
						this.objDragInterfaceId = var5;
						this.objDragSlot = var4;
						this.objDragArea = 2;
						this.objGrabX = super.mouseClickX;
						this.objGrabY = super.mouseClickY;
						if (Component.instances[var5].layer == this.viewportInterfaceId) {
							this.objDragArea = 1;
						}
						if (Component.instances[var5].layer == this.chatInterfaceId) {
							this.objDragArea = 3;
						}
						return;
					}
				}
			}
			if (var2 == 1 && (this.mouseButtonsOption == 1 || this.isAddFriendOption(145, this.menuSize - 1)) && this.menuSize > 2) {
				var2 = 2;
			}
			if (var2 == 1 && this.menuSize > 0) {
				this.useMenuOption(6412, this.menuSize - 1);
			}
			if (var2 != 2 || this.menuSize <= 0) {
				return;
			}
			this.showContextMenu(-386);
			return;
		}
		if (var2 != 1) {
			var3 = super.mouseX;
			var4 = super.mouseY;
			if (this.menuArea == 0) {
				var3 -= 8;
				var4 -= 11;
			}
			if (this.menuArea == 1) {
				var3 -= 562;
				var4 -= 231;
			}
			if (this.menuArea == 2) {
				var3 -= 22;
				var4 -= 375;
			}
			if (var3 < this.menuX - 10 || var3 > this.menuX + this.menuWidth + 10 || var4 < this.menuY - 10 || var4 > this.menuY + this.menuHeight + 10) {
				this.menuVisible = false;
				if (this.menuArea == 1) {
					this.redrawSidebar = true;
				}
				if (this.menuArea == 2) {
					this.redrawChatback = true;
				}
			}
		}
		if (var2 != 1) {
			return;
		}
		var3 = this.menuX;
		var4 = this.menuY;
		var5 = this.menuWidth;
		int var11 = super.mouseClickX;
		int var7 = super.mouseClickY;
		if (this.menuArea == 0) {
			var11 -= 8;
			var7 -= 11;
		}
		if (this.menuArea == 1) {
			var11 -= 562;
			var7 -= 231;
		}
		if (this.menuArea == 2) {
			var11 -= 22;
			var7 -= 375;
		}
		int var8 = -1;
		for (int var9 = 0; var9 < this.menuSize; var9++) {
			int var10 = var4 + 31 + (this.menuSize - 1 - var9) * 15;
			if (var11 > var3 && var11 < var3 + var5 && var7 > var10 - 13 && var7 < var10 + 3) {
				var8 = var9;
			}
		}
		if (var8 != -1) {
			this.useMenuOption(6412, var8);
		}
		this.menuVisible = false;
		if (this.menuArea == 1) {
			this.redrawSidebar = true;
		}
		if (this.menuArea == 2) {
			this.redrawChatback = true;
			return;
		}
	}

	@ObfuscatedName("client.n(B)V")
	public final void applyCutscene(byte arg0) {
		int var2 = this.cutsceneSrcLocalTileX * 128 + 64;
		int var3 = this.cutsceneSrcLocalTileZ * 128 + 64;
		int var4 = this.getHeightmapY(this.currentLevel, this.cutsceneSrcLocalTileX, (byte) 5, this.cutsceneSrcLocalTileZ) - this.cutsceneSrcHeight;
		if (this.cameraX < var2) {
			this.cameraX += this.cutsceneMoveSpeed + (var2 - this.cameraX) * this.cutsceneMoveAcceleration / 1000;
			if (this.cameraX > var2) {
				this.cameraX = var2;
			}
		}
		if (this.cameraX > var2) {
			this.cameraX -= this.cutsceneMoveSpeed + (this.cameraX - var2) * this.cutsceneMoveAcceleration / 1000;
			if (this.cameraX < var2) {
				this.cameraX = var2;
			}
		}
		if (this.cameraY < var4) {
			this.cameraY += this.cutsceneMoveSpeed + (var4 - this.cameraY) * this.cutsceneMoveAcceleration / 1000;
			if (this.cameraY > var4) {
				this.cameraY = var4;
			}
		}
		if (this.cameraY > var4) {
			this.cameraY -= this.cutsceneMoveSpeed + (this.cameraY - var4) * this.cutsceneMoveAcceleration / 1000;
			if (this.cameraY < var4) {
				this.cameraY = var4;
			}
		}
		if (this.cameraZ < var3) {
			this.cameraZ += this.cutsceneMoveSpeed + (var3 - this.cameraZ) * this.cutsceneMoveAcceleration / 1000;
			if (this.cameraZ > var3) {
				this.cameraZ = var3;
			}
		}
		if (this.cameraZ > var3) {
			this.cameraZ -= this.cutsceneMoveSpeed + (this.cameraZ - var3) * this.cutsceneMoveAcceleration / 1000;
			if (this.cameraZ < var3) {
				this.cameraZ = var3;
			}
		}
		var2 = this.cutsceneDstLocalTileX * 128 + 64;
		var3 = this.cutsceneDstLocalTileZ * 128 + 64;
		var4 = this.getHeightmapY(this.currentLevel, this.cutsceneDstLocalTileX, (byte) 5, this.cutsceneDstLocalTileZ) - this.cutsceneDstHeight;
		int var5 = var2 - this.cameraX;
		int var6 = var4 - this.cameraY;
		int var7 = var3 - this.cameraZ;
		int var8 = (int) Math.sqrt((double) (var5 * var5 + var7 * var7));
		int var9 = (int) (Math.atan2((double) var6, (double) var8) * 325.949D) & 0x7FF;
		int var10 = (int) (Math.atan2((double) var5, (double) var7) * -325.949D) & 0x7FF;
		if (arg0 != 2) {
			return;
		}
		if (var9 < 128) {
			var9 = 128;
		}
		if (var9 > 383) {
			var9 = 383;
		}
		if (this.cameraPitch < var9) {
			this.cameraPitch += this.cutsceneRotateSpeed + (var9 - this.cameraPitch) * this.cutsceneRotateAcceleration / 1000;
			if (this.cameraPitch > var9) {
				this.cameraPitch = var9;
			}
		}
		if (this.cameraPitch > var9) {
			this.cameraPitch -= this.cutsceneRotateSpeed + (this.cameraPitch - var9) * this.cutsceneRotateAcceleration / 1000;
			if (this.cameraPitch < var9) {
				this.cameraPitch = var9;
			}
		}
		int var11 = var10 - this.cameraYaw;
		if (var11 > 1024) {
			var11 -= 2048;
		}
		if (var11 < -1024) {
			var11 += 2048;
		}
		if (var11 > 0) {
			this.cameraYaw += this.cutsceneRotateSpeed + var11 * this.cutsceneRotateAcceleration / 1000;
			this.cameraYaw &= 0x7FF;
		}
		if (var11 < 0) {
			this.cameraYaw -= this.cutsceneRotateSpeed + -var11 * this.cutsceneRotateAcceleration / 1000;
			this.cameraYaw &= 0x7FF;
		}
		int var12 = var10 - this.cameraYaw;
		if (var12 > 1024) {
			var12 -= 2048;
		}
		if (var12 < -1024) {
			var12 += 2048;
		}
		if (var12 < 0 && var11 > 0 || var12 > 0 && var11 < 0) {
			this.cameraYaw = var10;
		}
	}

	@ObfuscatedName("client.C(I)V")
	public final void handleTabInput(int arg0) {
		if (arg0 != 29286) {
			this.field1136 = this.randomIn.takeNextValue();
		}
		if (super.mouseClickButton != 1) {
			return;
		}
		if (super.mouseClickX >= 549 && super.mouseClickX <= 583 && super.mouseClickY >= 195 && super.mouseClickY < 231 && this.tabInterfaceId[0] != -1) {
			this.redrawSidebar = true;
			this.selectedTab = 0;
			this.redrawSideicons = true;
		}
		if (super.mouseClickX >= 579 && super.mouseClickX <= 609 && super.mouseClickY >= 194 && super.mouseClickY < 231 && this.tabInterfaceId[1] != -1) {
			this.redrawSidebar = true;
			this.selectedTab = 1;
			this.redrawSideicons = true;
		}
		if (super.mouseClickX >= 607 && super.mouseClickX <= 637 && super.mouseClickY >= 194 && super.mouseClickY < 231 && this.tabInterfaceId[2] != -1) {
			this.redrawSidebar = true;
			this.selectedTab = 2;
			this.redrawSideicons = true;
		}
		if (super.mouseClickX >= 635 && super.mouseClickX <= 679 && super.mouseClickY >= 194 && super.mouseClickY < 229 && this.tabInterfaceId[3] != -1) {
			this.redrawSidebar = true;
			this.selectedTab = 3;
			this.redrawSideicons = true;
		}
		if (super.mouseClickX >= 676 && super.mouseClickX <= 706 && super.mouseClickY >= 194 && super.mouseClickY < 231 && this.tabInterfaceId[4] != -1) {
			this.redrawSidebar = true;
			this.selectedTab = 4;
			this.redrawSideicons = true;
		}
		if (super.mouseClickX >= 704 && super.mouseClickX <= 734 && super.mouseClickY >= 194 && super.mouseClickY < 231 && this.tabInterfaceId[5] != -1) {
			this.redrawSidebar = true;
			this.selectedTab = 5;
			this.redrawSideicons = true;
		}
		if (super.mouseClickX >= 732 && super.mouseClickX <= 766 && super.mouseClickY >= 195 && super.mouseClickY < 231 && this.tabInterfaceId[6] != -1) {
			this.redrawSidebar = true;
			this.selectedTab = 6;
			this.redrawSideicons = true;
		}
		if (super.mouseClickX >= 550 && super.mouseClickX <= 584 && super.mouseClickY >= 492 && super.mouseClickY < 528 && this.tabInterfaceId[7] != -1) {
			this.redrawSidebar = true;
			this.selectedTab = 7;
			this.redrawSideicons = true;
		}
		if (super.mouseClickX >= 582 && super.mouseClickX <= 612 && super.mouseClickY >= 492 && super.mouseClickY < 529 && this.tabInterfaceId[8] != -1) {
			this.redrawSidebar = true;
			this.selectedTab = 8;
			this.redrawSideicons = true;
		}
		if (super.mouseClickX >= 609 && super.mouseClickX <= 639 && super.mouseClickY >= 492 && super.mouseClickY < 529 && this.tabInterfaceId[9] != -1) {
			this.redrawSidebar = true;
			this.selectedTab = 9;
			this.redrawSideicons = true;
		}
		if (super.mouseClickX >= 637 && super.mouseClickX <= 681 && super.mouseClickY >= 493 && super.mouseClickY < 528 && this.tabInterfaceId[10] != -1) {
			this.redrawSidebar = true;
			this.selectedTab = 10;
			this.redrawSideicons = true;
		}
		if (super.mouseClickX >= 679 && super.mouseClickX <= 709 && super.mouseClickY >= 492 && super.mouseClickY < 529 && this.tabInterfaceId[11] != -1) {
			this.redrawSidebar = true;
			this.selectedTab = 11;
			this.redrawSideicons = true;
		}
		if (super.mouseClickX >= 706 && super.mouseClickX <= 736 && super.mouseClickY >= 492 && super.mouseClickY < 529 && this.tabInterfaceId[12] != -1) {
			this.redrawSidebar = true;
			this.selectedTab = 12;
			this.redrawSideicons = true;
		}
		if (super.mouseClickX >= 734 && super.mouseClickX <= 768 && super.mouseClickY >= 492 && super.mouseClickY < 528 && this.tabInterfaceId[13] != -1) {
			this.redrawSidebar = true;
			this.selectedTab = 13;
			this.redrawSideicons = true;
		}
		cyclelogic1++;
		if (cyclelogic1 > 150) {
			cyclelogic1 = 0;
			this.out.p1isaac((byte) -34, 233);
			this.out.p1(43);
			return;
		}
	}

	@ObfuscatedName("client.a(Lhc;B)Z")
	public final boolean handleSocialMenuOption(Component arg0, byte arg1) {
		int var3 = arg0.clientCode;
		if (arg1 != this.field1155) {
			this.field1404 = this.randomIn.takeNextValue();
		}
		if (var3 >= 1 && var3 <= 200) {
			if (var3 >= 101) {
				var3 -= 101;
			} else {
				var3--;
			}
			this.menuOption[this.menuSize] = "Remove @whi@" + this.friendName[var3];
			this.menuAction[this.menuSize] = 557;
			this.menuSize++;
			this.menuOption[this.menuSize] = "Message @whi@" + this.friendName[var3];
			this.menuAction[this.menuSize] = 679;
			this.menuSize++;
			return true;
		} else if (var3 >= 401 && var3 <= 500) {
			this.menuOption[this.menuSize] = "Remove @whi@" + arg0.text;
			this.menuAction[this.menuSize] = 556;
			this.menuSize++;
			return true;
		} else {
			return false;
		}
	}

	@ObfuscatedName("client.b(ILkb;I)V")
	public final void getNpcPosOldVis(int arg0, Packet arg1, int arg2) {
		if (arg0 >= 0) {
			return;
		}
		arg1.accessBits(223);
		int var4 = arg1.gBit(9, 8);
		int var5;
		if (var4 < this.npcCount) {
			for (var5 = var4; var5 < this.npcCount; var5++) {
				this.entityRemovalIds[this.entityRemovalCount++] = this.npcIds[var5];
			}
		}
		if (var4 > this.npcCount) {
			signlink.reporterror(this.username + " Too many npcs");
			throw new RuntimeException("eek");
		}
		this.npcCount = 0;
		for (var5 = 0; var5 < var4; var5++) {
			int var6 = this.npcIds[var5];
			NpcEntity var7 = this.npcs[var6];
			int var8 = arg1.gBit(9, 1);
			if (var8 == 0) {
				this.npcIds[this.npcCount++] = var6;
				var7.cycle = loopCycle;
			} else {
				int var9 = arg1.gBit(9, 2);
				if (var9 == 0) {
					this.npcIds[this.npcCount++] = var6;
					var7.cycle = loopCycle;
					this.entityUpdateIds[this.entityUpdateCount++] = var6;
				} else {
					int var10;
					int var11;
					if (var9 == 1) {
						this.npcIds[this.npcCount++] = var6;
						var7.cycle = loopCycle;
						var10 = arg1.gBit(9, 3);
						var7.moveAlongRoute(false, var10, (byte) 6);
						var11 = arg1.gBit(9, 1);
						if (var11 == 1) {
							this.entityUpdateIds[this.entityUpdateCount++] = var6;
						}
					} else if (var9 == 2) {
						this.npcIds[this.npcCount++] = var6;
						var7.cycle = loopCycle;
						var10 = arg1.gBit(9, 3);
						var7.moveAlongRoute(true, var10, (byte) 6);
						var11 = arg1.gBit(9, 3);
						var7.moveAlongRoute(true, var11, (byte) 6);
						int var12 = arg1.gBit(9, 1);
						if (var12 == 1) {
							this.entityUpdateIds[this.entityUpdateCount++] = var6;
						}
					} else if (var9 == 3) {
						this.entityRemovalIds[this.entityRemovalCount++] = var6;
					}
				}
			}
		}
	}

	public final String getParameter(String arg0) {
		return signlink.mainapp == null ? super.getParameter(arg0) : signlink.mainapp.getParameter(arg0);
	}

	@ObfuscatedName("client.l(Z)V")
	public final void tryReconnect(boolean arg0) {
		if (this.idleTimeout > 0) {
			this.logout(-780);
			return;
		}
		this.areaViewport.bind((byte) 62);
		if (arg0) {
			this.load();
		}
		this.fontPlain12.drawStringCenter(144, (byte) 6, 0, "Connection lost", 257);
		this.fontPlain12.drawStringCenter(143, (byte) 6, 16777215, "Connection lost", 256);
		this.fontPlain12.drawStringCenter(159, (byte) 6, 0, "Please wait - attempting to reestablish", 257);
		this.fontPlain12.drawStringCenter(158, (byte) 6, 16777215, "Please wait - attempting to reestablish", 256);
		this.areaViewport.draw(11, super.graphics, 8, 5193);
		this.flagSceneTileX = 0;
		ClientStream var2 = this.stream;
		this.ingame = false;
		this.login(this.username, this.password, true);
		if (!this.ingame) {
			this.logout(-780);
		}
		try {
			var2.close();
		} catch (Exception var3) {
		}
	}

	@ObfuscatedName("client.a(ILib;)V")
	public final void updateFlameBuffer(int arg0, Pix8 arg1) {
		short var3 = 256;
		for (int var4 = 0; var4 < this.flameBuffer0.length; var4++) {
			this.flameBuffer0[var4] = 0;
		}
		int var6;
		for (int var5 = 0; var5 < 5000; var5++) {
			var6 = (int) (Math.random() * 128.0D * (double) var3);
			this.flameBuffer0[var6] = (int) (Math.random() * 256.0D);
		}
		int var7;
		int var8;
		int var9;
		for (var6 = 0; var6 < 20; var6++) {
			for (var7 = 1; var7 < var3 - 1; var7++) {
				for (var8 = 1; var8 < 127; var8++) {
					var9 = var8 + (var7 << 7);
					this.flameBuffer1[var9] = (this.flameBuffer0[var9 - 1] + this.flameBuffer0[var9 + 1] + this.flameBuffer0[var9 - 128] + this.flameBuffer0[var9 + 128]) / 4;
				}
			}
			int[] var13 = this.flameBuffer0;
			this.flameBuffer0 = this.flameBuffer1;
			this.flameBuffer1 = var13;
		}
		if (arg0 <= 0) {
			this.field1277 = !this.field1277;
		}
		if (arg1 == null) {
			return;
		}
		var7 = 0;
		for (var8 = 0; var8 < arg1.height; var8++) {
			for (var9 = 0; var9 < arg1.width; var9++) {
				if (arg1.pixels[var7++] != 0) {
					int var10 = var9 + 16 + arg1.cropX;
					int var11 = var8 + 16 + arg1.cropY;
					int var12 = var10 + (var11 << 7);
					this.flameBuffer0[var12] = 0;
				}
			}
		}
	}

	@ObfuscatedName("client.j(II)V")
	public final void sortObjStacks(int arg0, int arg1) {
		LinkList var3 = this.levelObjStacks[this.currentLevel][arg0][arg1];
		if (var3 == null) {
			this.scene.removeObjStack(this.currentLevel, arg0, arg1);
			return;
		}
		int var4 = -99999999;
		ObjStackEntity var5 = null;
		ObjStackEntity var6;
		int var8;
		for (var6 = (ObjStackEntity) var3.head(); var6 != null; var6 = (ObjStackEntity) var3.next(551)) {
			ObjType var7 = ObjType.get(var6.index);
			var8 = var7.cost;
			if (var7.stackable) {
				var8 *= var6.count + 1;
			}
			if (var8 > var4) {
				var4 = var8;
				var5 = var6;
			}
		}
		var3.addHead(var5, -26173);
		int var15 = -1;
		var8 = -1;
		int var9 = 0;
		int var10 = 0;
		for (var6 = (ObjStackEntity) var3.head(); var6 != null; var6 = (ObjStackEntity) var3.next(551)) {
			if (var6.index != var5.index && var15 == -1) {
				var15 = var6.index;
				var9 = var6.count;
			}
			if (var6.index != var5.index && var6.index != var15 && var8 == -1) {
				var8 = var6.index;
				var10 = var6.count;
			}
		}
		Model var11 = null;
		if (var15 != -1) {
			var11 = ObjType.get(var15).getInterfaceModel(var9);
		}
		Model var12 = null;
		if (var8 != -1) {
			var12 = ObjType.get(var8).getInterfaceModel(var10);
		}
		int var13 = arg0 + (arg1 << 7) + 1610612736;
		ObjType var14 = ObjType.get(var5.index);
		this.scene.addObjStack(var14.getInterfaceModel(var5.count), var11, this.getHeightmapY(this.currentLevel, arg0 * 128 + 64, (byte) 5, arg1 * 128 + 64), this.currentLevel, var13, arg1, arg0, var12, 429);
	}

	@ObfuscatedName("client.D(I)V")
	public final void buildScene(int arg0) {
		try {
			this.minimapLevel = -1;
			this.mergedLocations.clear();
			this.locList.clear();
			this.spotanims.clear();
			this.projectiles.clear();
			Pix3D.clearTexels(false);
			this.clearCaches(this.field1208);
			this.scene.reset(742);
			for (int var2 = 0; var2 < 4; var2++) {
				this.levelCollisionMap[var2].reset((byte) 74);
			}
			System.gc();
			World var3 = new World(104, this.levelTileFlags, 104, this.levelHeightmap, this.field1471);
			byte[] var4 = new byte[100000];
			int var5 = this.sceneMapLandData.length;
			World.lowMemory = World3D.lowMemory;
			int var7;
			int var8;
			for (int var6 = 0; var6 < var5; var6++) {
				var7 = this.sceneMapIndex[var6] >> 8;
				var8 = this.sceneMapIndex[var6] & 0xFF;
				if (var7 == 33 && var8 >= 71 && var8 <= 73) {
					World.lowMemory = false;
				}
			}
			if (World.lowMemory) {
				this.scene.setMinLevel(0, this.currentLevel);
			} else {
				this.scene.setMinLevel(0, 0);
			}
			this.out.p1isaac((byte) -34, 108);
			int var11;
			for (var7 = 0; var7 < var5; var7++) {
				var8 = (this.sceneMapIndex[var7] >> 8) * 64 - this.sceneBaseTileX;
				int var9 = (this.sceneMapIndex[var7] & 0xFF) * 64 - this.sceneBaseTileZ;
				byte[] var10 = this.sceneMapLandData[var7];
				if (var10 != null) {
					var11 = (new Packet(363, var10)).g4();
					BZip2.read(var4, var11, var10, var10.length - 4, 4);
					var3.loadGround(var4, (this.sceneCenterZoneX - 6) * 8, 1, var9, var8, (this.sceneCenterZoneZ - 6) * 8);
				} else if (this.sceneCenterZoneZ < 800) {
					var3.clearLandscape(var8, var9, 3, 64, 64);
				}
			}
			this.out.p1isaac((byte) -34, 108);
			int var16;
			for (var8 = 0; var8 < var5; var8++) {
				byte[] var14 = this.sceneMapLocData[var8];
				if (var14 != null) {
					var16 = (new Packet(363, var14)).g4();
					BZip2.read(var4, var16, var14, var14.length - 4, 4);
					var11 = (this.sceneMapIndex[var8] >> 8) * 64 - this.sceneBaseTileX;
					int var12 = (this.sceneMapIndex[var8] & 0xFF) * 64 - this.sceneBaseTileZ;
					var3.loadLocations(var4, this.scene, this.levelCollisionMap, this.locList, true, var12, var11);
				}
			}
			this.out.p1isaac((byte) -34, 108);
			var3.build(this.scene, -270, this.levelCollisionMap);
			this.areaViewport.bind((byte) 62);
			this.out.p1isaac((byte) -34, 108);
			for (LocEntity var15 = (LocEntity) this.locList.head(); var15 != null; var15 = (LocEntity) this.locList.next(551)) {
				if ((this.levelTileFlags[1][var15.x][var15.z] & 0x2) == 2) {
					var15.level--;
					if (var15.level < 0) {
						var15.unlink();
					}
				}
			}
			for (var16 = 0; var16 < 104; var16++) {
				for (var11 = 0; var11 < 104; var11++) {
					this.sortObjStacks(var16, var11);
				}
			}
			for (LocAddEntity var17 = (LocAddEntity) this.spawnedLocations.head(); var17 != null; var17 = (LocAddEntity) this.spawnedLocations.next(551)) {
				this.addLoc(var17.angle, var17.x, var17.z, var17.layer, var17.locIndex, var17.shape, -27819, var17.plane);
			}
		} catch (Exception var13) {
		}
		LocType.modelCacheStatic.clear();
		if (arg0 <= 0) {
			this.field1512 = !this.field1512;
		}
		System.gc();
		Pix3D.initPool(20, -20);
	}

	@ObfuscatedName("client.b(I)V")
	public final void update(int arg0) {
        Runnable task=referenceTask; if(task!=null) { referenceTask=null; task.run(); }
		if (this.errorStarted || this.errorLoading || this.errorHost) {
			return;
		}
		loopCycle++;
		if (arg0 <= 0) {
			return;
		}
		if (this.ingame) {
			this.updateGame(true);
		} else {
			this.updateTitle((byte) -70);
		}
	}

	@ObfuscatedName("client.o(B)V")
	public final void updateEntityChats(byte arg0) {
		if (arg0 != 94) {
			this.load();
		}
		int var3;
		for (int var2 = -1; var2 < this.playerCount; var2++) {
			if (var2 == -1) {
				var3 = this.LOCAL_PLAYER_INDEX;
			} else {
				var3 = this.playerIds[var2];
			}
			PlayerEntity var4 = this.players[var3];
			if (var4 != null && var4.chatTimer > 0) {
				var4.chatTimer--;
				if (var4.chatTimer == 0) {
					var4.chat = null;
				}
			}
		}
		for (var3 = 0; var3 < this.npcCount; var3++) {
			int var6 = this.npcIds[var3];
			NpcEntity var5 = this.npcs[var6];
			if (var5 != null && var5.chatTimer > 0) {
				var5.chatTimer--;
				if (var5.chatTimer == 0) {
					var5.chat = null;
				}
			}
		}
	}

	@ObfuscatedName("client.a(Lhc;ZI)I")
	public final int executeClientscript1(Component arg0, boolean arg1, int arg2) {
		if (arg1) {
			this.field1214 = !this.field1214;
		}
		if (arg0.scripts == null || arg2 >= arg0.scripts.length) {
			return -2;
		}
		try {
			int[] var4 = arg0.scripts[arg2];
			int var5 = 0;
			int var6 = 0;
			while (true) {
				int var7 = var4[var6++];
				if (var7 == 0) {
					return var5;
				}
				if (var7 == 1) {
					var5 += this.skillLevel[var4[var6++]];
				}
				if (var7 == 2) {
					var5 += this.skillBaseLevel[var4[var6++]];
				}
				if (var7 == 3) {
					var5 += this.skillExperience[var4[var6++]];
				}
				Component var8;
				int var9;
				int var10;
				if (var7 == 4) {
					var8 = Component.instances[var4[var6++]];
					var9 = var4[var6++] + 1;
					for (var10 = 0; var10 < var8.invSlotObjId.length; var10++) {
						if (var8.invSlotObjId[var10] == var9) {
							var5 += var8.invSlotObjCount[var10];
						}
					}
				}
				if (var7 == 5) {
					var5 += this.varps[var4[var6++]];
				}
				if (var7 == 6) {
					var5 += levelExperience[this.skillBaseLevel[var4[var6++]] - 1];
				}
				if (var7 == 7) {
					var5 += this.varps[var4[var6++]] * 100 / 46875;
				}
				if (var7 == 8) {
					var5 += this.localPlayer.combatLevel;
				}
				int var12;
				if (var7 == 9) {
					for (var12 = 0; var12 < 19; var12++) {
						if (var12 == 18) {
							var12 = 20;
						}
						var5 += this.skillBaseLevel[var12];
					}
				}
				if (var7 == 10) {
					var8 = Component.instances[var4[var6++]];
					var9 = var4[var6++] + 1;
					for (var10 = 0; var10 < var8.invSlotObjId.length; var10++) {
						if (var8.invSlotObjId[var10] == var9) {
							var5 += 999999999;
							break;
						}
					}
				}
				if (var7 == 11) {
					var5 += this.energy;
				}
				if (var7 == 12) {
					var5 += this.weightCarried;
				}
				if (var7 == 13) {
					var12 = this.varps[var4[var6++]];
					var9 = var4[var6++];
					var5 += (var12 & 0x1 << var9) == 0 ? 0 : 1;
				}
			}
		} catch (Exception var11) {
			return -1;
		}
	}

	@ObfuscatedName("client.m(Z)V")
	public final void drawError(boolean arg0) {
		Graphics var2 = this.getBaseComponent(field1481).getGraphics();
		var2.setColor(Color.black);
		var2.fillRect(0, 0, 789, 532);
		this.setFramerate(386, 1);
		if (arg0) {
			this.load();
		}
		byte var3;
		int var4;
		if (this.errorLoading) {
			this.flameActive = false;
			var2.setFont(new Font("Helvetica", 1, 16));
			var2.setColor(Color.yellow);
			var3 = 35;
			var2.drawString("Sorry, an error has occured whilst loading RuneScape", 30, var3);
			var4 = var3 + 50;
			var2.setColor(Color.white);
			var2.drawString("To fix this try the following (in order):", 30, var4);
			int var5 = var4 + 50;
			var2.setColor(Color.white);
			var2.setFont(new Font("Helvetica", 1, 12));
			var2.drawString("1: Try closing ALL open web-browser windows, and reloading", 30, var5);
			int var6 = var5 + 30;
			var2.drawString("2: Try clearing your web-browsers cache from tools->internet options", 30, var6);
			int var7 = var6 + 30;
			var2.drawString("3: Try using a different game-world", 30, var7);
			int var8 = var7 + 30;
			var2.drawString("4: Try rebooting your computer", 30, var8);
			int var9 = var8 + 30;
			var2.drawString("5: Try selecting a different version of Java from the play-game menu", 30, var9);
		}
		if (this.errorHost) {
			this.flameActive = false;
			var2.setFont(new Font("Helvetica", 1, 20));
			var2.setColor(Color.white);
			var2.drawString("Error - unable to load game!", 50, 50);
			var2.drawString("To play RuneScape make sure you play from", 50, 100);
			var2.drawString("http://www.runescape.com", 50, 150);
		}
		if (!this.errorStarted) {
			return;
		}
		this.flameActive = false;
		var2.setColor(Color.yellow);
		var3 = 35;
		var2.drawString("Error a copy of RuneScape already appears to be loaded", 30, var3);
		var4 = var3 + 50;
		var2.setColor(Color.white);
		var2.drawString("To fix this try the following (in order):", 30, var4);
		var4 += 50;
		var2.setColor(Color.white);
		var2.setFont(new Font("Helvetica", 1, 12));
		var2.drawString("1: Try closing ALL open web-browser windows, and reloading", 30, var4);
		var4 += 30;
		var2.drawString("2: Try rebooting your computer, and reloading", 30, var4);
		var4 += 30;
	}

	@ObfuscatedName("client.p(B)V")
	public final void loadTitleBackground(byte arg0) {
		byte[] var2 = this.archiveTitle.read("title.dat", null, (byte) 2);
		Pix32 var3 = new Pix32(var2, this);
		this.imageTitle0.bind((byte) 62);
		var3.blitOpaque(34676, 0, 0);
		this.imageTitle1.bind((byte) 62);
		var3.blitOpaque(34676, -661, 0);
		this.imageTitle2.bind((byte) 62);
		var3.blitOpaque(34676, -128, 0);
		this.imageTitle3.bind((byte) 62);
		var3.blitOpaque(34676, -214, -386);
		this.imageTitle4.bind((byte) 62);
		var3.blitOpaque(34676, -214, -186);
		this.imageTitle5.bind((byte) 62);
		var3.blitOpaque(34676, 0, -265);
		this.imageTitle6.bind((byte) 62);
		var3.blitOpaque(34676, -574, -265);
		this.imageTitle7.bind((byte) 62);
		if (arg0 != 5) {
			this.field1277 = !this.field1277;
		}
		var3.blitOpaque(34676, -128, -186);
		this.imageTitle8.bind((byte) 62);
		var3.blitOpaque(34676, -574, -186);
		int[] var4 = new int[var3.width];
		for (int var5 = 0; var5 < var3.height; var5++) {
			for (int var6 = 0; var6 < var3.width; var6++) {
				var4[var6] = var3.pixels[var3.width - var6 - 1 + var3.width * var5];
			}
			for (int var7 = 0; var7 < var3.width; var7++) {
				var3.pixels[var7 + var3.width * var5] = var4[var7];
			}
		}
		this.imageTitle0.bind((byte) 62);
		var3.blitOpaque(34676, 394, 0);
		this.imageTitle1.bind((byte) 62);
		var3.blitOpaque(34676, -267, 0);
		this.imageTitle2.bind((byte) 62);
		var3.blitOpaque(34676, 266, 0);
		this.imageTitle3.bind((byte) 62);
		var3.blitOpaque(34676, 180, -386);
		this.imageTitle4.bind((byte) 62);
		var3.blitOpaque(34676, 180, -186);
		this.imageTitle5.bind((byte) 62);
		var3.blitOpaque(34676, 394, -265);
		this.imageTitle6.bind((byte) 62);
		var3.blitOpaque(34676, -180, -265);
		this.imageTitle7.bind((byte) 62);
		var3.blitOpaque(34676, 212, -186);
		this.imageTitle8.bind((byte) 62);
		var3.blitOpaque(34676, -180, -186);
		var3 = new Pix32(this.archiveTitle, "logo", 0);
		this.imageTitle2.bind((byte) 62);
		var3.draw(18, super.screenWidth / 2 - var3.width / 2 - 128, false);
		var3 = null;
		Object var8 = null;
		Object var9 = null;
		System.gc();
	}

	@ObfuscatedName("client.E(I)V")
	public final void pushLocs(int arg0) {
		int var20 = 30 / arg0;
		for (LocEntity var2 = (LocEntity) this.locList.head(); var2 != null; var2 = (LocEntity) this.locList.next(551)) {
			boolean var3 = false;
			var2.seqCycle += this.sceneDelta;
			if (var2.seqFrame == -1) {
				var2.seqFrame = 0;
				var3 = true;
			}
			label67: {
				do {
					do {
						if (var2.seqCycle <= var2.seq.delay[var2.seqFrame]) {
							break label67;
						}
						var2.seqCycle -= var2.seq.delay[var2.seqFrame] + 1;
						var2.seqFrame++;
						var3 = true;
					} while (var2.seqFrame < var2.seq.frameCount);
					var2.seqFrame -= var2.seq.replayoff;
				} while (var2.seqFrame >= 0 && var2.seqFrame < var2.seq.frameCount);
				var2.unlink();
				var3 = false;
			}
			if (var3) {
				int var4 = var2.level;
				int var5 = var2.x;
				int var6 = var2.z;
				int var7 = 0;
				if (var2.type == 0) {
					var7 = this.scene.getWallBitset(var4, var5, var6);
				}
				if (var2.type == 1) {
					var7 = this.scene.getWallDecorationBitset(var4, var6, 3, var5);
				}
				if (var2.type == 2) {
					var7 = this.scene.getLocBitset(var4, var5, var6);
				}
				if (var2.type == 3) {
					var7 = this.scene.getGroundDecorationBitset(var4, var5, var6);
				}
				if (var7 != 0 && (var7 >> 14 & 0x7FFF) == var2.index) {
					int var8 = this.levelHeightmap[var4][var5][var6];
					int var9 = this.levelHeightmap[var4][var5 + 1][var6];
					int var10 = this.levelHeightmap[var4][var5 + 1][var6 + 1];
					int var11 = this.levelHeightmap[var4][var5][var6 + 1];
					LocType var12 = LocType.get(var2.index);
					int var13 = -1;
					if (var2.seqFrame != -1) {
						var13 = var2.seq.frames[var2.seqFrame];
					}
					int var14;
					int var15;
					int var16;
					Model var17;
					if (var2.type == 2) {
						var14 = this.scene.getInfo(var4, var5, var6, var7);
						var15 = var14 & 0x1F;
						var16 = var14 >> 6;
						if (var15 == 11) {
							var15 = 10;
						}
						var17 = var12.getModel(var15, var16, var8, var9, var10, var11, var13);
						this.scene.setLocModel(var5, var17, 1, var4, var6);
					} else if (var2.type == 1) {
						Model var21 = var12.getModel(4, 0, var8, var9, var10, var11, var13);
						this.scene.setWallDecorationModel(266, var6, var5, var21, var4);
					} else if (var2.type == 0) {
						var14 = this.scene.getInfo(var4, var5, var6, var7);
						var15 = var14 & 0x1F;
						var16 = var14 >> 6;
						if (var15 == 2) {
							int var23 = var16 + 1 & 0x3;
							Model var18 = var12.getModel(2, var16 + 4, var8, var9, var10, var11, var13);
							Model var19 = var12.getModel(2, var23, var8, var9, var10, var11, var13);
							this.scene.setWallModels(var18, var19, var6, this.field1214, var5, var4);
						} else {
							var17 = var12.getModel(var15, var16, var8, var9, var10, var11, var13);
							this.scene.setWallModel(35568, var17, var6, var5, var4);
						}
					} else if (var2.type == 3) {
						var14 = this.scene.getInfo(var4, var5, var6, var7);
						var15 = var14 >> 6;
						Model var22 = var12.getModel(22, var15, var8, var9, var10, var11, var13);
						this.scene.setGroundDecorationModel(var22, var6, -48639, var5, var4);
					}
				} else {
					var2.unlink();
				}
			}
		}
	}

	@ObfuscatedName("client.b(IJ)V")
	public final void removeIgnore(int arg0, long arg1) {
		if (arg0 != 1) {
			this.packetType = -1;
		}
		if (arg1 == 0L) {
			return;
		}
		for (int var4 = 0; var4 < this.ignoreCount; var4++) {
			if (this.ignoreName37[var4] == arg1) {
				this.ignoreCount--;
				this.redrawSidebar = true;
				for (int var5 = var4; var5 < this.ignoreCount; var5++) {
					this.ignoreName37[var5] = this.ignoreName37[var5 + 1];
				}
				this.out.p1isaac((byte) -34, 171);
				this.out.p8(true, arg1);
				return;
			}
		}
	}

	@ObfuscatedName("client.q(B)V")
	public final void handleViewportOptions(byte arg0) {
		if (this.objSelected == 0 && this.spellSelected == 0) {
			this.menuOption[this.menuSize] = "Walk here";
			this.menuAction[this.menuSize] = 660;
			this.menuParamB[this.menuSize] = super.mouseX;
			this.menuParamC[this.menuSize] = super.mouseY;
			this.menuSize++;
		}
		int var2 = -1;
		if (arg0 != 2) {
			field1235 = !field1235;
		}
		for (int var3 = 0; var3 < Model.pickedCount; var3++) {
			int var4 = Model.pickedBitsets[var3];
			int var5 = var4 & 0x7F;
			int var6 = var4 >> 7 & 0x7F;
			int var7 = var4 >> 29 & 0x3;
			int var8 = var4 >> 14 & 0x7FFF;
			if (var4 != var2) {
				var2 = var4;
				int var10;
				if (var7 == 2 && this.scene.getInfo(this.currentLevel, var5, var6, var4) >= 0) {
					LocType var9 = LocType.get(var8);
					if (this.objSelected == 1) {
						this.menuOption[this.menuSize] = "Use " + this.objSelectedName + " with @cya@" + var9.name;
						this.menuAction[this.menuSize] = 450;
						this.menuParamA[this.menuSize] = var4;
						this.menuParamB[this.menuSize] = var5;
						this.menuParamC[this.menuSize] = var6;
						this.menuSize++;
					} else if (this.spellSelected != 1) {
						if (var9.op != null) {
							for (var10 = 4; var10 >= 0; var10--) {
								if (var9.op[var10] != null) {
									this.menuOption[this.menuSize] = var9.op[var10] + " @cya@" + var9.name;
									if (var10 == 0) {
										this.menuAction[this.menuSize] = 285;
									}
									if (var10 == 1) {
										this.menuAction[this.menuSize] = 504;
									}
									if (var10 == 2) {
										this.menuAction[this.menuSize] = 364;
									}
									if (var10 == 3) {
										this.menuAction[this.menuSize] = 581;
									}
									if (var10 == 4) {
										this.menuAction[this.menuSize] = 1501;
									}
									this.menuParamA[this.menuSize] = var4;
									this.menuParamB[this.menuSize] = var5;
									this.menuParamC[this.menuSize] = var6;
									this.menuSize++;
								}
							}
						}
						this.menuOption[this.menuSize] = "Examine @cya@" + var9.name;
						this.menuAction[this.menuSize] = 1175;
						this.menuParamA[this.menuSize] = var4;
						this.menuParamB[this.menuSize] = var5;
						this.menuParamC[this.menuSize] = var6;
						this.menuSize++;
					} else if ((this.activeSpellFlags & 0x4) == 4) {
						this.menuOption[this.menuSize] = this.spellCaption + " @cya@" + var9.name;
						this.menuAction[this.menuSize] = 55;
						this.menuParamA[this.menuSize] = var4;
						this.menuParamB[this.menuSize] = var5;
						this.menuParamC[this.menuSize] = var6;
						this.menuSize++;
					}
				}
				NpcEntity var11;
				if (var7 == 1) {
					NpcEntity var13 = this.npcs[var8];
					if (var13.type.size == 1 && (var13.x & 0x7F) == 64 && (var13.z & 0x7F) == 64) {
						for (var10 = 0; var10 < this.npcCount; var10++) {
							var11 = this.npcs[this.npcIds[var10]];
							if (var11 != null && var11 != var13 && var11.type.size == 1 && var11.x == var13.x && var11.z == var13.z) {
								this.addNpcOptions(var11.type, -641, var6, var5, this.npcIds[var10]);
							}
						}
					}
					this.addNpcOptions(var13.type, -641, var6, var5, var8);
				}
				if (var7 == 0) {
					PlayerEntity var14 = this.players[var8];
					if ((var14.x & 0x7F) == 64 && (var14.z & 0x7F) == 64) {
						for (var10 = 0; var10 < this.npcCount; var10++) {
							var11 = this.npcs[this.npcIds[var10]];
							if (var11 != null && var11.type.size == 1 && var11.x == var14.x && var11.z == var14.z) {
								this.addNpcOptions(var11.type, -641, var6, var5, this.npcIds[var10]);
							}
						}
						for (int var16 = 0; var16 < this.playerCount; var16++) {
							PlayerEntity var12 = this.players[this.playerIds[var16]];
							if (var12 != null && var12 != var14 && var12.x == var14.x && var12.z == var14.z) {
								this.addPlayerOptions(false, var6, this.playerIds[var16], var12, var5);
							}
						}
					}
					this.addPlayerOptions(false, var6, var8, var14, var5);
				}
				if (var7 == 3) {
					LinkList var15 = this.levelObjStacks[this.currentLevel][var5][var6];
					if (var15 != null) {
						for (ObjStackEntity var17 = (ObjStackEntity) var15.tail((byte) 2); var17 != null; var17 = (ObjStackEntity) var15.prev(false)) {
							ObjType var18 = ObjType.get(var17.index);
							if (this.objSelected == 1) {
								this.menuOption[this.menuSize] = "Use " + this.objSelectedName + " with @lre@" + var18.name;
								this.menuAction[this.menuSize] = 217;
								this.menuParamA[this.menuSize] = var17.index;
								this.menuParamB[this.menuSize] = var5;
								this.menuParamC[this.menuSize] = var6;
								this.menuSize++;
							} else if (this.spellSelected != 1) {
								for (int var19 = 4; var19 >= 0; var19--) {
									if (var18.op != null && var18.op[var19] != null) {
										this.menuOption[this.menuSize] = var18.op[var19] + " @lre@" + var18.name;
										if (var19 == 0) {
											this.menuAction[this.menuSize] = 224;
										}
										if (var19 == 1) {
											this.menuAction[this.menuSize] = 993;
										}
										if (var19 == 2) {
											this.menuAction[this.menuSize] = 99;
										}
										if (var19 == 3) {
											this.menuAction[this.menuSize] = 746;
										}
										if (var19 == 4) {
											this.menuAction[this.menuSize] = 877;
										}
										this.menuParamA[this.menuSize] = var17.index;
										this.menuParamB[this.menuSize] = var5;
										this.menuParamC[this.menuSize] = var6;
										this.menuSize++;
									} else if (var19 == 2) {
										this.menuOption[this.menuSize] = "Take @lre@" + var18.name;
										this.menuAction[this.menuSize] = 99;
										this.menuParamA[this.menuSize] = var17.index;
										this.menuParamB[this.menuSize] = var5;
										this.menuParamC[this.menuSize] = var6;
										this.menuSize++;
									}
								}
								this.menuOption[this.menuSize] = "Examine @lre@" + var18.name;
								this.menuAction[this.menuSize] = 1102;
								this.menuParamA[this.menuSize] = var17.index;
								this.menuParamB[this.menuSize] = var5;
								this.menuParamC[this.menuSize] = var6;
								this.menuSize++;
							} else if ((this.activeSpellFlags & 0x1) == 1) {
								this.menuOption[this.menuSize] = this.spellCaption + " @lre@" + var18.name;
								this.menuAction[this.menuSize] = 965;
								this.menuParamA[this.menuSize] = var17.index;
								this.menuParamB[this.menuSize] = var5;
								this.menuParamC[this.menuSize] = var6;
								this.menuSize++;
							}
						}
					}
				}
			}
		}
	}

	@ObfuscatedName("client.n(Z)V")
	public final void updatePlayers(boolean arg0) {
		this.ingame &= arg0;
		int var3;
		for (int var2 = -1; var2 < this.playerCount; var2++) {
			if (var2 == -1) {
				var3 = this.LOCAL_PLAYER_INDEX;
			} else {
				var3 = this.playerIds[var2];
			}
			PlayerEntity var4 = this.players[var3];
			if (var4 != null) {
				this.updateEntity(var4, (byte) -128, 1);
			}
		}
		cyclelogic6++;
		if (cyclelogic6 <= 1406) {
			return;
		}
		cyclelogic6 = 0;
		this.out.p1isaac((byte) -34, 219);
		this.out.p1(0);
		var3 = this.out.pos;
		this.out.p1(162);
		this.out.p1(22);
		if ((int) (Math.random() * 2.0D) == 0) {
			this.out.p1(84);
		}
		this.out.p2(31824);
		this.out.p2(13490);
		if ((int) (Math.random() * 2.0D) == 0) {
			this.out.p1(123);
		}
		if ((int) (Math.random() * 2.0D) == 0) {
			this.out.p1(134);
		}
		this.out.p1(100);
		this.out.p1(94);
		this.out.p2(35521);
		this.out.psize1(0, this.out.pos - var3);
	}

	@ObfuscatedName("client.r(B)V")
	public final void drawTileHint(byte arg0) {
		if (this.hintType != 2) {
			return;
		}
		this.projectFromGround((this.hintTileZ - this.sceneBaseTileZ << 7) + this.hintOffsetZ, (this.hintTileX - this.sceneBaseTileX << 7) + this.hintOffsetX, this.field1470, this.hintHeight * 2);
		if (arg0 != -11) {
			this.field1512 = !this.field1512;
		}
		if (this.projectX > -1 && loopCycle % 20 < 10) {
			this.imageHeadicons[2].draw(this.projectY - 28, this.projectX - 12, false);
		}
	}

	@ObfuscatedName("client.b(IILkb;)V")
	public final void getPlayerLocal(int arg0, int arg1, Packet arg2) {
		arg2.accessBits(223);
		int var4 = arg2.gBit(9, 1);
		if (arg0 != 11522) {
			this.levelObjStacks = null;
		}
		if (var4 == 0) {
			return;
		}
		int var5 = arg2.gBit(9, 2);
		if (var5 == 0) {
			this.entityUpdateIds[this.entityUpdateCount++] = this.LOCAL_PLAYER_INDEX;
			return;
		}
		int var6;
		int var7;
		if (var5 == 1) {
			var6 = arg2.gBit(9, 3);
			this.localPlayer.moveAlongRoute(false, var6, (byte) 6);
			var7 = arg2.gBit(9, 1);
			if (var7 == 1) {
				this.entityUpdateIds[this.entityUpdateCount++] = this.LOCAL_PLAYER_INDEX;
			}
			return;
		}
		int var8;
		if (var5 == 2) {
			var6 = arg2.gBit(9, 3);
			this.localPlayer.moveAlongRoute(true, var6, (byte) 6);
			var7 = arg2.gBit(9, 3);
			this.localPlayer.moveAlongRoute(true, var7, (byte) 6);
			var8 = arg2.gBit(9, 1);
			if (var8 == 1) {
				this.entityUpdateIds[this.entityUpdateCount++] = this.LOCAL_PLAYER_INDEX;
			}
		} else if (var5 == 3) {
			this.currentLevel = arg2.gBit(9, 2);
			var6 = arg2.gBit(9, 7);
			var7 = arg2.gBit(9, 7);
			var8 = arg2.gBit(9, 1);
			this.localPlayer.teleport(false, var8 == 1, var6, var7);
			int var9 = arg2.gBit(9, 1);
			if (var9 == 1) {
				this.entityUpdateIds[this.entityUpdateCount++] = this.LOCAL_PLAYER_INDEX;
			}
		}
	}

	@ObfuscatedName("client.o(Z)V")
	public final void drawChatback(boolean arg0) {
		if (arg0) {
			this.packetType = -1;
		}
		this.areaChatback.bind((byte) 62);
		Pix3D.lineOffset = this.areaChatbackOffsets;
		this.imageChatback.draw(0, 0, false);
		if (this.showSocialInput) {
			this.fontBold12.drawStringCenter(40, (byte) 6, 0, this.socialMessage, 239);
			this.fontBold12.drawStringCenter(60, (byte) 6, 128, this.socialInput + "*", 239);
		} else if (this.chatbackInputOpen) {
			this.fontBold12.drawStringCenter(40, (byte) 6, 0, "Enter amount:", 239);
			this.fontBold12.drawStringCenter(60, (byte) 6, 128, this.chatbackInput + "*", 239);
		} else if (this.modalMessage != null) {
			this.fontBold12.drawStringCenter(40, (byte) 6, 0, this.modalMessage, 239);
			this.fontBold12.drawStringCenter(60, (byte) 6, 128, "Click to continue", 239);
		} else if (this.chatInterfaceId != -1) {
			this.drawInterface(0, 0, 38682, Component.instances[this.chatInterfaceId], 0);
		} else if (this.stickyChatInterfaceId == -1) {
			PixFont var2 = this.fontPlain12;
			int var3 = 0;
			Pix2D.setClipping(77, 0, 463, 789, 0);
			for (int var4 = 0; var4 < 100; var4++) {
				if (this.messageText[var4] != null) {
					int var5 = this.messageType[var4];
					int var6 = 70 - var3 * 14 + this.chatScrollOffset;
					if (var5 == 0) {
						if (var6 > 0 && var6 < 110) {
							var2.drawString(4, var6, false, 0, this.messageText[var4]);
						}
						var3++;
					}
					if (var5 == 1) {
						if (var6 > 0 && var6 < 110) {
							var2.drawString(4, var6, false, 16777215, this.messageSender[var4] + ":");
							var2.drawString(var2.stringWidth(false, this.messageSender[var4]) + 12, var6, false, 255, this.messageText[var4]);
						}
						var3++;
					}
					if (var5 == 2 && (this.publicChatSetting == 0 || this.publicChatSetting == 1 && this.isFriend(-20, this.messageSender[var4]))) {
						if (var6 > 0 && var6 < 110) {
							var2.drawString(4, var6, false, 0, this.messageSender[var4] + ":");
							var2.drawString(var2.stringWidth(false, this.messageSender[var4]) + 12, var6, false, 255, this.messageText[var4]);
						}
						var3++;
					}
					if ((var5 == 3 || var5 == 7) && this.splitPrivateChat == 0 && (var5 == 7 || this.privateChatSetting == 0 || this.privateChatSetting == 1 && this.isFriend(-20, this.messageSender[var4]))) {
						if (var6 > 0 && var6 < 110) {
							var2.drawString(4, var6, false, 0, "From " + this.messageSender[var4] + ":");
							var2.drawString(var2.stringWidth(false, "From " + this.messageSender[var4]) + 12, var6, false, 8388608, this.messageText[var4]);
						}
						var3++;
					}
					if (var5 == 4 && (this.tradeChatSetting == 0 || this.tradeChatSetting == 1 && this.isFriend(-20, this.messageSender[var4]))) {
						if (var6 > 0 && var6 < 110) {
							var2.drawString(4, var6, false, 8388736, this.messageSender[var4] + " " + this.messageText[var4]);
						}
						var3++;
					}
					if (var5 == 5 && this.splitPrivateChat == 0 && this.privateChatSetting < 2) {
						if (var6 > 0 && var6 < 110) {
							var2.drawString(4, var6, false, 8388608, this.messageText[var4]);
						}
						var3++;
					}
					if (var5 == 6 && this.splitPrivateChat == 0 && this.privateChatSetting < 2) {
						if (var6 > 0 && var6 < 110) {
							var2.drawString(4, var6, false, 0, "To " + this.messageSender[var4] + ":");
							var2.drawString(var2.stringWidth(false, "To " + this.messageSender[var4]) + 12, var6, false, 8388608, this.messageText[var4]);
						}
						var3++;
					}
					if (var5 == 8 && (this.tradeChatSetting == 0 || this.tradeChatSetting == 1 && this.isFriend(-20, this.messageSender[var4]))) {
						if (var6 > 0 && var6 < 110) {
							var2.drawString(4, var6, false, 13350793, this.messageSender[var4] + " " + this.messageText[var4]);
						}
						var3++;
					}
				}
			}
			Pix2D.resetClipping(0);
			this.chatScrollHeight = var3 * 14 + 7;
			if (this.chatScrollHeight < 78) {
				this.chatScrollHeight = 78;
			}
			this.drawScrollbar(this.field1168, 463, 0, this.chatScrollHeight - this.chatScrollOffset - 77, this.chatScrollHeight, 77);
			var2.drawString(4, 90, false, 0, JString.formatName(0, this.username) + ":");
			var2.drawString(var2.stringWidth(false, this.username + ": ") + 6, 90, false, 255, this.chatTyped + "*");
			Pix2D.hline(0, 0, 77, 479, 0);
		} else {
			this.drawInterface(0, 0, 38682, Component.instances[this.stickyChatInterfaceId], 0);
		}
		if (this.menuVisible && this.menuArea == 2) {
			this.drawMenu(-961);
		}
		this.areaChatback.draw(375, super.graphics, 22, 5193);
		this.areaViewport.bind((byte) 62);
		Pix3D.lineOffset = this.areaViewportOffsets;
	}

	@ObfuscatedName("client.p(Z)Z")
	public final boolean read(boolean arg0) {
		if (arg0) {
			this.levelObjStacks = null;
		}
		if (this.stream == null) {
			return false;
		}
		String var3;
		int var4;
		try {
			int var2 = this.stream.available();
			if (var2 == 0) {
				return false;
			}
			if (this.packetType == -1) {
				this.stream.read(this.in.data, 0, 1);
				this.packetType = this.in.data[0] & 0xFF;
				if (this.randomIn != null) {
					this.packetType = this.packetType - this.randomIn.takeNextValue() & 0xFF;
				}
				this.packetSize = Protocol.SERVERPROT_SIZES[this.packetType];
				var2--;
			}
			if (this.packetSize == -1) {
				if (var2 <= 0) {
					return false;
				}
				this.stream.read(this.in.data, 0, 1);
				this.packetSize = this.in.data[0] & 0xFF;
				var2--;
			}
			if (this.packetSize == -2) {
				if (var2 <= 1) {
					return false;
				}
				this.stream.read(this.in.data, 0, 2);
				this.in.pos = 0;
				this.packetSize = this.in.g2();
				var2 -= 2;
			}
			if (var2 < this.packetSize) {
				return false;
			}
			this.in.pos = 0;
			this.stream.read(this.in.data, 0, this.packetSize);
			this.idleNetCycles = 0;
			this.lastPacketType2 = this.lastPacketType1;
			this.lastPacketType1 = this.lastPacketType0;
			this.lastPacketType0 = this.packetType;
			int var26;
			if (this.packetType == 150) {
				var26 = this.in.g2();
				byte var52 = this.in.g1b();
				this.varCache[var26] = var52;
				if (this.varps[var26] != var52) {
					this.varps[var26] = var52;
					this.updateVarp(var26, 49);
					this.redrawSidebar = true;
					if (this.stickyChatInterfaceId != -1) {
						this.redrawChatback = true;
					}
				}
				this.packetType = -1;
				return true;
			}
			int var5;
			int var7;
			int var9;
			int var10;
			long var39;
			if (this.packetType == 152) {
				var39 = this.in.g8(603);
				var5 = this.in.g1();
				String var44 = JString.formatName(0, JString.fromBase37(var39, false));
				for (var7 = 0; var7 < this.friendCount; var7++) {
					if (var39 == this.friendName37[var7]) {
						if (this.friendWorld[var7] != var5) {
							this.friendWorld[var7] = var5;
							this.redrawSidebar = true;
							if (var5 > 0) {
								this.addMessage(5, var44 + " has logged in.", (byte) 4, "");
							}
							if (var5 == 0) {
								this.addMessage(5, var44 + " has logged out.", (byte) 4, "");
							}
						}
						var44 = null;
						break;
					}
				}
				if (var44 != null && this.friendCount < 100) {
					this.friendName37[this.friendCount] = var39;
					this.friendName[this.friendCount] = var44;
					this.friendWorld[this.friendCount] = var5;
					this.friendCount++;
					this.redrawSidebar = true;
				}
				boolean var41 = false;
				while (!var41) {
					var41 = true;
					for (var9 = 0; var9 < this.friendCount - 1; var9++) {
						if (this.friendWorld[var9] != nodeId && this.friendWorld[var9 + 1] == nodeId || this.friendWorld[var9] == 0 && this.friendWorld[var9 + 1] != 0) {
							var10 = this.friendWorld[var9];
							this.friendWorld[var9] = this.friendWorld[var9 + 1];
							this.friendWorld[var9 + 1] = var10;
							String var42 = this.friendName[var9];
							this.friendName[var9] = this.friendName[var9 + 1];
							this.friendName[var9 + 1] = var42;
							long var50 = this.friendName37[var9];
							this.friendName37[var9] = this.friendName37[var9 + 1];
							this.friendName37[var9 + 1] = var50;
							this.redrawSidebar = true;
							var41 = false;
						}
					}
				}
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 43) {
				this.systemUpdateTimer = this.in.g2() * 30;
				this.packetType = -1;
				return true;
			}
			int var6;
			if (this.packetType == 80) {
				var26 = this.in.g1();
				var4 = this.in.g1();
				var5 = -1;
				for (var6 = 0; var6 < this.sceneMapIndex.length; var6++) {
					if (this.sceneMapIndex[var6] == (var26 << 8) + var4) {
						var5 = var6;
					}
				}
				if (var5 != -1) {
					signlink.cachesave("m" + var26 + "_" + var4, this.sceneMapLandData[var5]);
					this.sceneState = 1;
				}
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 1) {
				this.getNpcPos(this.in, this.packetSize, false);
				this.packetType = -1;
				return true;
			}
			int var8;
			if (this.packetType == 237) {
				var26 = this.in.g2();
				var4 = this.in.g2();
				if (this.sceneCenterZoneX == var26 && this.sceneCenterZoneZ == var4 && this.sceneState != 0) {
					this.packetType = -1;
					return true;
				}
				this.sceneCenterZoneX = var26;
				this.sceneCenterZoneZ = var4;
				this.sceneBaseTileX = (this.sceneCenterZoneX - 6) * 8;
				this.sceneBaseTileZ = (this.sceneCenterZoneZ - 6) * 8;
				this.sceneState = 1;
				this.areaViewport.bind((byte) 62);
				this.fontPlain12.drawStringCenter(151, (byte) 6, 0, "Loading - please wait.", 257);
				this.fontPlain12.drawStringCenter(150, (byte) 6, 16777215, "Loading - please wait.", 256);
				this.areaViewport.draw(11, super.graphics, 8, 5193);
				signlink.looprate(5);
				var5 = (this.packetSize - 2) / 10;
				this.sceneMapLandData = new byte[var5][];
				this.sceneMapLocData = new byte[var5][];
				this.sceneMapIndex = new int[var5];
				this.out.p1isaac((byte) -34, 150);
				this.out.p1(0);
				var6 = 0;
				int var11;
				for (var7 = 0; var7 < var5; var7++) {
					var8 = this.in.g1();
					var9 = this.in.g1();
					var10 = this.in.g4();
					var11 = this.in.g4();
					this.sceneMapIndex[var7] = (var8 << 8) + var9;
					byte[] var12;
					if (var10 != 0) {
						var12 = signlink.cacheload("m" + var8 + "_" + var9);
						if (var12 != null) {
							this.crc32.reset();
							this.crc32.update(var12);
							if ((int) this.crc32.getValue() != var10) {
								var12 = null;
							}
						}
						if (var12 == null) {
							this.sceneState = 0;
							this.out.p1(0);
							this.out.p1(var8);
							this.out.p1(var9);
							var6 += 3;
						} else {
							this.sceneMapLandData[var7] = var12;
						}
					}
					if (var11 != 0) {
						var12 = signlink.cacheload("l" + var8 + "_" + var9);
						if (var12 != null) {
							this.crc32.reset();
							this.crc32.update(var12);
							if ((int) this.crc32.getValue() != var11) {
								var12 = null;
							}
						}
						if (var12 == null) {
							this.sceneState = 0;
							this.out.p1(1);
							this.out.p1(var8);
							this.out.p1(var9);
							var6 += 3;
						} else {
							this.sceneMapLocData[var7] = var12;
						}
					}
				}
				this.out.psize1(0, var6);
				signlink.looprate(50);
				this.areaViewport.bind((byte) 62);
				if (this.sceneState == 0) {
					this.fontPlain12.drawStringCenter(166, (byte) 6, 0, "Map area updated since last visit, so load will take longer this time only", 257);
					this.fontPlain12.drawStringCenter(165, (byte) 6, 16777215, "Map area updated since last visit, so load will take longer this time only", 256);
				}
				this.areaViewport.draw(11, super.graphics, 8, 5193);
				var8 = this.sceneBaseTileX - this.mapLastBaseX;
				var9 = this.sceneBaseTileZ - this.mapLastBaseZ;
				this.mapLastBaseX = this.sceneBaseTileX;
				this.mapLastBaseZ = this.sceneBaseTileZ;
				for (var10 = 0; var10 < 8192; var10++) {
					NpcEntity var40 = this.npcs[var10];
					if (var40 != null) {
						for (int var46 = 0; var46 < 10; var46++) {
							var40.pathTileX[var46] -= var8;
							var40.pathTileZ[var46] -= var9;
						}
						var40.x -= var8 * 128;
						var40.z -= var9 * 128;
					}
				}
				for (var11 = 0; var11 < this.MAX_PLAYER_COUNT; var11++) {
					PlayerEntity var48 = this.players[var11];
					if (var48 != null) {
						for (int var13 = 0; var13 < 10; var13++) {
							var48.pathTileX[var13] -= var8;
							var48.pathTileZ[var13] -= var9;
						}
						var48.x -= var8 * 128;
						var48.z -= var9 * 128;
					}
				}
				byte var49 = 0;
				byte var45 = 104;
				byte var14 = 1;
				if (var8 < 0) {
					var49 = 103;
					var45 = -1;
					var14 = -1;
				}
				byte var15 = 0;
				byte var16 = 104;
				byte var17 = 1;
				if (var9 < 0) {
					var15 = 103;
					var16 = -1;
					var17 = -1;
				}
				for (int var18 = var49; var18 != var45; var18 += var14) {
					for (int var19 = var15; var19 != var16; var19 += var17) {
						int var20 = var18 + var8;
						int var21 = var19 + var9;
						for (int var22 = 0; var22 < 4; var22++) {
							if (var20 >= 0 && var21 >= 0 && var20 < 104 && var21 < 104) {
								this.levelObjStacks[var22][var18][var19] = this.levelObjStacks[var22][var20][var21];
							} else {
								this.levelObjStacks[var22][var18][var19] = null;
							}
						}
					}
				}
				for (LocAddEntity var53 = (LocAddEntity) this.spawnedLocations.head(); var53 != null; var53 = (LocAddEntity) this.spawnedLocations.next(551)) {
					var53.x -= var8;
					var53.z -= var9;
					if (var53.x < 0 || var53.z < 0 || var53.x >= 104 || var53.z >= 104) {
						var53.unlink();
					}
				}
				if (this.flagSceneTileX != 0) {
					this.flagSceneTileX -= var8;
					this.flagSceneTileZ -= var9;
				}
				this.cutscene = false;
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 197) {
				var26 = this.in.g2();
				Component.instances[var26].model = this.localPlayer.getHeadModel(-718);
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 25) {
				this.hintType = this.in.g1();
				if (this.hintType == 1) {
					this.hintNpc = this.in.g2();
				}
				if (this.hintType >= 2 && this.hintType <= 6) {
					if (this.hintType == 2) {
						this.hintOffsetX = 64;
						this.hintOffsetZ = 64;
					}
					if (this.hintType == 3) {
						this.hintOffsetX = 0;
						this.hintOffsetZ = 64;
					}
					if (this.hintType == 4) {
						this.hintOffsetX = 128;
						this.hintOffsetZ = 64;
					}
					if (this.hintType == 5) {
						this.hintOffsetX = 64;
						this.hintOffsetZ = 0;
					}
					if (this.hintType == 6) {
						this.hintOffsetX = 64;
						this.hintOffsetZ = 128;
					}
					this.hintType = 2;
					this.hintTileX = this.in.g2();
					this.hintTileZ = this.in.g2();
					this.hintHeight = this.in.g1();
				}
				if (this.hintType == 10) {
					this.hintPlayer = this.in.g2();
				}
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 54) {
				var3 = this.in.gjstr();
				var4 = this.in.g4();
				var5 = this.in.g4();
				if (!var3.equals(this.currentMidi) && this.midiActive && !lowMemory) {
					this.setMidi(false, var4, var3, var5);
				}
				this.currentMidi = var3;
				this.midiCrc = var4;
				this.midiSize = var5;
				this.nextMusicDelay = 0;
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 142) {
				this.logout(-780);
				this.packetType = -1;
				return false;
			}
			if (this.packetType == 20) {
				var26 = this.in.g1();
				var4 = this.in.g1();
				var5 = -1;
				for (var6 = 0; var6 < this.sceneMapIndex.length; var6++) {
					if (this.sceneMapIndex[var6] == (var26 << 8) + var4) {
						var5 = var6;
					}
				}
				if (var5 != -1) {
					signlink.cachesave("l" + var26 + "_" + var4, this.sceneMapLocData[var5]);
					this.sceneState = 1;
				}
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 19) {
				this.flagSceneTileX = 0;
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 139) {
				this.localPid = this.in.g2();
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 151 || this.packetType == 23 || this.packetType == 50 || this.packetType == 191 || this.packetType == 69 || this.packetType == 49 || this.packetType == 223 || this.packetType == 42 || this.packetType == 76 || this.packetType == 59) {
				this.readZonePacket((byte) -45, this.in, this.packetType);
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 28) {
				var26 = this.in.g2();
				var4 = this.in.g2();
				if (this.chatInterfaceId != -1) {
					this.chatInterfaceId = -1;
					this.redrawChatback = true;
				}
				if (this.chatbackInputOpen) {
					this.chatbackInputOpen = false;
					this.redrawChatback = true;
				}
				this.viewportInterfaceId = var26;
				this.sidebarInterfaceId = var4;
				this.redrawSidebar = true;
				this.redrawSideicons = true;
				this.pressedContinueOption = false;
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 175) {
				var26 = this.in.g2();
				var4 = this.in.g4();
				this.varCache[var26] = var4;
				if (this.varps[var26] != var4) {
					this.varps[var26] = var4;
					this.updateVarp(var26, 49);
					this.redrawSidebar = true;
					if (this.stickyChatInterfaceId != -1) {
						this.redrawChatback = true;
					}
				}
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 146) {
				var26 = this.in.g2();
				var4 = this.in.g2();
				Component.instances[var26].anim = var4;
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 167) {
				var26 = this.in.g2();
				var4 = this.in.g1();
				if (var26 == 65535) {
					var26 = -1;
				}
				this.tabInterfaceId[var4] = var26;
				this.redrawSidebar = true;
				this.redrawSideicons = true;
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 220) {
				var26 = this.in.g1();
				var4 = this.in.g1();
				var5 = this.in.g2();
				var6 = this.in.g2();
				var7 = -1;
				for (var8 = 0; var8 < this.sceneMapIndex.length; var8++) {
					if (this.sceneMapIndex[var8] == (var26 << 8) + var4) {
						var7 = var8;
					}
				}
				if (var7 != -1) {
					if (this.sceneMapLocData[var7] == null || this.sceneMapLocData[var7].length != var6) {
						this.sceneMapLocData[var7] = new byte[var6];
					}
					this.in.gdata(this.packetSize - 6, -110, var5, this.sceneMapLocData[var7]);
				}
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 133) {
				Packet var51 = InputTracking.stop(854);
				if (var51 != null) {
					this.out.p1isaac((byte) -34, 81);
					this.out.p2(var51.pos);
					this.out.pdata(var51.data, var51.pos, 0, (byte) -106);
					var51.release((byte) 8);
				}
				this.packetType = -1;
				return true;
			}
			Component var27;
			if (this.packetType == 98) {
				this.redrawSidebar = true;
				var26 = this.in.g2();
				var27 = Component.instances[var26];
				var5 = this.in.g1();
				for (var6 = 0; var6 < var5; var6++) {
					var27.invSlotObjId[var6] = this.in.g2();
					var7 = this.in.g1();
					if (var7 == 255) {
						var7 = this.in.g4();
					}
					var27.invSlotObjCount[var6] = var7;
				}
				for (var7 = var5; var7 < var27.invSlotObjId.length; var7++) {
					var27.invSlotObjId[var7] = 0;
					var27.invSlotObjCount[var7] = 0;
				}
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 226) {
				InputTracking.setEnabled(-31717);
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 243) {
				this.showSocialInput = false;
				this.chatbackInputOpen = true;
				this.chatbackInput = "";
				this.redrawChatback = true;
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 15) {
				var26 = this.in.g2();
				var27 = Component.instances[var26];
				for (var5 = 0; var5 < var27.invSlotObjId.length; var5++) {
					var27.invSlotObjId[var5] = -1;
					var27.invSlotObjId[var5] = 0;
				}
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 140) {
				this.lastAddress = this.in.g4();
				this.daysSinceLastLogin = this.in.g2();
				this.daysSinceRecoveriesChanged = this.in.g1();
				this.unreadMessages = this.in.g2();
				if (this.lastAddress != 0 && this.viewportInterfaceId == -1) {
					signlink.dnslookup(JString.formatIPv4(-946, this.lastAddress));
					this.closeInterfaces((byte) -60);
					short var47 = 650;
					if (this.daysSinceRecoveriesChanged != 201) {
						var47 = 655;
					}
					this.reportAbuseInput = "";
					this.reportAbuseMuteOption = false;
					for (var4 = 0; var4 < Component.instances.length; var4++) {
						if (Component.instances[var4] != null && Component.instances[var4].clientCode == var47) {
							this.viewportInterfaceId = Component.instances[var4].layer;
							break;
						}
					}
				}
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 126) {
				this.flashingTab = this.in.g1();
				if (this.flashingTab == this.selectedTab) {
					if (this.flashingTab == 3) {
						this.selectedTab = 1;
					} else {
						this.selectedTab = 3;
					}
					this.redrawSidebar = true;
				}
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 212) {
				if (this.midiActive && !lowMemory) {
					var26 = this.in.g2();
					var4 = this.in.g4();
					var5 = this.packetSize - 6;
					byte[] var35 = new byte[var4];
					BZip2.read(var35, var4, this.in.data, var5, this.in.pos);
					this.saveMidi(var35, 625, var4, false);
					this.nextMusicDelay = var26;
				}
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 254) {
				this.inMultizone = this.in.g1();
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 12) {
				var26 = this.in.g2();
				var4 = this.in.g1();
				var5 = this.in.g2();
				if (this.waveEnabled && !lowMemory && this.waveCount < 50) {
					this.waveIds[this.waveCount] = var26;
					this.waveLoops[this.waveCount] = var4;
					this.waveDelay[this.waveCount] = var5 + Wave.delays[var26];
					this.waveCount++;
				}
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 204) {
				var26 = this.in.g2();
				var4 = this.in.g2();
				NpcType var33 = NpcType.get(var4);
				Component.instances[var26].model = var33.getHeadModel(false);
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 7) {
				this.baseX = this.in.g1();
				this.baseZ = this.in.g1();
				this.packetType = -1;
				return true;
			}
			Component var34;
			if (this.packetType == 103) {
				var26 = this.in.g2();
				var4 = this.in.g2();
				var5 = this.in.g2();
				var34 = Component.instances[var26];
				Model var43 = var34.model;
				if (var43 != null) {
					var43.recolor(var4, var5);
				}
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 32) {
				this.publicChatSetting = this.in.g1();
				this.privateChatSetting = this.in.g1();
				this.tradeChatSetting = this.in.g1();
				this.redrawPrivacySettings = true;
				this.redrawChatback = true;
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 195) {
				var26 = this.in.g2();
				this.resetInterfaceAnimation(-321, var26);
				if (this.chatInterfaceId != -1) {
					this.chatInterfaceId = -1;
					this.redrawChatback = true;
				}
				if (this.chatbackInputOpen) {
					this.chatbackInputOpen = false;
					this.redrawChatback = true;
				}
				this.sidebarInterfaceId = var26;
				this.redrawSidebar = true;
				this.redrawSideicons = true;
				this.viewportInterfaceId = -1;
				this.pressedContinueOption = false;
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 14) {
				var26 = this.in.g2();
				this.resetInterfaceAnimation(-321, var26);
				if (this.sidebarInterfaceId != -1) {
					this.sidebarInterfaceId = -1;
					this.redrawSidebar = true;
					this.redrawSideicons = true;
				}
				this.chatInterfaceId = var26;
				this.redrawChatback = true;
				this.viewportInterfaceId = -1;
				this.pressedContinueOption = false;
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 209) {
				var26 = this.in.g2();
				var4 = this.in.g2b();
				var5 = this.in.g2b();
				var34 = Component.instances[var26];
				var34.x = var4;
				var34.y = var5;
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 3) {
				this.cutscene = true;
				this.cutsceneSrcLocalTileX = this.in.g1();
				this.cutsceneSrcLocalTileZ = this.in.g1();
				this.cutsceneSrcHeight = this.in.g2();
				this.cutsceneMoveSpeed = this.in.g1();
				this.cutsceneMoveAcceleration = this.in.g1();
				if (this.cutsceneMoveAcceleration >= 100) {
					this.cameraX = this.cutsceneSrcLocalTileX * 128 + 64;
					this.cameraZ = this.cutsceneSrcLocalTileZ * 128 + 64;
					this.cameraY = this.getHeightmapY(this.currentLevel, this.cutsceneSrcLocalTileX, (byte) 5, this.cutsceneSrcLocalTileZ) - this.cutsceneSrcHeight;
				}
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 135) {
				this.baseX = this.in.g1();
				this.baseZ = this.in.g1();
				for (var26 = this.baseX; var26 < this.baseX + 8; var26++) {
					for (var4 = this.baseZ; var4 < this.baseZ + 8; var4++) {
						if (this.levelObjStacks[this.currentLevel][var26][var4] != null) {
							this.levelObjStacks[this.currentLevel][var26][var4] = null;
							this.sortObjStacks(var26, var4);
						}
					}
				}
				for (LocAddEntity var36 = (LocAddEntity) this.spawnedLocations.head(); var36 != null; var36 = (LocAddEntity) this.spawnedLocations.next(551)) {
					if (var36.x >= this.baseX && var36.x < this.baseX + 8 && var36.z >= this.baseZ && var36.z < this.baseZ + 8 && var36.plane == this.currentLevel) {
						this.addLoc(var36.lastAngle, var36.x, var36.z, var36.layer, var36.lastLocIndex, var36.lastShape, -27819, var36.plane);
						var36.unlink();
					}
				}
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 132) {
				var26 = this.in.g1();
				var4 = this.in.g1();
				var5 = this.in.g2();
				var6 = this.in.g2();
				var7 = -1;
				for (var8 = 0; var8 < this.sceneMapIndex.length; var8++) {
					if (this.sceneMapIndex[var8] == (var26 << 8) + var4) {
						var7 = var8;
					}
				}
				if (var7 != -1) {
					if (this.sceneMapLandData[var7] == null || this.sceneMapLandData[var7].length != var6) {
						this.sceneMapLandData[var7] = new byte[var6];
					}
					this.in.gdata(this.packetSize - 6, -110, var5, this.sceneMapLandData[var7]);
				}
				this.packetType = -1;
				return true;
			}
			boolean var32;
			if (this.packetType == 41) {
				var39 = this.in.g8(603);
				var5 = this.in.g4();
				var6 = this.in.g1();
				var32 = false;
				for (var8 = 0; var8 < 100; var8++) {
					if (this.messageIds[var8] == var5) {
						var32 = true;
						break;
					}
				}
				if (var6 <= 1) {
					for (var9 = 0; var9 < this.ignoreCount; var9++) {
						if (this.ignoreName37[var9] == var39) {
							var32 = true;
							break;
						}
					}
				}
				if (!var32 && this.overrideChat == 0) {
					try {
						this.messageIds[this.privateMessageCount] = var5;
						this.privateMessageCount = (this.privateMessageCount + 1) % 100;
						String var37 = WordPack.unpack(this.in, 118, this.packetSize - 13);
						String var38 = WordFilter.filter(var37, 0);
						if (var6 > 1) {
							this.addMessage(7, var38, (byte) 4, JString.formatName(0, JString.fromBase37(var39, false)));
						} else {
							this.addMessage(3, var38, (byte) 4, JString.formatName(0, JString.fromBase37(var39, false)));
						}
					} catch (Exception var23) {
						signlink.reporterror("cde1");
					}
				}
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 193) {
				for (var26 = 0; var26 < this.varps.length; var26++) {
					if (this.varps[var26] != this.varCache[var26]) {
						this.varps[var26] = this.varCache[var26];
						this.updateVarp(var26, 49);
						this.redrawSidebar = true;
					}
				}
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 87) {
				var26 = this.in.g2();
				var4 = this.in.g2();
				Component.instances[var26].model = new Model(false, var4);
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 185) {
				var26 = this.in.g2b();
				this.stickyChatInterfaceId = var26;
				this.redrawChatback = true;
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 68) {
				if (this.selectedTab == 12) {
					this.redrawSidebar = true;
				}
				this.energy = this.in.g1();
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 74) {
				this.cutscene = true;
				this.cutsceneDstLocalTileX = this.in.g1();
				this.cutsceneDstLocalTileZ = this.in.g1();
				this.cutsceneDstHeight = this.in.g2();
				this.cutsceneRotateSpeed = this.in.g1();
				this.cutsceneRotateAcceleration = this.in.g1();
				if (this.cutsceneRotateAcceleration >= 100) {
					var26 = this.cutsceneDstLocalTileX * 128 + 64;
					var4 = this.cutsceneDstLocalTileZ * 128 + 64;
					var5 = this.getHeightmapY(this.currentLevel, this.cutsceneDstLocalTileX, (byte) 5, this.cutsceneDstLocalTileZ) - this.cutsceneDstHeight;
					var6 = var26 - this.cameraX;
					var7 = var5 - this.cameraY;
					var8 = var4 - this.cameraZ;
					var9 = (int) Math.sqrt((double) (var6 * var6 + var8 * var8));
					this.cameraPitch = (int) (Math.atan2((double) var7, (double) var9) * 325.949D) & 0x7FF;
					this.cameraYaw = (int) (Math.atan2((double) var6, (double) var8) * -325.949D) & 0x7FF;
					if (this.cameraPitch < 128) {
						this.cameraPitch = 128;
					}
					if (this.cameraPitch > 383) {
						this.cameraPitch = 383;
					}
				}
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 84) {
				this.selectedTab = this.in.g1();
				this.redrawSidebar = true;
				this.redrawSideicons = true;
				this.packetType = -1;
				return true;
			}
			String var28;
			if (this.packetType == 4) {
				var3 = this.in.gjstr();
				long var30;
				if (var3.endsWith(":tradereq:")) {
					var28 = var3.substring(0, var3.indexOf(":"));
					var30 = JString.toBase37(var28);
					var32 = false;
					for (var8 = 0; var8 < this.ignoreCount; var8++) {
						if (this.ignoreName37[var8] == var30) {
							var32 = true;
							break;
						}
					}
					if (!var32 && this.overrideChat == 0) {
						this.addMessage(4, "wishes to trade with you.", (byte) 4, var28);
					}
				} else if (var3.endsWith(":duelreq:")) {
					var28 = var3.substring(0, var3.indexOf(":"));
					var30 = JString.toBase37(var28);
					var32 = false;
					for (var8 = 0; var8 < this.ignoreCount; var8++) {
						if (this.ignoreName37[var8] == var30) {
							var32 = true;
							break;
						}
					}
					if (!var32 && this.overrideChat == 0) {
						this.addMessage(8, "wishes to duel with you.", (byte) 4, var28);
					}
				} else {
					this.addMessage(0, var3, (byte) 4, "");
				}
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 46) {
				var26 = this.in.g2();
				var4 = this.in.g2();
				var5 = this.in.g2();
				ObjType var31 = ObjType.get(var4);
				Component.instances[var26].model = var31.getInterfaceModel(50);
				Component.instances[var26].xan = var31.xan2d;
				Component.instances[var26].yan = var31.yan2d;
				Component.instances[var26].zoom = var31.zoom2d * 100 / var5;
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 168) {
				var26 = this.in.g2();
				this.resetInterfaceAnimation(-321, var26);
				if (this.sidebarInterfaceId != -1) {
					this.sidebarInterfaceId = -1;
					this.redrawSidebar = true;
					this.redrawSideicons = true;
				}
				if (this.chatInterfaceId != -1) {
					this.chatInterfaceId = -1;
					this.redrawChatback = true;
				}
				if (this.chatbackInputOpen) {
					this.chatbackInputOpen = false;
					this.redrawChatback = true;
				}
				this.viewportInterfaceId = var26;
				this.pressedContinueOption = false;
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 2) {
				var26 = this.in.g2();
				var4 = this.in.g2();
				var5 = var4 >> 10 & 0x1F;
				var6 = var4 >> 5 & 0x1F;
				var7 = var4 & 0x1F;
				Component.instances[var26].colour = (var5 << 19) + (var6 << 11) + (var7 << 3);
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 136) {
				for (var26 = 0; var26 < this.players.length; var26++) {
					if (this.players[var26] != null) {
						this.players[var26].primarySeqId = -1;
					}
				}
				for (var4 = 0; var4 < this.npcs.length; var4++) {
					if (this.npcs[var4] != null) {
						this.npcs[var4].primarySeqId = -1;
					}
				}
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 26) {
				var26 = this.in.g2();
				boolean var29 = this.in.g1() == 1;
				Component.instances[var26].hide = var29;
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 21) {
				this.ignoreCount = this.packetSize / 8;
				for (var26 = 0; var26 < this.ignoreCount; var26++) {
					this.ignoreName37[var26] = this.in.g8(603);
				}
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 239) {
				this.cutscene = false;
				for (var26 = 0; var26 < 5; var26++) {
					this.cameraModifierEnabled[var26] = false;
				}
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 129) {
				if (this.sidebarInterfaceId != -1) {
					this.sidebarInterfaceId = -1;
					this.redrawSidebar = true;
					this.redrawSideicons = true;
				}
				if (this.chatInterfaceId != -1) {
					this.chatInterfaceId = -1;
					this.redrawChatback = true;
				}
				if (this.chatbackInputOpen) {
					this.chatbackInputOpen = false;
					this.redrawChatback = true;
				}
				this.viewportInterfaceId = -1;
				this.pressedContinueOption = false;
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 201) {
				var26 = this.in.g2();
				var28 = this.in.gjstr();
				Component.instances[var26].text = var28;
				if (Component.instances[var26].layer == this.tabInterfaceId[this.selectedTab]) {
					this.redrawSidebar = true;
				}
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 44) {
				this.redrawSidebar = true;
				var26 = this.in.g1();
				var4 = this.in.g4();
				var5 = this.in.g1();
				this.skillExperience[var26] = var4;
				this.skillLevel[var26] = var5;
				this.skillBaseLevel[var26] = 1;
				for (var6 = 0; var6 < 98; var6++) {
					if (var4 >= levelExperience[var6]) {
						this.skillBaseLevel[var26] = var6 + 2;
					}
				}
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 162) {
				this.baseX = this.in.g1();
				this.baseZ = this.in.g1();
				while (this.in.pos < this.packetSize) {
					var26 = this.in.g1();
					this.readZonePacket((byte) -45, this.in, var26);
				}
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 22) {
				if (this.selectedTab == 12) {
					this.redrawSidebar = true;
				}
				this.weightCarried = this.in.g2b();
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 13) {
				var26 = this.in.g1();
				var4 = this.in.g1();
				var5 = this.in.g1();
				var6 = this.in.g1();
				this.cameraModifierEnabled[var26] = true;
				this.cameraModifierJitter[var26] = var4;
				this.cameraModifierWobbleScale[var26] = var5;
				this.cameraModifierWobbleSpeed[var26] = var6;
				this.cameraModifierCycle[var26] = 0;
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 213) {
				this.redrawSidebar = true;
				var26 = this.in.g2();
				var27 = Component.instances[var26];
				while (this.in.pos < this.packetSize) {
					var5 = this.in.g1();
					var6 = this.in.g2();
					var7 = this.in.g1();
					if (var7 == 255) {
						var7 = this.in.g4();
					}
					if (var5 >= 0 && var5 < var27.invSlotObjId.length) {
						var27.invSlotObjId[var5] = var6;
						var27.invSlotObjCount[var5] = var7;
					}
				}
				this.packetType = -1;
				return true;
			}
			if (this.packetType == 184) {
				this.getPlayer(this.in, this.packetSize, 822);
				if (this.sceneState == 1) {
					this.sceneState = 2;
					World.levelBuilt = this.currentLevel;
					this.buildScene(869);
				}
				if (lowMemory && this.sceneState == 2 && World.levelBuilt != this.currentLevel) {
					this.areaViewport.bind((byte) 62);
					this.fontPlain12.drawStringCenter(151, (byte) 6, 0, "Loading - please wait.", 257);
					this.fontPlain12.drawStringCenter(150, (byte) 6, 16777215, "Loading - please wait.", 256);
					this.areaViewport.draw(11, super.graphics, 8, 5193);
					World.levelBuilt = this.currentLevel;
					this.buildScene(869);
				}
				if (this.currentLevel != this.minimapLevel && this.sceneState == 2) {
					this.minimapLevel = this.currentLevel;
					this.createMinimap(this.currentLevel, -153);
				}
				this.packetType = -1;
				return true;
			}
			signlink.reporterror("T1 - " + this.packetType + "," + this.packetSize + " - " + this.lastPacketType1 + "," + this.lastPacketType2);
			this.logout(-780);
		} catch (IOException var24) {
			this.tryReconnect(false);
		} catch (Exception var25) {
            var25.printStackTrace(); // observation only in isolated reference overlay
			var3 = "T2 - " + this.packetType + "," + this.lastPacketType1 + "," + this.lastPacketType2 + " - " + this.packetSize + "," + (this.sceneBaseTileX + this.localPlayer.pathTileX[0]) + "," + (this.sceneBaseTileZ + this.localPlayer.pathTileZ[0]) + " - ";
			for (var4 = 0; var4 < this.packetSize && var4 < 50; var4++) {
				var3 = var3 + this.in.data[var4] + ",";
			}
			signlink.reporterror(var3);
			this.logout(-780);
		}
		return true;
	}

	@ObfuscatedName("client.s(B)V")
	public final void drawSidebar(byte arg0) {
		this.areaSidebar.bind((byte) 62);
		Pix3D.lineOffset = this.areaSidebarOffsets;
		this.imageInvback.draw(0, 0, false);
		if (this.sidebarInterfaceId != -1) {
			this.drawInterface(0, 0, 38682, Component.instances[this.sidebarInterfaceId], 0);
		} else if (this.tabInterfaceId[this.selectedTab] != -1) {
			this.drawInterface(0, 0, 38682, Component.instances[this.tabInterfaceId[this.selectedTab]], 0);
		}
		if (this.menuVisible && this.menuArea == 1) {
			this.drawMenu(-961);
		}
		this.areaSidebar.draw(231, super.graphics, 562, 5193);
		if (arg0 == 2) {
			boolean var2 = false;
			this.areaViewport.bind((byte) 62);
			Pix3D.lineOffset = this.areaViewportOffsets;
		}
	}

	@ObfuscatedName("client.a(ILjava/lang/String;)Z")
	public final boolean isFriend(int arg0, String arg1) {
		while (arg0 >= 0) {
			this.out.p1(74);
		}
		if (arg1 == null) {
			return false;
		}
		for (int var3 = 0; var3 < this.friendCount; var3++) {
			if (arg1.equalsIgnoreCase(this.friendName[var3])) {
				return true;
			}
		}
		if (arg1.equalsIgnoreCase(this.localPlayer.name)) {
			return true;
		} else {
			return false;
		}
	}

	public final void init() {
		nodeId = Integer.parseInt(this.getParameter("nodeid"));
		portOffset = Integer.parseInt(this.getParameter("portoff"));
		String var1 = this.getParameter("lowmem");
		if (var1 != null && var1.equals("1")) {
			setLowMemory(true);
		} else {
			setHighMemory(9);
		}
		String var2 = this.getParameter("free");
		if (var2 != null && var2.equals("1")) {
			members = false;
		} else {
			members = true;
		}
		this.initApplet(532, false, 789);
	}

	@ObfuscatedName("client.a(ZIILkb;Lz;)V")
	public final void getPlayerExtended(boolean arg0, int arg1, int arg2, Packet arg3, PlayerEntity arg4) {
		if (!arg0) {
			this.field1214 = !this.field1214;
		}
		int var6;
		if ((arg2 & 0x1) == 1) {
			var6 = arg3.g1();
			byte[] var7 = new byte[var6];
			Packet var8 = new Packet(363, var7);
			arg3.gdata(var6, -110, 0, var7);
			this.playerAppearanceBuffer[arg1] = var8;
			arg4.read(false, var8);
		}
		int var15;
		if ((arg2 & 0x2) == 2) {
			var6 = arg3.g2();
			if (var6 == 65535) {
				var6 = -1;
			}
			if (var6 == arg4.primarySeqId) {
				arg4.primarySeqLoop = 0;
			}
			var15 = arg3.g1();
			if (var6 == -1 || arg4.primarySeqId == -1 || SeqType.instances[var6].priority > SeqType.instances[arg4.primarySeqId].priority || SeqType.instances[arg4.primarySeqId].priority == 0) {
				arg4.primarySeqId = var6;
				arg4.primarySeqFrame = 0;
				arg4.primarySeqCycle = 0;
				arg4.primarySeqDelay = var15;
				arg4.primarySeqLoop = 0;
			}
		}
		if ((arg2 & 0x4) == 4) {
			arg4.targetId = arg3.g2();
			if (arg4.targetId == 65535) {
				arg4.targetId = -1;
			}
		}
		if ((arg2 & 0x8) == 8) {
			arg4.chat = arg3.gjstr();
			arg4.chatColor = 0;
			arg4.chatStyle = 0;
			arg4.chatTimer = 150;
			this.addMessage(2, arg4.chat, (byte) 4, arg4.name);
		}
		if ((arg2 & 0x10) == 16) {
			arg4.damage = arg3.g1();
			arg4.damageType = arg3.g1();
			arg4.combatCycle = loopCycle + 400;
			arg4.health = arg3.g1();
			arg4.totalHealth = arg3.g1();
		}
		if ((arg2 & 0x20) == 32) {
			arg4.targetTileX = arg3.g2();
			arg4.targetTileZ = arg3.g2();
		}
		if ((arg2 & 0x40) == 64) {
			var6 = arg3.g2();
			var15 = arg3.g1();
			int var16 = arg3.g1();
			int var9 = arg3.pos;
			if (arg4.name != null) {
				long var10 = JString.toBase37(arg4.name);
				boolean var12 = false;
				if (var15 <= 1) {
					for (int var13 = 0; var13 < this.ignoreCount; var13++) {
						if (this.ignoreName37[var13] == var10) {
							var12 = true;
							break;
						}
					}
				}
				if (!var12 && this.overrideChat == 0) {
					try {
						String var17 = WordPack.unpack(arg3, 118, var16);
						String var18 = WordFilter.filter(var17, 0);
						arg4.chat = var18;
						arg4.chatColor = var6 >> 8;
						arg4.chatStyle = var6 & 0xFF;
						arg4.chatTimer = 150;
						if (var15 > 1) {
							this.addMessage(1, var18, (byte) 4, arg4.name);
						} else {
							this.addMessage(2, var18, (byte) 4, arg4.name);
						}
					} catch (Exception var14) {
						signlink.reporterror("cde2");
					}
				}
			}
			arg3.pos = var9 + var16;
		}
		if ((arg2 & 0x100) == 256) {
			arg4.spotanimId = arg3.g2();
			var6 = arg3.g4();
			arg4.spotanimOffset = var6 >> 16;
			arg4.spotanimLastCycle = loopCycle + (var6 & 0xFFFF);
			arg4.spotanimFrame = 0;
			arg4.spotanimCycle = 0;
			if (arg4.spotanimLastCycle > loopCycle) {
				arg4.spotanimFrame = -1;
			}
			if (arg4.spotanimId == 65535) {
				arg4.spotanimId = -1;
			}
		}
		if ((arg2 & 0x200) != 512) {
			return;
		}
		arg4.forceMoveStartSceneTileX = arg3.g1();
		arg4.forceMoveStartSceneTileZ = arg3.g1();
		arg4.forceMoveEndSceneTileX = arg3.g1();
		arg4.forceMoveEndSceneTileZ = arg3.g1();
		arg4.forceMoveEndCycle = arg3.g2() + loopCycle;
		arg4.forceMoveStartCycle = arg3.g2() + loopCycle;
		arg4.forceMoveFaceDirection = arg3.g1();
		arg4.pathLength = 0;
		arg4.pathTileX[0] = arg4.forceMoveEndSceneTileX;
		arg4.pathTileZ[0] = arg4.forceMoveEndSceneTileZ;
	}

	@ObfuscatedName("client.a(ZLjava/lang/String;I)V")
	public final void drawProgress(boolean arg0, String arg1, int arg2) {
		this.loadTitle((byte) 99);
		if (this.archiveTitle == null) {
			super.drawProgress(true, arg1, arg2);
			return;
		}
		this.imageTitle4.bind((byte) 62);
		short var4 = 360;
		short var5 = 200;
		byte var6 = 20;
		this.fontBold12.drawStringCenter(var5 / 2 - 26 - var6, (byte) 6, 16777215, "RuneScape is loading - please wait...", var4 / 2);
		this.ingame &= arg0;
		int var7 = var5 / 2 - 18 - var6;
		Pix2D.drawRect(3, var4 / 2 - 152, 9179409, 34, var7, 304);
		Pix2D.drawRect(3, var4 / 2 - 151, 0, 32, var7 + 1, 302);
		Pix2D.fillRect(var7 + 2, var4 / 2 - 150, 9179409, (byte) 93, arg2 * 3, 30);
		Pix2D.fillRect(var7 + 2, var4 / 2 - 150 + arg2 * 3, 0, (byte) 93, 300 - arg2 * 3, 30);
		this.fontBold12.drawStringCenter(var5 / 2 + 5 - var6, (byte) 6, 16777215, arg1, var4 / 2);
		this.imageTitle4.draw(186, super.graphics, 214, 5193);
		if (!this.redrawBackground) {
			return;
		}
		this.redrawBackground = false;
		if (!this.flameActive) {
			this.imageTitle0.draw(0, super.graphics, 0, 5193);
			this.imageTitle1.draw(0, super.graphics, 661, 5193);
		}
		this.imageTitle2.draw(0, super.graphics, 128, 5193);
		this.imageTitle3.draw(386, super.graphics, 214, 5193);
		this.imageTitle5.draw(265, super.graphics, 0, 5193);
		this.imageTitle6.draw(265, super.graphics, 574, 5193);
		this.imageTitle7.draw(186, super.graphics, 128, 5193);
		this.imageTitle8.draw(186, super.graphics, 574, 5193);
	}

	public static final void main(String[] arg0) {
		try {
			System.out.println("RS2 user client - release #" + 225);
			if (arg0.length == 4) {
				nodeId = Integer.parseInt(arg0[0]);
				portOffset = Integer.parseInt(arg0[1]);
				if (arg0[2].equals("lowmem")) {
					setLowMemory(true);
				} else if (arg0[2].equals("highmem")) {
					setHighMemory(9);
				} else {
					System.out.println("Usage: node-id, port-offset, [lowmem/highmem], [free/members]");
					return;
				}
				if (arg0[3].equals("free")) {
					members = false;
				} else if (arg0[3].equals("members")) {
					members = true;
				} else {
					System.out.println("Usage: node-id, port-offset, [lowmem/highmem], [free/members]");
					return;
				}
				signlink.startpriv(InetAddress.getLocalHost());
				client var1 = new client();
				var1.initApplication(532, 789, 0);
			} else {
				System.out.println("Usage: node-id, port-offset, [lowmem/highmem], [free/members]");
			}
		} catch (Exception var2) {
		}
	}

	static {
		int var0 = 0;
		for (int var1 = 0; var1 < 99; var1++) {
			int var2 = var1 + 1;
			int var3 = (int) ((double) var2 + Math.pow(2.0D, (double) var2 / 7.0D) * 300.0D);
			var0 += var3;
			levelExperience[var1] = var0 / 4;
		}
		field1235 = true;
		field1236 = 15;
		nodeId = 10;
		members = true;
		field1264 = 629;
		RSA_EXPONENT = new BigInteger("58778699976184461502525193738213253649000149147835990136706041084440742975821");
		field1307 = new int[][] { { 6798, 107, 10283, 16, 4797, 7744, 5799, 4634, 33697, 22433, 2983, 54193 }, { 8741, 12, 64030, 43162, 7735, 8404, 1701, 38430, 24094, 10153, 56621, 4783, 1341, 16578, 35003, 25239 }, { 25238, 8742, 12, 64030, 43162, 7735, 8404, 1701, 38430, 24094, 10153, 56621, 4783, 1341, 16578, 35003 }, { 4626, 11146, 6439, 12, 4758, 10270 }, { 4550, 4537, 5681, 5673, 5790, 6806, 8076, 4574 } };
		field1323 = 3;
		RSA_MODULUS = new BigInteger("7162900525229798032761816791230527296329313291232324290237849263501208207972894053929065636522363163621000728841182238772712427862772219676577293600221789");
		field1438 = new int[] { 9104, 10275, 7595, 3610, 7975, 8526, 918, 38802, 24466, 10145, 58654, 5027, 1457, 16565, 34991, 25486 };
		field1481 = 3;
	}
}
