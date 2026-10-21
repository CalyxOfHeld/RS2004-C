// Observational harness only: delegates unchanged production decoder/handler behavior.
import repository from '../../../../engine/src/network/game/client/ClientGameProtRepository.ts';
const decoder = repository.getDecoder.bind(repository);
const handler = repository.getHandler.bind(repository);
const observed = new Set([4,52,155,195,71,133,157,211,31,59,212,38,6,181,108,235,93,194,8,27,113,100,245,172,96,97,116,164,53,185,206]);
repository.getDecoder = function(prot: any) {
    const original=decoder(prot);
    if(!original || !observed.has(prot.id)) return original;
    return {
        decode(buf: any,length: number) {
            const message=original.decode(buf,length);
            console.log('[OBSERVE-DECODE]',JSON.stringify({opcode:prot.id,message}));
            return message;
        }
    } as any;
};
repository.getHandler = function(prot: any) {
    const original=handler(prot);
    if(!original || !observed.has(prot.id)) return original;
    return {
        handle(message: any,player: any) {
            const result=original.handle(message,player);
            console.log('[OBSERVE-HANDLER]',JSON.stringify({opcode:prot.id,accepted:result,account:player.username,lastCom:player.lastCom,lastSlot:player.lastSlot,lastItem:player.lastItem}));
            return result;
        }
    } as any;
};
await import('../../../../engine/src/app.ts');

const {default:world}=await import('../../../../engine/src/engine/World.ts');
setTimeout(()=>{for(const npc of world.npcs) if(npc.type===0) console.log('[OBSERVE-NPC]',JSON.stringify({id:npc.nid,type:npc.type,x:npc.x,z:npc.z,level:npc.level}));},12000);
