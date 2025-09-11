#include <iostream>
#include "gurobi_c++.h"
#include <boost/config.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/iteration_macros.hpp>
#include "MapFileReader.h"

int main(){
    return Map::runMapFileReader();
}