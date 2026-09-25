#pragma once
#include "GridPaintSelection.h"
// Shared row/column locking, contiguous runs and corner junctions for both painters.
struct GridEdgeLine {
    bool active=false;int axis=0,fixed=0,along=0,direction=0;
    float previousX=0,previousY=0;
    void reset(){active=false;}
    void begin(GridPaintTarget edge,float x,float y){
        if(edge.side==1){++edge.x;edge.side=3;}
        else if(edge.side==2){++edge.y;edge.side=0;}
        axis=edge.side==3?1:0;fixed=axis?edge.x:edge.y;along=axis?edge.y:edge.x;
        previousX=x;previousY=y;direction=0;active=true;
    }
    template<class Emit> void sample(float x,float y,Emit emit){
        if(!active)return;
        const float perpendicular=(axis?x:y)-fixed;
        const float deltaAcross=axis?x-previousX:y-previousY;
        const float deltaAlong=axis?y-previousY:x-previousX;
        // A full, deliberate departure from the locked row/column makes a corner.
        // Crossing to the adjacent cell by a few pixels does not change the line.
        const bool turn=std::abs(perpendicular)>=.75f && std::abs(deltaAcross)>std::abs(deltaAlong)*1.25f && deltaAcross*perpendicular>0;
        auto output=[&](int position){emit(axis?GridPaintTarget{fixed,position,3,1,true}:GridPaintTarget{position,fixed,0,1,true});};
        if(turn){
            const int junction=along+(direction==0?((axis?y:x)>=along+.5f?1:0):(direction>0?1:0));
            const int oldFixed=fixed;axis=1-axis;fixed=junction;
            direction=perpendicular>0?1:-1;along=oldFixed+(direction>0?0:-1);output(along);
        }
        const int next=int(std::floor(axis?y:x));
        if(next!=along){const int step=next>along?1:-1;if(direction && step!=direction)output(along);direction=step;
            for(int i=along+step;i!=next+step;i+=step)output(i);
            along=next;
        }
        previousX=x;previousY=y;
    }
};
