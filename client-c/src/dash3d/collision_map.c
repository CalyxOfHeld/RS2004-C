#include "dash3d/collision_map.h"
#include <string.h>

void collision_map_init(CollisionMap *cm, int size_x, int size_z) {
    if (size_x > COLLISION_MAX_X) size_x = COLLISION_MAX_X;
    if (size_z > COLLISION_MAX_Z) size_z = COLLISION_MAX_Z;
    cm->size_x = size_x;
    cm->size_z = size_z;
    collision_map_reset(cm);
}

void collision_map_reset(CollisionMap *cm) {
    for (int x = 0; x < cm->size_x; x++) {
        for (int z = 0; z < cm->size_z; z++) {
            if (x == 0 || z == 0 || x == cm->size_x - 1 || z == cm->size_z - 1) {
                cm->flags[x][z] = 16777215; // 0xFFFFFF (blocked border)
            } else {
                cm->flags[x][z] = 0;
            }
        }
    }
}

void collision_map_add_loc(CollisionMap *cm, int x, int z, int width, int length, bool blockrange) {
    int mask = 256; // LOC block
    if (blockrange) {
        mask += 131072; // 0x20000
    }
    for (int tx = x; tx < x + width; tx++) {
        if (tx >= 0 && tx < cm->size_x) {
            for (int tz = z; tz < z + length; tz++) {
                if (tz >= 0 && tz < cm->size_z) {
                    cm->flags[tx][tz] |= mask;
                }
            }
        }
    }
}

void collision_map_del_loc(CollisionMap *cm, int x, int z, int width, int length, bool blockrange) {
    int mask = 256;
    if (blockrange) {
        mask += 131072;
    }
    for (int tx = x; tx < x + width; tx++) {
        if (tx >= 0 && tx < cm->size_x) {
            for (int tz = z; tz < z + length; tz++) {
                if (tz >= 0 && tz < cm->size_z) {
                    cm->flags[tx][tz] &= ~mask;
                }
            }
        }
    }
}

// Revision-225 CollisionMap.addWall/delWall: paired directional walk flags
// and, when requested, the corresponding visibility flags nine bits higher.
static void change_flags(CollisionMap *cm,int x,int z,int flags,bool add) {
    if(!cm || x<0 || z<0 || x>=cm->size_x || z>=cm->size_z) return;
    if(add) cm->flags[x][z]|=flags;else cm->flags[x][z]&=0xffffff-flags;
}
void collision_map_block_ground(CollisionMap *cm,int x,int z,bool block) {change_flags(cm,x,z,0x200000,block);}
static void change_wall(CollisionMap *cm,int x,int z,int shape,int angle,bool range,bool add) {
    static const int cardinal[4]={0x80,0x2,0x8,0x20},dx[4]={-1,0,1,0},dz[4]={0,1,0,-1};
    static const int diagonal[4]={0x1,0x4,0x10,0x40},cx[4]={-1,1,1,-1},cz[4]={1,1,-1,-1};
    if(angle<0 || angle>3 || shape<0 || shape>3) return;
    if(shape==0 || shape==2) {
        for(int side=0;side<(shape==2 ? 2:1);side++) {
            int r=(angle+side)&3,own=cardinal[r],opposite=cardinal[(r+2)&3];
            if(range) {own|=own<<9;opposite|=opposite<<9;}
            change_flags(cm,x,z,own,add);change_flags(cm,x+dx[r],z+dz[r],opposite,add);
        }
    } else {
        int own=diagonal[angle],opposite=diagonal[(angle+2)&3];
        if(range) {own|=own<<9;opposite|=opposite<<9;}
        change_flags(cm,x,z,own,add);change_flags(cm,x+cx[angle],z+cz[angle],opposite,add);
    }
}
void collision_map_add_wall(CollisionMap *cm,int x,int z,int shape,int angle,bool range) {change_wall(cm,x,z,shape,angle,range,true);}
void collision_map_del_wall(CollisionMap *cm,int x,int z,int shape,int angle,bool range) {change_wall(cm,x,z,shape,angle,range,false);}

