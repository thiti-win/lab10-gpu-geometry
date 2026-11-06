#include "../src/core/mesh.h"
#include "../external/mc_tables.h"
#include <map>
#include <iostream>
float box(vec3 p,vec3 b) { vec3 q=abs(p)-b; return length(max(q,0))+min(max(q.x,max(q.y,q.z)),0); }
float field(vec3 p) {
    float body=box(p,{.95f,.55f,.55f})-.05f;
    float bump=length(p-vec3{.35f,.64f,0})-.4f,k=.15f;
    float h=clamp(.5f+.5f*(bump-body)/k,0,1); body=mix(bump,body,h)-k*h*(1-h);
    float hole=sqrtf(p.x*p.x+p.y*p.y)-.3f; body=max(body,-hole);
    float groove=box(p-vec3{-.55f,.57f,0},{.065f,.10f,.8f}); return max(body,-groove);
}
Mesh generate(float offset) {
    const int nx=81,ny=65,nz=57; const float step=.03f;
    const int corner[8][3]={{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0,0,1},{1,0,1},{1,1,1},{0,1,1}};
    const int edge[12][2]={{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
    auto id=[&](int x,int y,int z) {return (z*ny+y)*nx+x;};
    auto position=[&](int x,int y,int z) {return vec3{(x-40)*step+offset,(y-26)*step+offset*.7f,(z-28)*step+offset*.3f};};
    std::vector<float> values(nx*ny*nz);
    for(int z=0;z<nz;++z) for(int y=0;y<ny;++y) for(int x=0;x<nx;++x) values[id(x,y,z)]=field(position(x,y,z));
    Mesh m; std::map<std::pair<int,int>,int> welded;
    for(int z=0;z<nz-1;++z) for(int y=0;y<ny-1;++y) for(int x=0;x<nx-1;++x) {
        int ids[8],mask=0; vec3 p[8]; float f[8];
        for(int c=0;c<8;++c) { int xx=x+corner[c][0],yy=y+corner[c][1],zz=z+corner[c][2]; ids[c]=id(xx,yy,zz); p[c]=position(xx,yy,zz); f[c]=values[ids[c]]; if(f[c]<0) mask|=1<<c; }
        const int* table=triangle_table[mask];
        for(int j=0;j<16 && table[j]>=0;j+=3) {
            int vs[3];
            for(int t=0;t<3;++t) { int a=edge[table[j+t]][0],b=edge[table[j+t]][1]; auto key=std::minmax(ids[a],ids[b]);
                auto it=welded.find(key); if(it!=welded.end()) vs[t]=it->second;
                else { vs[t]=(int)m.positions.size(); welded[key]=vs[t]; m.positions.push_back(mix(p[a],p[b],f[a]/(f[a]-f[b]))); }
            }
            // Case tables point inward for negative-inside fields. Orient outward.
            m.tris.push_back({vs[0],vs[2],vs[1]});
        }
    }
    return m;
}
int main(int argc,char** argv) { try {
    for(int attempt=0;attempt<12;++attempt) {
        auto m=generate(.00137f+attempt*.000213f); auto s=stats(m);
        std::cout<<"attempt="<<attempt<<" V="<<s.V<<" E="<<s.E<<" F="<<s.F<<" chi="<<s.chi<<" boundary="<<s.boundary<<" nonmanifold="<<s.nonmanifold<<" closed_manifold="<<s.manifold<<'\n';
        if(s.manifold && s.chi==0 && s.F>=20000 && s.F<=40000) { write_obj(m,argc>1?argv[1]:"data/part.obj"); return 0; }
    }
    throw std::runtime_error("No closed genus-one mesh in the requested triangle range");
} catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; } }
