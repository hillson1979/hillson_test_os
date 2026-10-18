#include "GL/gl.h"
#include "graph_user.h"
static GLfloat clear_red, clear_green, clear_blue, clear_alpha;
static graph_display_info_t display;
static uint32_t framebuffer[1024 * 768];
static uint32_t depthbuffer[1024 * 768];
static int initialized;
static const GLVertex2D* vertex_data;
static GLsizei vertex_count;
static const uint32_t* texture_pixels;
static int texture_width, texture_height, texture_enabled;
static int depth_enabled;
static int rotation_step;
static GLMatrix4 modelview, projection;
static GLenum matrix_mode=GL_MODELVIEW;
static int matrices_ready;
static int32_t mul16(int32_t a,int32_t b){return (int32_t)(((int64_t)a*b)>>16);}
static void matrix_identity(GLMatrix4* m){int i;for(i=0;i<16;++i)m->m[i]=(i%5==0)?65536:0;}
static void matrix_init(void){if(!matrices_ready){matrix_identity(&modelview);matrix_identity(&projection);matrices_ready=1;}}
static GLMatrix4* active_matrix(void){return matrix_mode==GL_PROJECTION?&projection:&modelview;}
void glMatrixMode(GLenum mode){matrix_init();matrix_mode=mode;}
void glLoadIdentity(void){matrix_init();matrix_identity(active_matrix());}
void glTranslateFixed(int x,int y,int z){GLMatrix4* m;matrix_init();m=active_matrix();m->m[12]+=x<<8;m->m[13]+=y<<8;m->m[14]+=z<<8;}
void glRotateYStep(int step){GLMatrix4* m;static const int c[8]={65536,57344,40960,24576,0,-24576,-40960,-57344};static const int s[8]={0,24576,40960,57344,65536,57344,40960,24576};int r=step&7;matrix_init();m=active_matrix();m->m[0]=c[r];m->m[2]=s[r];m->m[8]=-s[r];m->m[10]=c[r];}
void glPerspectiveFixed(int focal_length){matrix_init();projection.m[0]=focal_length<<8;projection.m[5]=focal_length<<8;projection.m[10]=65536;projection.m[11]=256;projection.m[14]=-(focal_length<<8);projection.m[15]=0;}
static void ensure_init(void) {
    if (!initialized) initialized = graph_get_display_info(&display) == 0;
}
void glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { clear_red=r; clear_green=g; clear_blue=b; clear_alpha=a; }
void glClear(GLenum mask) {
    (void)mask;
    ensure_init();
    if (initialized && (mask & GL_COLOR_BUFFER_BIT)) {
        uint32_t r=(uint32_t)(clear_red*255.0f), g=(uint32_t)(clear_green*255.0f), b=(uint32_t)(clear_blue*255.0f);
        uint32_t y, x, color = 0xFF000000u | (r<<16) | (g<<8) | b;
        if (display.width > 1024 || display.height > 768) return;
        for (y=0; y<display.height; ++y) for (x=0; x<display.width; ++x) framebuffer[y*1024+x] = color;
    }
    if (initialized && (mask & GL_DEPTH_BUFFER_BIT)) {
        uint32_t y,x;
        for(y=0;y<display.height;++y)for(x=0;x<display.width;++x)depthbuffer[y*1024+x]=0xFFFFFFFFu;
    }
}
void glEnable(GLenum capability) { if(capability==GL_TEXTURE_2D) texture_enabled=1; if(capability==GL_DEPTH_TEST)depth_enabled=1; }
void glViewport(int x, int y, int width, int height) { (void)x; (void)y; (void)width; (void)height; ensure_init(); }
void glVertexData2D(const GLVertex2D* vertices, GLsizei count) { vertex_data = vertices; vertex_count = count; }
void glTexImage2D(GLsizei width, GLsizei height, const uint32_t* pixels) {
    if(width>0&&height>0&&pixels){texture_width=width;texture_height=height;texture_pixels=pixels;}
}
static int edge(int ax, int ay, int bx, int by, int px, int py) {
    return (px-ax)*(by-ay) - (py-ay)*(bx-ax);
}
static uint32_t interpolate_color(const GLVertex2D* a, const GLVertex2D* b,
                                  const GLVertex2D* c, int wa, int wb, int wc,
                                  int area) {
    int ar=(a->color>>16)&255, ag=(a->color>>8)&255, ab=a->color&255;
    int br=(b->color>>16)&255, bg=(b->color>>8)&255, bb=b->color&255;
    int cr=(c->color>>16)&255, cg=(c->color>>8)&255, cb=c->color&255;
    uint32_t r=(uint32_t)((ar*wa+br*wb+cr*wc)/area);
    uint32_t g=(uint32_t)((ag*wa+bg*wb+cg*wc)/area);
    uint32_t bl=(uint32_t)((ab*wa+bb*wb+cb*wc)/area);
    return 0xFF000000u|(r<<16)|(g<<8)|bl;
}
static uint32_t sample_texture(const GLVertex2D* a, const GLVertex2D* b,
                               const GLVertex2D* c, int wa, int wb, int wc,
                               int area) {
    int q=(a->q*wa+b->q*wb+c->q*wc)/area;
    int u=(a->uq*wa+b->uq*wb+c->uq*wc)/area;
    int v=(a->vq*wa+b->vq*wb+c->vq*wc)/area;
    if(q>0){u=(u<<16)/q;v=(v<<16)/q;}
    int tx=(u*(texture_width-1))>>16, ty=(v*(texture_height-1))>>16;
    if(tx<0)tx=0; if(ty<0)ty=0; if(tx>=texture_width)tx=texture_width-1; if(ty>=texture_height)ty=texture_height-1;
    return texture_pixels[ty*texture_width+tx];
}
static void triangle(const GLVertex2D* a, const GLVertex2D* b, const GLVertex2D* c) {
    int minx=a->x, maxx=a->x, miny=a->y, maxy=a->y, x, y;
    int area=edge(a->x,a->y,b->x,b->y,c->x,c->y), sign=1;
    if(area==0)return; if(area<0){area=-area;sign=-1;}
    if(b->x<minx)minx=b->x; if(c->x<minx)minx=c->x; if(b->x>maxx)maxx=b->x; if(c->x>maxx)maxx=c->x;
    if(b->y<miny)miny=b->y; if(c->y<miny)miny=c->y; if(b->y>maxy)maxy=b->y; if(c->y>maxy)maxy=c->y;
    if(minx<0)minx=0; if(miny<0)miny=0; if(maxx>=(int)display.width)maxx=(int)display.width-1; if(maxy>=(int)display.height)maxy=(int)display.height-1;
    for(y=miny;y<=maxy;++y) for(x=minx;x<=maxx;++x) {
        int e0=edge(a->x,a->y,b->x,b->y,x,y)*sign;
        int e1=edge(b->x,b->y,c->x,c->y,x,y)*sign;
        int e2=edge(c->x,c->y,a->x,a->y,x,y)*sign;
        if(e0>=0&&e1>=0&&e2>=0) {
            uint32_t index=(uint32_t)y*1024u+(uint32_t)x;
            uint32_t depth=(uint32_t)(((uint64_t)a->depth*e1+(uint64_t)b->depth*e2+(uint64_t)c->depth*e0)/(uint32_t)area);
            if(!depth_enabled||depth<depthbuffer[index]) {
                if(depth_enabled)depthbuffer[index]=depth;
                framebuffer[index]=(texture_enabled&&texture_pixels)?sample_texture(a,b,c,e1,e2,e0,area):interpolate_color(a,b,c,e1,e2,e0,area);
            }
        }
    }
}
void glDrawArrays(GLenum mode, int first, GLsizei count) {
    int i; ensure_init();
    if(!initialized || mode!=GL_TRIANGLES || !vertex_data || first<0 || count<3 || first+count>vertex_count) return;
    for(i=first;i+2<first+count;i+=3) triangle(&vertex_data[i],&vertex_data[i+1],&vertex_data[i+2]);
}
int glPresent(void) { ensure_init(); return initialized ? graph_present_frame(framebuffer, display.width, display.height, 1024 * 4) : -1; }

