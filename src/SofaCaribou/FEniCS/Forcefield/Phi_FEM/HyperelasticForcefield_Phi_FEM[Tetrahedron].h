#pragma once
#include <SofaCaribou/Forcefield/HyperelasticForcefield.h>
#include <SofaCaribou/Forcefield/CaribouForcefield[Tetrahedron].h>
#include <Caribou/Geometry/Tetrahedron.h>

#include <SofaCaribou/FEniCS/Forcefield/Phi_FEM/HyperelasticForcefield_Phi_FEM.h>

namespace SofaCaribou::forcefield {

// Tetrahedron linear specialization
extern template class HyperelasticForcefield<caribou::geometry::Tetrahedron>;

}