// Direct port: bundled CollisionMap.testWall
bool collision_map_test_wall(const CollisionMap *cm,int srcX,int srcZ,int dstX,int dstZ,int shape,int angle) {
    if(!cm || srcX<0 || srcZ<0 || srcX>=cm->size_x || srcZ>=cm->size_z) return false;
        if (srcX == dstX && srcZ == dstZ) {
            return true;
        }

        int sx = srcX - 0;
        int sz = srcZ - 0;
        int dx = dstX - 0;
        int dz = dstZ - 0;

        if (shape == 0) {
            if (angle == 0) {
                if (sx == dx - 1 && sz == dz) {
                    return true;
                } else if (sx == dx && sz == dz + 1 && (cm->flags[srcX][srcZ] & 0x280120) == 0) {
                    return true;
                } else if (sx == dx && sz == dz - 1 && (cm->flags[srcX][srcZ] & 0x280102) == 0) {
                    return true;
                }
            } else if (angle == 1) {
                if (sx == dx && sz == dz + 1) {
                    return true;
                } else if (sx == dx - 1 && sz == dz && (cm->flags[srcX][srcZ] & 0x280108) == 0) {
                    return true;
                } else if (sx == dx + 1 && sz == dz && (cm->flags[srcX][srcZ] & 0x280180) == 0) {
                    return true;
                }
            } else if (angle == 2) {
                if (sx == dx + 1 && sz == dz) {
                    return true;
                } else if (sx == dx && sz == dz + 1 && (cm->flags[srcX][srcZ] & 0x280120) == 0) {
                    return true;
                } else if (sx == dx && sz == dz - 1 && (cm->flags[srcX][srcZ] & 0x280102) == 0) {
                    return true;
                }
            } else if (angle == 3) {
                if (sx == dx && sz == dz - 1) {
                    return true;
                } else if (sx == dx - 1 && sz == dz && (cm->flags[srcX][srcZ] & 0x280108) == 0) {
                    return true;
                } else if (sx == dx + 1 && sz == dz && (cm->flags[srcX][srcZ] & 0x280180) == 0) {
                    return true;
                }
            }
        } else if (shape == 2) {
            if (angle == 0) {
                if (sx == dx - 1 && sz == dz) {
                    return true;
                } else if (sx == dx && sz == dz + 1) {
                    return true;
                } else if (sx == dx + 1 && sz == dz && (cm->flags[srcX][srcZ] & 0x280180) == 0) {
                    return true;
                } else if (sx == dx && sz == dz - 1 && (cm->flags[srcX][srcZ] & 0x280102) == 0) {
                    return true;
                }
            } else if (angle == 1) {
                if (sx == dx - 1 && sz == dz && (cm->flags[srcX][srcZ] & 0x280108) == 0) {
                    return true;
                } else if (sx == dx && sz == dz + 1) {
                    return true;
                } else if (sx == dx + 1 && sz == dz) {
                    return true;
                } else if (sx == dx && sz == dz - 1 && (cm->flags[srcX][srcZ] & 0x280102) == 0) {
                    return true;
                }
            } else if (angle == 2) {
                if (sx == dx - 1 && sz == dz && (cm->flags[srcX][srcZ] & 0x280108) == 0) {
                    return true;
                } else if (sx == dx && sz == dz + 1 && (cm->flags[srcX][srcZ] & 0x280120) == 0) {
                    return true;
                } else if (sx == dx + 1 && sz == dz) {
                    return true;
                } else if (sx == dx && sz == dz - 1) {
                    return true;
                }
            } else if (angle == 3) {
                if (sx == dx - 1 && sz == dz) {
                    return true;
                } else if (sx == dx && sz == dz + 1 && (cm->flags[srcX][srcZ] & 0x280120) == 0) {
                    return true;
                } else if (sx == dx + 1 && sz == dz && (cm->flags[srcX][srcZ] & 0x280180) == 0) {
                    return true;
                } else if (sx == dx && sz == dz - 1) {
                    return true;
                }
            }
        } else if (shape == 9) {
            if (sx == dx && sz == dz + 1 && (cm->flags[srcX][srcZ] & 0x20) == 0) {
                return true;
            } else if (sx == dx && sz == dz - 1 && (cm->flags[srcX][srcZ] & 0x2) == 0) {
                return true;
            } else if (sx == dx - 1 && sz == dz && (cm->flags[srcX][srcZ] & 0x8) == 0) {
                return true;
            } else if (sx == dx + 1 && sz == dz && (cm->flags[srcX][srcZ] & 0x80) == 0) {
                return true;
            }
        }
        return false;
    }

