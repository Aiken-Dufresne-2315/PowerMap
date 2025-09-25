#include "BaseUGraphProperty.h"

namespace Map {

    //------------------------------------------------------------------------------
    // Customized Vertex Functions
    //------------------------------------------------------------------------------

    //------------------------------------------------------------------------------
    // Customized Edge Functions
    //------------------------------------------------------------------------------

    //------------------------------------------------------------------------------
    // Customized Layout Functions
    //------------------------------------------------------------------------------

    //------------------------------------------------------------------------------
    // Special functions
    //------------------------------------------------------------------------------
    void printGraph( const BaseUGraphProperty & graph ) {
        std::cerr << "num_vertices = " << num_vertices( graph ) << std::endl;
        std::cerr << "num_edges = " << num_edges( graph ) << std::endl;

        // print vertex information
        BGL_FORALL_VERTICES( vd, graph, BaseUGraphProperty ) {
            std::cerr << " id = " << graph[vd].getID() << " coord = " << graph[vd].getCoord();
        }
    }

    void clearGraph( BaseUGraphProperty & graph ) {
        // clear edges
        while (num_edges(graph) > 0) {
            BaseUGraphProperty::edge_iterator ei = edges(graph).first;
            remove_edge(*ei, graph);
        }

        // clear vertices
        while (num_vertices(graph) > 0) {
            BaseUGraphProperty::vertex_iterator vi = vertices(graph).first;
            clear_vertex(*vi, graph);
            remove_vertex(*vi, graph);
        }
    }

} // namespace Map