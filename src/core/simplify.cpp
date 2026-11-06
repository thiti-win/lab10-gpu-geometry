#include "simplify.h"
#include <queue>
#include <iterator>
#include <algorithm>
#include <set>
#include <array>
#include <unordered_map>
#include <cstdint>
// Vertex valence is small. Contiguous sorted lists avoid per-entry tree allocations.
struct IndexSet : std::vector<int> {
    using std::vector<int>::vector;
    void insert(int value) {
        auto at = std::lower_bound(begin(), end(), value);
        if (at == end() || *at != value) std::vector<int>::insert(at, value);
    }
    template<class Iterator> void insert(Iterator first, Iterator last) {
        for (; first != last; ++first) insert(*first);
    }
    size_t count(int value) const { return std::binary_search(begin(), end(), value); }
    void erase(int value) {
        auto at = std::lower_bound(begin(), end(), value);
        if (at != end() && *at == value) std::vector<int>::erase(at);
    }
};
struct Candidate { double cost; int a,b,va,vb; vec3 p; uint64_t serial; bool operator<(const Candidate& c) const { return cost>c.cost; } };
Mesh simplify_mesh(const Mesh& input,int target,CollapseStats& report, bool unsafe) {
    Mesh m=input; int n=(int)m.positions.size(),remaining=(int)m.tris.size();
    std::vector<Quadric> qs(n); std::vector<IndexSet> faces(n);
    std::vector<int> version(n); std::vector<bool> alive(n,true),face_alive(m.tris.size(),true);
    for(int f=0;f<remaining;++f) {
        auto t=m.tris[f]; Quadric q=plane_quadric(m.positions[t.x],m.positions[t.y],m.positions[t.z]);
        for(int v:{t.x,t.y,t.z}) { qs[v]=qs[v]+q; faces[v].insert(f); }
    }
    auto neighbours=[&](int v) { IndexSet ns; for(int f:faces[v]) { auto t=m.tris[f]; for(int u:{t.x,t.y,t.z}) if(u!=v) ns.insert(u); } return ns; };
    std::priority_queue<Candidate> heap;
    // Only one queued cost per edge and endpoint version pair.
    std::unordered_map<uint64_t, Candidate> pending;
    uint64_t serial = 0;
    auto edge_key = [](int a, int b) { return (uint64_t(a) << 32) | uint32_t(b); };
    auto push=[&](int a,int b) {
        if(a>b) std::swap(a,b); if(a==b || !alive[a] || !alive[b]) return;
        auto key = edge_key(a,b);
        auto found = pending.find(key);
        vec3 p;
        double base;
        if (found != pending.end() && found->second.va == version[a] && found->second.vb == version[b]) {
            p = found->second.p;
            base = evaluate(qs[a]+qs[b],p);
        } else {
            base = collapse_cost(qs[a]+qs[b],m.positions[a],m.positions[b],&p);
        }
        // Re-evaluate the competition hook even when endpoints have not moved.
        double cost = base + extra_cost(m,a,b,p);
        if (found != pending.end() && found->second.va == version[a] && found->second.vb == version[b]
            && found->second.cost == cost) return;
        if (std::isfinite(cost)) {
            Candidate candidate{cost,a,b,version[a],version[b],p,++serial};
            pending[key] = candidate;
            heap.push(candidate);
        } else {
            pending.erase(key);
        }
    };
    std::set<std::pair<int,int>> edges;
    for(auto t:m.tris) for(auto e:{std::pair<int,int>{t.x,t.y},{t.y,t.z},{t.z,t.x}}) edges.insert(std::minmax(e.first,e.second));
    for(auto e:edges) push(e.first,e.second);
    while(remaining>target && !heap.empty()) {
        Candidate c=heap.top(); heap.pop(); int a=c.a,b=c.b;
        if(!alive[a] || !alive[b] || version[a]!=c.va || version[b]!=c.vb) continue;
        auto queued = pending.find(edge_key(a,b));
        if (queued == pending.end() || queued->second.serial != c.serial) continue;
        pending.erase(queued);
        auto na=neighbours(a),nb=neighbours(b); if(!na.count(b)) continue;
        IndexSet opposite,common; int edge_faces=0;
        for(int f:faces[a]) if(faces[b].count(f)) { ++edge_faces; auto t=m.tris[f]; for(int v:{t.x,t.y,t.z}) if(v!=a && v!=b) opposite.insert(v); }
        std::set_intersection(na.begin(),na.end(),nb.begin(),nb.end(),std::back_inserter(common));
        if(!unsafe && (edge_faces!=2 || opposite.size()!=2 || common!=opposite)) { ++report.link; continue; }
        // The tetrahedron case passes the vertex link test but would duplicate faces.
        IndexSet affected=faces[a]; affected.insert(faces[b].begin(),faces[b].end());
        bool safe=true; std::set<std::array<int,3>> local_faces;
        for(int f:affected) {
            auto t=m.tris[f]; int v[3]={t.x,t.y,t.z}; bool has_a=faces[a].count(f),has_b=faces[b].count(f);
            if(has_a && has_b) continue;
            vec3 old[3]={m.positions[v[0]],m.positions[v[1]],m.positions[v[2]]},now[3];
            for(int i=0;i<3;++i) { if(v[i]==b) v[i]=a; now[i]=v[i]==a?c.p:m.positions[v[i]]; }
            std::array<int,3> key={v[0],v[1],v[2]}; std::sort(key.begin(),key.end());
            if(!unsafe && !local_faces.insert(key).second) { ++report.link; safe=false; break; }
            vec3 normal=cross(now[1]-now[0],now[2]-now[0]);
            if(length(normal)<1e-12f) { ++report.degenerate; safe=false; break; }
            if(dot(normal,cross(old[1]-old[0],old[2]-old[0]))<0) { ++report.flip; safe=false; break; }
        }
        if(!safe) continue;
        IndexSet touched=na; touched.insert(nb.begin(),nb.end()); touched.insert(a); touched.insert(b);
        for(int f:affected) {
            auto t=m.tris[f]; for(int v:{t.x,t.y,t.z}) faces[v].erase(f);
            if((t.x==a || t.y==a || t.z==a) && (t.x==b || t.y==b || t.z==b)) { face_alive[f]=false; --remaining; continue; }
            if(t.x==b) t.x=a; if(t.y==b) t.y=a; if(t.z==b) t.z=a; m.tris[f]=t;
            for(int v:{t.x,t.y,t.z}) faces[v].insert(f);
        }
        m.positions[a]=c.p; qs[a]=qs[a]+qs[b]; alive[b]=false; ++report.done;
        // Costs change only at a and b. Retry rejected edges in the changed neighbourhood.
        ++version[a];
        ++version[b];
        // Refresh each affected edge once, rather than once from each end.
        std::vector<std::pair<int,int>> refresh;
        for(int v:touched) if(alive[v]) for(int u:neighbours(v))
            refresh.push_back(std::minmax(v,u));
        std::sort(refresh.begin(),refresh.end());
        refresh.erase(std::unique(refresh.begin(),refresh.end()),refresh.end());
        for(auto edge:refresh) push(edge.first,edge.second);
    }
    Mesh result; std::vector<int> remap(n,-1);
    for(size_t f=0;f<m.tris.size();++f) if(face_alive[f]) {
        auto t=m.tris[f]; int v[3]={t.x,t.y,t.z};
        for(int& u:v) { if(remap[u]<0) { remap[u]=(int)result.positions.size(); result.positions.push_back(m.positions[u]); } u=remap[u]; }
        result.tris.push_back({v[0],v[1],v[2]});
    }
    return result;
}
