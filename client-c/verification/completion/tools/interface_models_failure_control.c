#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int fail_after=-1;
static void *controlled_calloc(size_t n,size_t size){if(fail_after==0)return NULL;if(fail_after>0)fail_after--;void *p=malloc(n*size);if(p)memset(p,0,n*size);return p;}
#define calloc controlled_calloc
#include "../../../src/ui/component_packets.c"
#undef calloc
int main(void){Arena a=arena_create(64*1024*1024);Jagfile *models=jagfile_load_file("../engine/data/pack/client/models",&a),*config=jagfile_load_file("../engine/data/pack/client/config",&a);assert(models && config);model_init_repo(models,&a);ObjConfig objects={0};obj_unpack(&objects,config,true);ObjType object;assert(obj_get(&objects,995,&object));int model_id=object.model;obj_free_entry(&object);Component c={0};c.id=0;c.type=6;c.zoom=1000;ComponentConfig cfg={0};cfg.count=1;cfg.instances=&c;cfg.obj_config=&objects;VarpContext varps={0};uint8_t raw[]={0,0,(uint8_t)(model_id>>8),(uint8_t)model_id},obj[]={0,0,3,227,0,100},recol[]={0,0,0,0,0,1};Component before=c;
 for(int kind=0;kind<2;kind++)for(int fail=0;fail<2;fail++){fail_after=fail;assert(component_apply_packet(&cfg,&varps,&a,kind?46:87,kind?obj:raw,kind?6:4)==-1 && !memcmp(&c,&before,sizeof(c)));fail_after=-1;component_models_clear225(&cfg);}
 int lengths=0;for(int opindex=0;opindex<3;opindex++){int op=(int[]){46,87,103}[opindex],valid=op==87?4:6;for(int n=0;n<=7;n++)if(n!=valid){assert(component_apply_packet(&cfg,&varps,&a,op,obj,(size_t)n)==-1 && !memcmp(&c,&before,sizeof(c)));lengths++;}}
 uint8_t bad_id[]={255,255,0,0};assert(component_apply_packet(&cfg,&varps,&a,87,bad_id,4)==-1);obj[4]=obj[5]=0;assert(component_apply_packet(&cfg,&varps,&a,46,obj,6)==-1);obj[5]=100;assert(component_apply_packet(&cfg,&varps,&a,46,obj,6)==1);assert(component_model_colourless225(&cfg,c.model));Model *saved=c.model;assert(component_apply_packet(&cfg,&varps,&a,103,recol,6)==-1 && c.model==saved);int faces=c.model->num_faces;c.model->num_faces=0;assert(component_apply_packet(&cfg,&varps,&a,103,recol,6)==1);c.model->num_faces=faces;component_models_clear225(&cfg);int owned,cached;component_models_stats225(&cfg,&owned,&cached);assert(!owned && !cached && !c.model && lengths==21);obj_free(&objects);arena_free(&a);puts("PASS interface model safety:4 allocation failures,21 wrong lengths,invalid component/zero divisor/colourless recolour;empty-face no-op,publication retained,cache/model cleanup");return 0;}
