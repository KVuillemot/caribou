#pragma once
#include <SofaCaribou/Forcefield/HyperelasticForcefield.h>
#include <SofaCaribou/Forcefield/CaribouForcefield[Tetrahedron10].h>
#include <Caribou/Geometry/Tetrahedron10.h>

#include <SofaCaribou/FEniCS/Forcefield/Phi_FEM/HyperelasticForcefield_Phi_FEM.h>

namespace SofaCaribou::forcefield {

// Tetrahedron quadratic specialization
extern template class HyperelasticForcefield<caribou::geometry::Tetrahedron10>;

}