// Direct port: bundled CollisionMap.testWDecor
bool collision_map_test_decor(const CollisionMap *cm,int srcX,int srcZ,int dstX,int dstZ,int shape,int angle) {
    if(!cm || srcX<0 || srcZ<0 || srcX>=cm->size_x || srcZ>=cm->size_z) return false;
        if (srcX == dstX && srcZ == dstZ) {
            return true;
        }

        int sx = srcX - 0;
        int sz = srcZ - 0;
        int dx = dstX - 0;
        int dz = dstZ - 0;

        if (shape == 6 || shape == 7) {
            if (shape == 7) {
                angle = (angle + 2) & 0x3;
            }

            if (angle == 0) {
                if (sx == dx + 1 && sz == dz && (cm->flags[srcX][srcZ] & 0x80) == 0) {
                    return true;
                } else if (sx == dx && sz == dz - 1 && (cm->flags[srcX][srcZ] & 0x2) == 0) {
                    return true;
                }
            } else if (angle == 1) {
                if (sx == dx - 1 && sz == dz && (cm->flags[srcX][srcZ] & 0x8) == 0) {
                    return true;
                } else if (sx == dx && sz == dz - 1 && (cm->flags[srcX][srcZ] & 0x2) == 0) {
                    return true;
                }
            } else if (angle == 2) {
                if (sx == dx - 1 && sz == dz && (cm->flags[srcX][srcZ] & 0x8) == 0) {
                    return true;
                } else if (sx == dx && sz == dz + 1 && (cm->flags[srcX][srcZ] & 0x20) == 0) {
                    return true;
                }
            } else if (angle == 3) {
                if (sx == dx + 1 && sz == dz && (cm->flags[srcX][srcZ] & 0x80) == 0) {
                    return true;
                } else if (sx == dx && sz == dz + 1 && (cm->flags[srcX][srcZ] & 0x20) == 0) {
                    return true;
                }
            }
        } else if (shape == 8) {
            if (sx == dx && sz == dz + 1 && (cm->flags[srcX][srcZ] & 0x20) == 0) {
                return true;
            } else if (sx == dx && sz == dz - 1 && (cm->flags[srcX][srcZ] & 0x2) == 0) {
                return true;
            } else if (sx == dx - 1 && sz == dz && (cm->flags[srcX][srcZ] & 0x8) == 0) {
                return true;
            } else if (sx == dx + 1 && sz == dz && (cm->flags[srcX][srcZ] & 0x80) == 0) {
                return true;
            }
        }
        return false;
    }

// Direct port: bundled CollisionMap.testLoc
bool collision_map_test_loc(const CollisionMap *cm,int srcX,int srcZ,int dstX,int dstZ,int dstSizeX,int dstSizeZ,int forceapproach) {
    if(!cm || srcX<0 || srcZ<0 || srcX>=cm->size_x || srcZ>=cm->size_z) return false;
        int maxX = dstX + dstSizeX - 1;
        int maxZ = dstZ + dstSizeZ - 1;

        if (srcX >= dstX && srcX <= maxX && srcZ >= dstZ && srcZ <= maxZ) {
            return true;
        } else if (srcX == dstX - 1 && srcZ >= dstZ && srcZ <= maxZ && (cm->flags[srcX][srcZ] & 0x8) == 0 && (forceapproach & 0x8) == 0) {
            return true;
        } else if (srcX == maxX + 1 && srcZ >= dstZ && srcZ <= maxZ && (cm->flags[srcX][srcZ] & 0x80) == 0 && (forceapproach & 0x2) == 0) {
            return true;
        } else if (srcZ == dstZ - 1 && srcX >= dstX && srcX <= maxX && (cm->flags[srcX][srcZ] & 0x2) == 0 && (forceapproach & 0x4) == 0) {
            return true;
        } else if (srcZ == maxZ + 1 && srcX >= dstX && srcX <= maxX && (cm->flags[srcX][srcZ] & 0x20) == 0 && (forceapproach & 0x1) == 0) {
            return true;
        }
        return false;
    }
