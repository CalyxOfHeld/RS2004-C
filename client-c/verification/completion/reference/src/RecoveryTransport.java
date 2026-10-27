import java.io.*;
import java.net.*;
import java.util.*;
// Diagnostic transport only: original ClientStream/login code stays unchanged.
public final class RecoveryTransport extends Socket {
 public final byte[] loginInput,expectedLogin;
 public final ByteArrayOutputStream loginOutput=new ByteArrayOutputStream(),gameOutput=new ByteArrayOutputStream();
 private final ByteArrayOutputStream received=new ByteArrayOutputStream();
 private int loginRead,gameRead;public boolean closed,failAvailable,overread;public OutputStream gameSink;public String mismatch;
 public RecoveryTransport(byte[] input,byte[] expected){loginInput=input.clone();expectedLogin=expected.clone();}
 public synchronized void release(byte[] bytes){received.write(bytes,0,bytes.length);notifyAll();}
 public void release(ByteArrayOutputStream bytes){release(bytes.toByteArray());}
 public synchronized int consumed(){return gameRead;}
 public synchronized boolean handshakeComplete(){return loginOutput.size()==expectedLogin.length;}
 public synchronized byte[] gameWritten(){return gameOutput.toByteArray();}
 private synchronized int next()throws IOException {
  if(closed)throw new IOException("diagnostic socket closed");
  if(loginRead<loginInput.length){
   // The final response waits for the actual original writer's login packet.
   if(loginRead==8){long deadline=System.currentTimeMillis()+3000;
    while(!handshakeComplete() && mismatch==null && System.currentTimeMillis()<deadline)try{wait(1);}catch(InterruptedException ex){throw new IOException(ex);}
    if(mismatch!=null)throw new IOException(mismatch);
    if(!handshakeComplete())throw new IOException("login writer did not complete");
   }
   return loginInput[loginRead++]&255;
  }
  if(gameRead>=received.size()){overread=true;throw new IOException("diagnostic original read past released game bytes");}
  return received.toByteArray()[gameRead++]&255;
 }
 private final InputStream input=new InputStream(){
  public int available()throws IOException{synchronized(RecoveryTransport.this){
   if(failAvailable && gameRead>=received.size()){failAvailable=false;throw new IOException("controlled original receive failure");}
   return loginRead<loginInput.length?loginInput.length-loginRead:received.size()-gameRead;}}
  public int read()throws IOException{return next();}
  public int read(byte[] b,int off,int len)throws IOException{if(len==0)return 0;for(int i=0;i<len;i++)b[off+i]=(byte)next();return len;}
  public void close()throws IOException{RecoveryTransport.this.close();}
 };
 private final OutputStream output=new OutputStream(){
  public void write(int value)throws IOException{synchronized(RecoveryTransport.this){
   if(closed)throw new IOException("diagnostic socket closed");
   int offset=loginOutput.size();
   if(offset<expectedLogin.length){
    if((expectedLogin[offset]&255)!=(value&255)){
     mismatch="original login differs at byte "+offset;RecoveryTransport.this.notifyAll();throw new IOException(mismatch);}
    loginOutput.write(value);
   }else {gameOutput.write(value);if(gameSink!=null)gameSink.write(value);}
   RecoveryTransport.this.notifyAll();
  }}
  public void write(byte[] b,int off,int len)throws IOException{for(int i=0;i<len;i++)write(b[off+i]);}
  public void close()throws IOException{RecoveryTransport.this.close();}
 };
 public InputStream getInputStream(){return input;}
 public OutputStream getOutputStream(){return output;}
 public void setSoTimeout(int timeout){}
 public void setTcpNoDelay(boolean on){}
 public synchronized void close(){closed=true;notifyAll();}
}
