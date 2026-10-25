#pragma once
#include "gba/core/Model.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <string>
#include <vector>

namespace gba::genreutil {

inline double attr(const Json& obj,const std::string& key,double fallback=0){return obj[key].isNumber()?obj[key].asNumber():fallback;}
inline std::string str(const Json& obj,const std::string& key,const std::string& fallback=""){return obj[key].isString()?obj[key].asString():fallback;}
inline double gene(const Genome& g,const std::string& key,double fallback=1.0){return g.get(key,fallback);}
inline double clamp01(double x){return std::clamp(x,0.0,1.0);}
inline double rand01(std::mt19937_64&r){return std::generate_canonical<double,53>(r);}
inline double noise(std::mt19937_64&r,double sigma){std::normal_distribution<double>d(0,sigma);return d(r);}
inline ParameterSpec spec(std::string k,double d,double lo,double hi,double mut,bool integer=false,std::string cat=""){
    ParameterSpec s; s.key=std::move(k);s.defaultValue=d;s.minValue=lo;s.maxValue=hi;s.mutationScale=mut;s.integer=integer;s.category=std::move(cat);return s;
}
inline std::vector<Json> arrayObjects(const Json& j,const std::string& key){std::vector<Json>v;if(j[key].isArray())for(const auto&x:j[key].asArray())if(x.isObject())v.push_back(x);return v;}
inline double roleMatch(const std::string& role,double armor,double crowd,double boss,double hidden,double fast){
    if(role=="armor")return 1+1.5*armor;
    if(role=="crowd")return 1+1.1*crowd;
    if(role=="boss")return 1+1.4*boss;
    if(role=="detection")return 0.8+1.4*hidden;
    if(role=="control")return 1+0.9*fast;
    if(role=="support")return 0.85+0.25*(armor+crowd+boss+fast);
    return 1.0;
}

} // namespace gba::genreutil
