#include "CheckOverlap.h"
#include "BaseEdgeProperty.h"
#include "BaseVertexProperty.h"
#include "BaseUGraphProperty.h"
#include <cmath>

namespace Map {
    const double EPSILON = 1e-9;
    
    bool isCollinear(double x, double y, double x_A, double y_A, double x_B, double y_B){
        double cross_product = (y - y_A) * (x_B - x_A) - (x - x_A) * (y_B - y_A);
        return std::abs(cross_product) < EPSILON;
    }   

    bool VVOverlap(const BaseVertexProperty& vertex_1, const BaseVertexProperty& vertex_2) {
        return (std::abs(vertex_1.getCoord().x() - vertex_2.getCoord().x()) < EPSILON) &&
               (std::abs(vertex_1.getCoord().y() - vertex_2.getCoord().y()) < EPSILON);
    }

    bool VEOverlap(const BaseVertexProperty& vertex, const BaseEdgeProperty& edge) {
        double x_A = edge.Source().getCoord().x();
        double y_A = edge.Source().getCoord().y();
        double x_B = edge.Target().getCoord().x();
        double y_B = edge.Target().getCoord().y();

        double x_P = vertex.getCoord().x();
        double y_P = vertex.getCoord().y();

        if(isCollinear(x_P, y_P, x_A, y_A, x_B, y_B)) {
            if(std::abs(x_A - x_B) < EPSILON) { // vertical line
                return (y_P >= std::min(y_A, y_B) - EPSILON) && (y_P <= std::max(y_A, y_B) + EPSILON);
            }     
            else {
                return (x_P >= std::min(x_A, x_B) - EPSILON) && (x_P <= std::max(x_A, x_B) + EPSILON);
            }
        }
        else{
            return false;
        }
    }

    // check if two edges overlap
    bool EEOverlap(const BaseEdgeProperty& edge_1, const BaseEdgeProperty& edge_2) {
        double x_A = edge_1.Source().getCoord().x();
        double y_A = edge_1.Source().getCoord().y();
        double x_B = edge_1.Target().getCoord().x();
        double y_B = edge_1.Target().getCoord().y();

        double x_C = edge_2.Source().getCoord().x();
        double y_C = edge_2.Source().getCoord().y();
        double x_D = edge_2.Target().getCoord().x();
        double y_D = edge_2.Target().getCoord().y();

        if(isCollinear(x_C, y_C, x_A, y_A, x_B, y_B) && isCollinear(x_D, y_D, x_A, y_A, x_B, y_B)) {
            if(std::abs(x_C - x_A) < EPSILON && std::abs(x_A - x_B) < EPSILON) {
                double a = std::min(y_A, y_B);
                double b = std::max(y_A, y_B);
                double c = std::min(y_C, y_D);
                double d = std::max(y_C, y_D);    
                return !(c >= b + EPSILON || d <= a - EPSILON);
            }     
            else {
                double a = std::min(x_A, x_B);
                double b = std::max(x_A, x_B);
                double c = std::min(x_C, x_D);
                double d = std::max(x_C, x_D);    
                return !(c >= b + EPSILON || d <= a - EPSILON);
            }
        }
        else{
            return false;
        }
    }

    // check 
    bool globalVVOverlap(const BaseVertexProperty& v, const BaseUGraphProperty& graph) {
        std::pair<BaseUGraphProperty::vertex_iterator, BaseUGraphProperty::vertex_iterator> vp = boost::vertices(graph);
        for (BaseUGraphProperty::vertex_iterator vit = vp.first; vit != vp.second; ++vit) {
            BaseVertexProperty tempV = graph[*vit];
            if (v.getID() == tempV.getID()) {
                continue;
            }
            else {
                if (VVOverlap(v, tempV)) {
                    return true;
                }
            }
        }
        return false;
    }

    bool globalVEOverlap(const BaseVertexProperty& v, const BaseUGraphProperty& graph) {
        std::pair<BaseUGraphProperty::edge_iterator, BaseUGraphProperty::edge_iterator> out_ep = boost::out_edges(, graph);
        std::pair<BaseUGraphProperty::edge_iterator, BaseUGraphProperty::edge_iterator> ep = boost::edges(graph);
        for (BaseUGraphProperty::edge_iterator eit = ep.first; eit != ep.second; ++eit) {
            BaseEdgeProperty tempE = graph[*eit];
            if (e.ID() == tempE.ID()) {
                continue;
            }
            else {
                if( )
            }
        }
    }
}