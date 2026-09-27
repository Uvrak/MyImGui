#pragma once
#include <array>
#include <algorithm>
namespace ArmorFitting {
inline constexpr int count=20;
using Point=std::array<float,3>;
using Pulls=std::array<Point,count>;
inline constexpr std::array<Point,count> anchors{{{.3f,.7f,.95f},{.7f,.7f,.95f},{.3f,.32f,.95f},{.7f,.32f,.95f},{.5f,.03f,.9f},{.3f,.7f,.05f},{.7f,.7f,.05f},{.5f,.25f,.05f},{.5f,.03f,.1f},{.02f,.5f,.5f},{.98f,.5f,.5f},{.2f,.96f,.5f},{.8f,.96f,.5f},{.5f,.92f,.9f},{.03f,.65f,.5f},{.97f,.65f,.5f},{.2f,.87f,.78f},{.8f,.87f,.78f},{.2f,.87f,.22f},{.8f,.87f,.22f}}};
inline constexpr const char* labels[]={"Brust rechts","Brust links","Bauch rechts","Bauch links","Saum vorne","Ruecken rechts","Ruecken links","Ruecken unten","Saum hinten","Seite rechts","Seite links","Schulter rechts","Schulter links","Halsausschnitt vorne","Armausschnitt rechts","Armausschnitt links","Schulter vorne rechts","Schulter vorne links","Schulter hinten rechts","Schulter hinten links"};
inline Point deform(Point p,const Pulls& pulls){auto result=p;for(int i=0;i<count;++i){float d=0;for(int a=0;a<3;++a){float radius=i>=16?(a==0?.26f:a==1?.28f:.32f):i>=14?(a==0?.28f:a==1?.25f:.55f):(a==0?.40f:a==1?.40f:.48f);float x=(p[a]-anchors[i][a])/radius;d+=x*x;}float w=std::max(0.f,1-d);w=w*w*w;for(int a=0;a<3;++a)result[a]+=w*pulls[i][a];}return result;}
}
