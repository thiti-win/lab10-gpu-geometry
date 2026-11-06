#pragma once
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <vector>
struct Cli {
    std::vector<std::string> args;
    Cli(int n,char** v) { for(int i=1;i<n;++i) args.emplace_back(v[i]); }
    bool has(const std::string& flag) const { for(auto& a:args) if(a==flag) return true; return false; }
    std::string get(const std::string& flag,std::string fallback) const {
        for(size_t i=0;i<args.size();++i) if(args[i]==flag) {
            if(i+1==args.size() || args[i+1].rfind("--",0)==0) throw std::runtime_error("Missing value for "+flag);
            return args[i+1];
        }
        return fallback;
    }
    int integer(const std::string& flag,int fallback) const { return std::stoi(get(flag,std::to_string(fallback))); }
};
template<class T> inline void row(const std::string& name,const T& value) { std::cout<<"  "<<std::left<<std::setw(26)<<name<<value<<"\n"; }
inline void panel(const std::string& name) { std::cout<<"\nGeometry on the GPU | "<<name<<"\n"<<std::fixed<<std::setprecision(4); }
inline void positive(int n,const char* name) { if(n<=0) throw std::runtime_error(std::string(name)+" must be positive"); }
