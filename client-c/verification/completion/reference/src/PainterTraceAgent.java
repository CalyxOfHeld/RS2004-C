import java.lang.instrument.*;
import java.security.ProtectionDomain;
import jdk.internal.org.objectweb.asm.*;

/** Java-8-only observation hooks; original source files remain unchanged. */
public final class PainterTraceAgent {
    public static void premain(String args,Instrumentation instrumentation){
        instrumentation.addTransformer(new ClassFileTransformer(){
            public byte[] transform(ClassLoader loader,String name,Class<?> type,ProtectionDomain domain,byte[] bytes){
                if(!name.equals("jagex2/graphics/Model") && !name.equals("jagex2/dash3d/World3D"))return null;
                ClassReader reader=new ClassReader(bytes);ClassWriter writer=new ClassWriter(reader,ClassWriter.COMPUTE_MAXS);final int[] count={0};
                reader.accept(new ClassVisitor(Opcodes.ASM5,writer){
                    @Override public MethodVisitor visitMethod(int access,String method,String descriptor,String signature,String[] exceptions){
                        MethodVisitor original=super.visitMethod(access,method,descriptor,signature,exceptions);
                        final String hook;
                        if(name.equals("jagex2/graphics/Model") && method.equals("draw") && descriptor.equals("(IIIIIIIII)V"))hook="model";
                        else if(name.equals("jagex2/dash3d/World3D") && method.equals("draw") && descriptor.equals("(IIIIIII)V"))hook="begin";
                        else if(name.equals("jagex2/dash3d/World3D") && method.equals("drawTileUnderlay") && descriptor.equals("(Ljagex2/dash3d/type/TileUnderlay;IIIIIII)V"))hook="underlay";
                        else if(name.equals("jagex2/dash3d/World3D") && method.equals("drawTileOverlay") && descriptor.equals("(IILjagex2/dash3d/type/TileOverlay;IIIIZ)V"))hook="overlay";
                        else if(name.equals("jagex2/dash3d/World3D") && method.equals("drawTile") && descriptor.equals("(Ljagex2/dash3d/type/Ground;Z)V"))hook="tileStart";
                        else return original;
                        count[0]++;
                        return new MethodVisitor(Opcodes.ASM5,original){
                            @Override public void visitCode(){
                                super.visitCode();
                                visitVarInsn(Opcodes.ALOAD,0);
                                if(hook.equals("model")){
                                    for(int i=1;i<=9;i++)visitVarInsn(Opcodes.ILOAD,i);
                                    visitMethodInsn(Opcodes.INVOKESTATIC,"PainterTraceHooks","model","(Ljagex2/graphics/Model;IIIIIIIII)V",false);
                                }else if(hook.equals("begin")){
                                    for(int i=1;i<=7;i++)visitVarInsn(Opcodes.ILOAD,i);
                                    visitMethodInsn(Opcodes.INVOKESTATIC,"PainterTraceHooks","begin","(Ljagex2/dash3d/World3D;IIIIIII)V",false);
                                }else if(hook.equals("underlay")){
                                    visitVarInsn(Opcodes.ALOAD,1);for(int i=2;i<=8;i++)visitVarInsn(Opcodes.ILOAD,i);
                                    visitMethodInsn(Opcodes.INVOKESTATIC,"PainterTraceHooks","underlay","(Ljagex2/dash3d/World3D;Ljagex2/dash3d/type/TileUnderlay;IIIIIII)V",false);
                                }else if(hook.equals("tileStart")){
                                    visitVarInsn(Opcodes.ALOAD,1);
                                    visitMethodInsn(Opcodes.INVOKESTATIC,"PainterTraceHooks","tileStart","(Ljagex2/dash3d/World3D;Ljagex2/dash3d/type/Ground;)V",false);
                                }else{
                                    for(int i=1;i<=8;i++)visitVarInsn(i==3?Opcodes.ALOAD:Opcodes.ILOAD,i);
                                    visitMethodInsn(Opcodes.INVOKESTATIC,"PainterTraceHooks","overlay","(Ljagex2/dash3d/World3D;IILjagex2/dash3d/type/TileOverlay;IIIIZ)V",false);
                                }
                            }
                            @Override public void visitInsn(int opcode){
                                if(hook.equals("begin") && opcode==Opcodes.RETURN)visitMethodInsn(Opcodes.INVOKESTATIC,"PainterTraceHooks","end","()V",false);
                                super.visitInsn(opcode);
                            }
                        };
                    }
                },0);
                int expected=name.equals("jagex2/graphics/Model")?1:4;
                if(count[0]!=expected)throw new IllegalStateException("unexpected observation hook count "+name+" "+count[0]);
                System.err.println("PainterTraceAgent: "+name+" observation hooks="+count[0]);return writer.toByteArray();
            }
        });
    }
}
