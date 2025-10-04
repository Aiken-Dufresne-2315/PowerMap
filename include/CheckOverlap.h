#ifndef _Map_CheckOverlap_H
#define _Map_CheckOverlap_H
#include "BaseEdgeProperty.h"
#include "BaseUGraphProperty.h"

namespace Map {

    bool VVOverlap(const BaseVertexProperty& vertex_1, const BaseVertexProperty& vertex_2);

    bool VEOverlap(const BaseVertexProperty& vertex, const BaseEdgeProperty& edge);

    bool EEOverlap(const BaseEdgeProperty& edge_1, const BaseEdgeProperty& edge_2);

    bool overlapHappens(int vertexID, const Coord2& newPos, const BaseUGraphProperty& graph);
}

#endif // _Map_CheckOverlap_H