#include <SofaCaribou/FEniCS/Forcefield/HyperelasticForcefield_FEniCS[Tetrahedron].h>
#include <SofaCaribou/FEniCS/Forcefield/HyperelasticForcefield_FEniCS.inl>

#include <SofaCaribou/FEniCS/Forcefield/Phi_FEM/HyperelasticForcefield_Phi_FEM.inl>
#include <SofaCaribou/FEniCS/Forcefield/Phi_FEM/HyperelasticForcefield_Phi_FEM[Tetrahedron].h>

DISABLE_ALL_WARNINGS_BEGIN
#include <sofa/core/ObjectFactory.h>
DISABLE_ALL_WARNINGS_END

using sofa::core::RegisterObject;
using namespace caribou::geometry;
using namespace caribou;

namespace SofaCaribou::forcefield {

// ---------------------------------
// Tetrahedron linear specialization
// ---------------------------------

// This will force the compiler to compile the following templated class
template class HyperelasticForcefield_Phi_FEM<Tetrahedron>;

} // namespace SofaCaribou::forcefield

namespace sofa::core::objectmodel {
using namespace SofaCaribou::forcefield;

[[maybe_unused]]
static int _c_ = RegisterObject("Caribou hyperelastic force field")
    .add<HyperelasticForcefield_Phi_FEM<Tetrahedron>>();
}
