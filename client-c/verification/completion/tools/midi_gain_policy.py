from pathlib import Path
import ctypes,struct,json,argparse
parser=argparse.ArgumentParser()
parser.add_argument('--library',required=True);parser.add_argument('--fixture',required=True);parser.add_argument('--outdir',required=True)
args=parser.parse_args();run=Path(args.outdir);run.mkdir(parents=True,exist_ok=False)
dll=ctypes.CDLL(str(Path(args.library).resolve()));fn=dll.midi_gain225
fn.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_int,ctypes.POINTER(ctypes.c_void_p),ctypes.POINTER(ctypes.c_size_t)];fn.restype=ctypes.c_bool;dll.gain_release225.argtypes=[ctypes.c_void_p]

def events(data):
 if data[:4]==b'RIFF':
  at=12
  while data[at:at+4]!=b'data':n=int.from_bytes(data[at+4:at+8],'little');at+=8+n+(n&1)
  n=int.from_bytes(data[at+4:at+8],'little');data=data[at+8:at+8+n]
 assert data[:4]==b'MThd';header=int.from_bytes(data[4:8],'big');tracks=int.from_bytes(data[10:12],'big');signature=data[8:8+header];at=8+header;result=[]
 while len(result)<tracks:
  kind=data[at:at+4];n=int.from_bytes(data[at+4:at+8],'big');track=data[at+8:at+8+n];at+=8+n
  if kind!=b'MTrk':continue
  p=0;clock=0;running=None;entries=[]
  def vlq():
   nonlocal p
   value=0
   for _ in range(4):
    byte=track[p];p+=1;value=(value<<7)|(byte&127)
    if byte<128:return value
   raise ValueError('vlq')
  while p<len(track):
   clock+=vlq();status=track[p]
   if status>=128:
    p+=1
    if status<240:running=status
   else:status=running
   assert status is not None
   if status<240:
    n=1 if status>>4 in (12,13) else 2;value=track[p:p+n];assert len(value)==n;p+=n
   elif status==255:
    typ=track[p];p+=1;n=vlq();value=bytes([typ])+track[p:p+n];p+=n
   elif status in (240,247):n=vlq();value=track[p:p+n];p+=n
   else:raise ValueError(hex(status))
   entries.append((clock,status,value))
  result.append(entries)
 return signature,result
def cc7(event):return event[1]>>4==11 and event[2][0]==7
fixture=Path(args.fixture).read_bytes();assert fixture[:4]==b'MID1';count=int.from_bytes(fixture[4:8],'big');pos=8;rows=[];outdir=run/'converted';outdir.mkdir()
for row in range(count):
 n=int.from_bytes(fixture[pos:pos+2],'big');pos+=2;name=fixture[pos:pos+n].decode();pos+=n;packed,crc,n=struct.unpack_from('>III',fixture,pos);pos+=12;source=fixture[pos:pos+n];pos+=n;original=events(source)
 for level in (255,161,102,64,0):
  src=ctypes.create_string_buffer(source);out=ctypes.c_void_p();size=ctypes.c_size_t();assert fn(src,len(source),level,ctypes.byref(out),ctypes.byref(size)),(name,level)
  converted=ctypes.string_at(out,size.value);dll.gain_release225(out)
  if level==255:assert converted==source
  signature,tracks=events(converted);assert signature==original[0]
  controls=0
  for index,(old,new) in enumerate(zip(original[1],tracks)):
   assert [e for e in old if not cc7(e)]==[e for e in new if not cc7(e)],(name,level,index,'note/tempo/ticks')
   expected=[(e[0],e[1],bytes([7,(e[2][1]*level+127)//255])) for e in old if cc7(e)]
   if level!=255 and index==0:expected=[(0,176+i,bytes([7,(100*level+127)//255])) for i in range(16)]+expected
   assert [e for e in new if cc7(e)]==expected,(name,level,index,'channel volume')
   controls+=len(expected)
  (outdir/(name.replace('/','_')+'.'+str(level))).write_bytes(converted);rows.append(dict(name=name,level=level,bytes=len(converted),control_events=controls))
(run/'result.json').write_text(json.dumps(rows,indent=2));print('PASS',count,'original Java cache MIDI/RMID files at5 levels;1150 outputs preserve all non-volume events/ticks/tempo andhave expected CC7 policy;full volume byte-identical')

# Malformed event streams and invalid API inputs must fail without returning output.
for source,level in [(None,64),(b'MThd'+bytes(10),64),(b'MThd'+bytes(10),-1),(b'MThd'+bytes(10),256),(b'MThd'+bytes.fromhex('00000006000000010060')+b'MTrk'+bytes.fromhex('000000058080808000'),64)]:
    src=ctypes.create_string_buffer(source) if source is not None else None
    out=ctypes.c_void_p();n=ctypes.c_size_t()
    assert not fn(src,len(source) if source else 0,level,ctypes.byref(out),ctypes.byref(n))
    assert not out.value and n.value==0
print('PASS malformed MIDI/VLQ/null/volume guards')
