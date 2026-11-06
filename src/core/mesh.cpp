#include "mesh.h"
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <random>
#include <filesystem>
#include <numeric>
#include <algorithm>
Mesh read_obj(const std::string& path) {
    std::ifstream in(path); if(!in) throw std::runtime_error("Cannot read "+path);
    Mesh m; std::string line,tag;
    while(std::getline(in,line)) {
        std::istringstream s(line); if(!(s>>tag)) continue;
        if(tag=="v") { vec3 p; if(!(s>>p.x>>p.y>>p.z) || !std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) throw std::runtime_error("Invalid OBJ vertex"); m.positions.push_back(p); }
        if(tag=="f") {
            std::vector<int> ids; std::string word;
            while(s>>word && word[0]!='#') { int i=std::stoi(word.substr(0,word.find('/'))); i=i<0?(int)m.positions.size()+i:i-1; if(i<0 || i>=(int)m.positions.size()) throw std::runtime_error("Invalid OBJ face index"); ids.push_back(i); }
            if(ids.size()<3) throw std::runtime_error("Invalid OBJ face");
            for(size_t i=1;i+1<ids.size();++i) m.tris.push_back({ids[0],ids[i],ids[i+1]});
        }
    }
    if(m.tris.empty()) throw std::runtime_error("OBJ has no triangles: "+path);
    return m;
}
void write_obj(const Mesh& m,const std::string& path) {
    auto parent=std::filesystem::path(path).parent_path(); if(!parent.empty()) std::filesystem::create_directories(parent);
    std::ofstream out(path); if(!out) throw std::runtime_error("Cannot write "+path);
    out.precision(9); out<<"# Geometry on the GPU\n";
    for(auto p:m.positions) out<<"v "<<p.x<<' '<<p.y<<' '<<p.z<<'\n';
    for(auto t:m.tris) out<<"f "<<t.x+1<<' '<<t.y+1<<' '<<t.z+1<<'\n';
}
MeshStats stats(const Mesh& m) {
    MeshStats s; s.V=(int)m.positions.size(); s.F=(int)m.tris.size();
    std::map<std::pair<int,int>,int> edges;
    std::map<std::pair<int,int>,int> first_face;
    std::vector<int> parent(s.F); std::iota(parent.begin(),parent.end(),0);
    auto root=[&](int f) { while(parent[f]!=f) { parent[f]=parent[parent[f]]; f=parent[f]; } return f; };
    std::vector<std::vector<std::pair<int,int>>> links(s.V);
    std::set<std::vector<int>> faces;
    for(int f=0;f<s.F;++f) {
        auto t=m.tris[f];
        int v[3]={t.x,t.y,t.z}; std::vector<int> key={t.x,t.y,t.z}; std::sort(key.begin(),key.end());
        if(!faces.insert(key).second || length(cross(m.positions[t.y]-m.positions[t.x],m.positions[t.z]-m.positions[t.x]))<1e-14f) s.manifold=false;
        for(int i=0;i<3;++i) {
            int a=v[i],b=v[(i+1)%3]; ++edges[std::minmax(a,b)];
            // Faces connect through shared edges, not merely shared vertices.
            auto inserted=first_face.emplace(std::minmax(a,b),f);
            if(!inserted.second) parent[root(f)]=root(inserted.first->second);
            links[a].push_back({b,v[(i+2)%3]});
        }
    }
    for(auto e:edges) { if(e.second==1) ++s.boundary; if(e.second>2) ++s.nonmanifold; }
    // An edge-only check misses pinched vertices. A closed vertex link is one cycle.
    for(auto& link:links) {
        std::map<int,std::vector<int>> adj;
        for(auto e:link) { adj[e.first].push_back(e.second); adj[e.second].push_back(e.first); }
        if(adj.empty()) { s.manifold=false; continue; }
        if(s.boundary==0) for(auto& e:adj) if(e.second.size()!=2) s.manifold=false;
        std::set<int> seen; std::vector<int> todo={adj.begin()->first};
        while(!todo.empty()) { int v=todo.back(); todo.pop_back(); if(!seen.insert(v).second) continue; for(int u:adj[v]) todo.push_back(u); }
        if(seen.size()!=adj.size()) s.manifold=false;
    }
    s.E=(int)edges.size(); s.chi=s.V-s.E+s.F;
    for(int f=0;f<s.F;++f) if(root(f)==f) ++s.components;
    s.manifold=s.manifold && s.boundary==0 && s.nonmanifold==0;
    if(s.manifold && s.components==1) s.genus=(2-s.chi)/2.0;
    return s;
}
double bbox_diagonal(const Mesh& m) {
    vec3 lo=m.positions.at(0),hi=lo;
    for(auto p:m.positions) { lo={min(lo.x,p.x),min(lo.y,p.y),min(lo.z,p.z)}; hi={max(hi.x,p.x),max(hi.y,p.y),max(hi.z,p.z)}; }
    return length(hi-lo);
}
std::vector<vec3> sample_surface(const Mesh& m,int n,unsigned seed) {
    if(n<=0) throw std::runtime_error("Samples must be positive");
    std::vector<double> cdf; double total=0;
    for(auto t:m.tris) { total+=.5*length(cross(m.positions[t.y]-m.positions[t.x],m.positions[t.z]-m.positions[t.x])); cdf.push_back(total); }
    if(total<=0) throw std::runtime_error("Mesh has zero area");
    std::mt19937 rng(seed);
    // Explicit conversion makes the random stream independent of STL distributions.
    auto uniform=[&] { return (rng()+0.5)/4294967296.0; };
    std::vector<vec3> points; points.reserve(n);
    for(int i=0;i<n;++i) {
        size_t j=std::lower_bound(cdf.begin(),cdf.end(),uniform()*total)-cdf.begin();
        auto t=m.tris[j]; float u=(float)sqrt(uniform()),v=(float)uniform();
        points.push_back(m.positions[t.x]*(1-u)+m.positions[t.y]*(u*(1-v))+m.positions[t.z]*(u*v));
    }
    return points;
}
std::vector<vec3> triangle_vertices(const Mesh& m) {
    std::vector<vec3> v; for(auto t:m.tris) { v.push_back(m.positions[t.x]); v.push_back(m.positions[t.y]); v.push_back(m.positions[t.z]); } return v;
}
