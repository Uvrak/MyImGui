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
    bool contains(int x,int y) const{
        const auto less=[](GridCellCoord a,GridCellCoord b){return a.y==b.y?a.x<b.x:a.y<b.y;};
        return std::binary_search(m_cells.begin(),m_cells.end(),GridCellCoord{x,y},less);
    }
    void select(GridCellCoord a,GridCellCoord b,int width=0,int height=0){
        clear();const int left=(std::min)(a.x,b.x),right=(std::max)(a.x,b.x);
        const int top=(std::min)(a.y,b.y),bottom=(std::max)(a.y,b.y);
        const int x0=width>0?(std::max)(0,left):left,x1=width>0?(std::min)(width-1,right):right;
        const int y0=height>0?(std::max)(0,top):top,y1=height>0?(std::min)(height-1,bottom):bottom;
        for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x)m_cells.push_back({x,y});
    }
};
