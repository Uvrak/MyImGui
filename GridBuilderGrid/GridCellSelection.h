#pragma once
#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <vector>
enum class GridSelectionGeometry { Rectangle };
enum class GridBorderShape { Rectangular, Diagonal };
struct GridCellCoord { int x=0,y=0;bool operator==(const GridCellCoord&) const = default; };
struct SelectionActionContext { std::vector<GridCellCoord> cells; GridBorderShape borderShape=GridBorderShape::Rectangular; };
using GridSelectionAction=std::function<void(const SelectionActionContext&)>;
// Inclusive rectangular working area, independent of the requested border style.
class GridCellSelection {
    GridSelectionGeometry m_shape=GridSelectionGeometry::Rectangle;
    std::vector<GridCellCoord> m_cells;
public:
    GridSelectionGeometry geometry() const{return m_shape;}
    void setGeometry(GridSelectionGeometry shape){m_shape=shape;}
    void clear(){m_cells.clear();}
    const std::vector<GridCellCoord>& cells() const{return m_cells;}
    bool bounds(int& left,int& top,int& right,int& bottom) const{
        if(m_cells.empty())return false;
        left=right=m_cells.front().x;top=bottom=m_cells.front().y;
        for(const auto cell:m_cells){left=(std::min)(left,cell.x);right=(std::max)(right,cell.x);top=(std::min)(top,cell.y);bottom=(std::max)(bottom,cell.y);}
        return true;
    }
    bool contains(int x,int y) const{
        const auto less=[](GridCellCoord a,GridCellCoord b){return a.y==b.y?a.x<b.x:a.y<b.y;};
        return std::binary_search(m_cells.begin(),m_cells.end(),GridCellCoord{x,y},less);
    }
    void toggle(GridCellCoord cell,int width=0,int height=0){
        if((width>0 && (cell.x<0 || cell.x>=width)) || (height>0 && (cell.y<0 || cell.y>=height)))return;
        const auto less=[](GridCellCoord a,GridCellCoord b){return a.y==b.y?a.x<b.x:a.y<b.y;};
        const auto position=std::lower_bound(m_cells.begin(),m_cells.end(),cell,less);
        if(position!=m_cells.end() && *position==cell)m_cells.erase(position);
        else m_cells.insert(position,cell);
    }
    void resizePreservingShape(const std::vector<GridCellCoord>& original,
        int oldLeft,int oldTop,int oldRight,int oldBottom,
        int newLeft,int newTop,int newRight,int newBottom){
        m_cells.clear();
        const auto less=[](GridCellCoord a,GridCellCoord b){return a.y==b.y?a.x<b.x:a.y<b.y;};
        const int oldWidth=oldRight-oldLeft+1,oldHeight=oldBottom-oldTop+1;
        const int newWidth=newRight-newLeft+1,newHeight=newBottom-newTop+1;
        auto sourceCoordinate=[](int value,int newStart,int newSize,int oldStart,int oldSize){
            if(newSize<=1 || oldSize<=1)return oldStart;
            return oldStart+int(std::lround(double(value-newStart)*double(oldSize-1)/double(newSize-1)));
        };
        for(int y=newTop;y<=newBottom;++y)for(int x=newLeft;x<=newRight;++x){
            const GridCellCoord source{sourceCoordinate(x,newLeft,newWidth,oldLeft,oldWidth),sourceCoordinate(y,newTop,newHeight,oldTop,oldHeight)};
            if(std::binary_search(original.begin(),original.end(),source,less))m_cells.push_back({x,y});
        }
    }
    void select(GridCellCoord a,GridCellCoord b,int width=0,int height=0){
        clear();const int left=(std::min)(a.x,b.x),right=(std::max)(a.x,b.x);
        const int top=(std::min)(a.y,b.y),bottom=(std::max)(a.y,b.y);
        const int x0=width>0?(std::max)(0,left):left,x1=width>0?(std::min)(width-1,right):right;
        const int y0=height>0?(std::max)(0,top):top,y1=height>0?(std::min)(height-1,bottom):bottom;
        for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x)m_cells.push_back({x,y});
    }
};
