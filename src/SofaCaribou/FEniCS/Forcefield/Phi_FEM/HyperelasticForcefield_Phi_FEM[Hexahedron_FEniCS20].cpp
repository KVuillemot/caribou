#include <SofaCaribou/FEniCS/Forcefield/HyperelasticForcefield_FEniCS[Hexahedron_FEniCS20].h>
#include <SofaCaribou/FEniCS/Forcefield/HyperelasticForcefield_FEniCS.inl>

#include <SofaCaribou/FEniCS/Forcefield/Phi_FEM/HyperelasticForcefield_Phi_FEM.inl>
#include <SofaCaribou/FEniCS/Forcefield/Phi_FEM/HyperelasticForcefield_Phi_FEM[Hexahedron_FEniCS20].h>
DISABLE_ALL_WARNINGS_BEGIN
#include <sofa/core/ObjectFactory.h>
DISABLE_ALL_WARNINGS_END

using sofa::core::RegisterObject;
using namespace caribou::geometry;
using namespace caribou;

namespace SofaCaribou::forcefield {

// --------------------------------
// Hexahedron_FEniCS quadratic specialization
// --------------------------------

// This will force the compiler to compile the following templated class
template class HyperelasticForcefield_Phi_FEM<Hexahedron_FEniCS20>;

} // namespace SofaCaribou::forcefield

namespace sofa::core::objectmodel {
using namespace SofaCaribou::forcefield;

[[maybe_unused]]
static int _c_ = RegisterObject("Caribou hyperelastic force field")
    .add<HyperelasticForcefield_Phi_FEM<Hexahedron_FEniCS20>>();
}
