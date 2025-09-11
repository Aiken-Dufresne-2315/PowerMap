#ifndef _Map_Types_H
#define _Map_Types_H

#include "BaseVertexProperty.h"
#include "BaseEdgeProperty.h"
#include "BaseGraphProperty.h"
#include "BaseUGraphProperty.h"

namespace Map {
    using Vertex    = BaseVertexProperty;
    using Edge      = BaseEdgeProperty;
    using Graph     = BaseGraphProperty;
    using UGraph    = BaseUGraphProperty;
}

#endif