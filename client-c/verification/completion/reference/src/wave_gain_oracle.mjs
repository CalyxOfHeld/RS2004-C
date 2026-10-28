import fs from 'node:fs';
let node;
globalThis.window={AudioContext:class {createGain(){node={gain:{value:0},connect(){}};return node;} }};
globalThis.document={addEventListener(){},removeEventListener(){}};
const source=fs.readFileSync(process.argv[2]);
const original=await import('data:text/javascript;base64,'+source.toString('base64'));
const bytes=Buffer.alloc(8+4*12);bytes.writeUInt32BE(0x57473235,0);bytes.writeInt32BE(4,4);
for(let i=0;i<4;i++){original.setWaveVolume(-4*i);bytes.writeInt32BE(-400*i,8+i*12);bytes.writeDoubleBE(node.gain.value,12+i*12);}
fs.writeFileSync(process.argv[3],bytes);
console.log('Original web setWaveVolume:4 gains;unchanged module with browser AudioContext/document boundaries replaced,no device');
