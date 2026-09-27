#pragma once
#include <array>
#include <algorithm>
namespace HelmetFitting {
inline constexpr int count=11;
using Point=std::array<float,3>;
using Pulls=std::array<Point,count>;
inline constexpr std::array<Point,count> anchors{{{.5f,1.f,.5f},{.5f,.55f,.98f},{.02f,.45f,.5f},{.98f,.45f,.5f},{.5f,.45f,.02f},{.12f,.02f,.5f},{.88f,.02f,.5f},{.15f,.65f,.82f},{.85f,.65f,.82f},{.15f,.65f,.18f},{.85f,.65f,.18f}}};
inline constexpr const char* labels[]={"Helm oben","Stirnkante","Helm rechts","Helm links","Hinterkopf","Unterkante rechts","Unterkante links","Schlaefe rechts enger","Schlaefe links enger","Hinterkopf rechts enger","Hinterkopf links enger"};
inline Point deform(Point p,const Pulls& pulls){auto result=p;for(int i=0;i<count;++i){float d=0;for(int a=0;a<3;++a){float radius=i>=7?.38f:(a==0?.48f:a==1?.48f:.55f);float x=(p[a]-anchors[i][a])/radius;d+=x*x;}float w=std::max(0.f,1-d);w=w*w;for(int a=0;a<3;++a)result[a]+=w*pulls[i][a];}return result;}
}