void glDrawTriangles3D(const GLVertex3D* vertices, GLsizei count) {
    GLVertex2D projected[36];
    int i;
    int32_t x,y,z,ox,oy,oz; matrix_init(); ensure_init();
    if(!initialized||!vertices||count<3||count>36)return;
    for(i=0;i<count;++i) {
        /* Fixed-point Y rotation (cos ~= 7/8, sin ~= 3/8) exposes cube faces. */
        static const int cs[8]={8,7,5,3,0,-3,-5,-7};
        static const int sn[8]={0,3,5,7,8,7,5,3};
        int r=rotation_step&7;
        ox=vertices[i].x<<8; oy=vertices[i].y<<8; oz=vertices[i].z<<8;
        {
            int32_t oldx=ox, oldz=oz;
            ox=(oldx*cs[r]+oldz*sn[r])/8;
            oz=(oldz*cs[r]-oldx*sn[r])/8;
        }
        x=mul16(modelview.m[0],ox)+mul16(modelview.m[4],oy)+mul16(modelview.m[8],oz)+modelview.m[12];
        y=mul16(modelview.m[1],ox)+mul16(modelview.m[5],oy)+mul16(modelview.m[9],oz)+modelview.m[13];
        z=mul16(modelview.m[2],ox)+mul16(modelview.m[6],oy)+mul16(modelview.m[10],oz)+modelview.m[14];
        int rx=x>>8, rz=z>>8;
        z=rz+512;
        if(z<32)z=32;
        int focal = projection.m[0] ? (projection.m[0] >> 8) : 420;
        int clip_w = z > 0 ? z : 1;
        int clip_x = rx * focal;
        int clip_y = (y >> 8) * focal;
        projected[i].x=(int)display.width/2+clip_x/clip_w;
        projected[i].y=(int)display.height/2-clip_y/clip_w;
        projected[i].color=vertices[i].color;
        projected[i].u=projected[i].v=0;
        projected[i].depth=(uint32_t)clip_w;
        projected[i].q=(1<<16)/clip_w;
        projected[i].uq=(vertices[i].u*projected[i].q)>>16;
        projected[i].vq=(vertices[i].v*projected[i].q)>>16;
    }
    glVertexData2D(projected,count);
    glDrawArrays(GL_TRIANGLES,0,count);
}
void glSetRotationStep(int step) { rotation_step=step&7; }
