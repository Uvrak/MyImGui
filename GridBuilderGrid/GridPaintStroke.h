#pragma once
#include "GridPaintSelection.h"
#include "GridEdgeLine.h"
#include <set>
#include <tuple>
// Shared press/hold/release lifetime for schematic Pencil and graphical strokes.
struct GridStrokeGesture {
    bool active=false;
    bool update(bool pressed,bool held,bool allowed=true){
        if(!allowed || !held){active=false;return false;}
        const bool started=pressed && !active;
        if(started)active=true;
        return started;
    }
    void reset(){active=false;}
};
class GridPaintStroke {
    std::set<std::tuple<int,int,int>> visited;
    bool sampled=false;float lastX=0,lastY=0;
public:
    GridStrokeGesture gesture;
    GridEdgeLine edgeLine;
    void reset(){gesture.reset();visited.clear();sampled=false;edgeLine.reset();}
    void breakPath(){sampled=false;}
    bool update(bool pressed,bool held,bool allowed){
        const bool started=gesture.update(pressed,held,allowed);
        if(started || !gesture.active){visited.clear();sampled=false;edgeLine.reset();}
        return started;
    }
    bool visit(GridPaintTarget target){
        if(!target.valid)return false;
        // Canonical physical edges also deduplicate East/West and North/South aliases.
        if(target.side==1){++target.x;target.side=3;}
        else if(target.side==2){++target.y;target.side=0;}
        const bool vertical=target.side==3;
        const int span=target.side<0?1:target.span;
        for(int i=0;i<span;++i)if(visited.contains({target.x+(vertical?0:i),target.y+(vertical?i:0),target.side}))return false;
        for(int i=0;i<span;++i)visited.emplace(target.x+(vertical?0:i),target.y+(vertical?i:0),target.side);
        return true;
    }
    template<class Paint> void sample(float x,float y,Paint paint){
        if(!gesture.active)return;
        const float dx=sampled?x-lastX:0,dy=sampled?y-lastY:0;
        const int steps=(std::max)(1,int(std::ceil((std::max)(std::abs(dx),std::abs(dy))*8)));
        for(int i=1;i<=steps;++i)paint(x-dx+dx*i/steps,y-dy+dy*i/steps);
        lastX=x;lastY=y;sampled=true;
    }
};
