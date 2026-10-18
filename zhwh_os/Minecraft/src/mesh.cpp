#include "minecraft/mesh.hpp"
namespace minecraft {
static bool solid(const Chunk& c,int x,int y,int z){return x>=0&&x<kChunkSize&&y>=0&&y<kChunkHeight&&z>=0&&z<kChunkSize&&c.get(x,y,z)!=Block::Air;}
static uint32_t color(Block b,int face){if(b==Block::Grass)return face==4?0xFF70B850u:0xFF4E8A38u;if(b==Block::Dirt)return 0xFF8A5A32u;if(b==Block::Stone)return 0xFF808080u;return 0;}
static void quad(Mesh& m,int x,int y,int z,int face,uint32_t c){static const int faces[6][4][3]={{{1,0,0},{1,1,0},{1,1,1},{1,0,1}},{{0,0,1},{0,1,1},{0,1,0},{0,0,0}},{{0,1,0},{0,1,1},{1,1,1},{1,1,0}},{{0,0,1},{1,0,1},{1,0,0},{0,0,0}},{{1,0,1},{1,1,1},{0,1,1},{0,0,1}},{{0,0,0},{0,1,0},{1,1,0},{1,0,0}}}; static const int idx[6]={0,1,2,0,2,3}; for(int i=0;i<6;++i){auto&p=faces[face][idx[i]];m.vertices.push_back({x+p[0],y+p[1],z+p[2],c,(i==0||i==3)?0:65536,(i<2||i==5)?0:65536});}}
Mesh buildChunkMesh(const Chunk& c){Mesh m;for(int y=0;y<kChunkHeight;++y)for(int z=0;z<kChunkSize;++z)for(int x=0;x<kChunkSize;++x){Block b=c.get(x,y,z);if(b==Block::Air)continue;const int dx[6]={1,-1,0,0,0,0},dy[6]={0,0,1,-1,0,0},dz[6]={0,0,0,0,1,-1};for(int f=0;f<6;++f)if(!solid(c,x+dx[f],y+dy[f],z+dz[f]))quad(m,x,y,z,f,color(b,f));}return m;}
}
