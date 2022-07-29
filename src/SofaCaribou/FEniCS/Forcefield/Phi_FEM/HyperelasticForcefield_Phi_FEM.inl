#pragma once

#include <SofaCaribou/config.h>
#include <SofaCaribou/FEniCS/Forcefield/HyperelasticForcefield_FEniCS.h>

#include <SofaCaribou/FEniCS/Forcefield/Phi_FEM/HyperelasticForcefield_Phi_FEM.h>

#include <SofaCaribou/Forcefield/CaribouForcefield.inl>
#include <SofaCaribou/Topology/CaribouTopology.h>

#include <SofaCaribou/Topology/FictitiousGrid.h>
#include <SofaCaribou/Topology/FictitiousGrid.inl>


DISABLE_ALL_WARNINGS_BEGIN
#include <sofa/helper/AdvancedTimer.h>
#include <sofa/core/MechanicalParams.h>
DISABLE_ALL_WARNINGS_END

#include <Caribou/Mechanics/Elasticity/Strain.h>
#ifdef CARIBOU_WITH_OPENMP
#include <omp.h>

#include <iostream>

#endif

namespace SofaCaribou::forcefield {

template <typename Element>
HyperelasticForcefield_Phi_FEM<Element>::HyperelasticForcefield_Phi_FEM()
: d_material(initLink(
    "material",
    "Material used to compute the hyperelastic force field."))
, d_enable_multithreading(initData(&d_enable_multithreading,
    false,
    "enable_multithreading",
    "Enable the multithreading computation of the stiffness matrix. Only use this if you have a "
    "very large number of elements, otherwise performance might be worse than single threading."
    "When enabled, use the environment variable OMP_NUM_THREADS=N to use N threads."))
{
}

template <typename Element>
void HyperelasticForcefield_Phi_FEM<Element>::init()
{
    using sofa::core::topology::BaseMeshTopology;
    using sofa::core::objectmodel::BaseContext;
    Inherit::init();

    // auto grid = this->getContext()->getObjects<FictitiousGrid<DataTypes>(BaseContext::Local);
    // No material set, try to find one in the current context
    if (not d_material.get()) {
        auto materials = this->getContext()->template getObjects<material::FEniCS_Material<Element, DataTypes>>(BaseContext::Local);
        
        if (materials.empty()) {
            msg_warning() << "Could not find an hyperelastic material in the current context.";
        } else if (materials.size() > 1) {
            msg_warning() << "Multiple materials were found in the context node. "   <<
                             "Please specify which one should be use by explicitly " <<
                             "setting the material's path in the '" << d_material.getName() << "' parameter.";
        } else {
            d_material.set(materials[0]);
            msg_info() << "Automatically found the material '" << d_material.get()->getPathName() << "'.";
        }
    }
    if(!d_material->MaterialIsAvailable()) {
        msg_error() << "No material named " << d_material->getMaterialName() << " available";
        return;
    }

    // Assemble the initial stiffness matrix
    assemble_stiffness();
}

template<typename Element>
void HyperelasticForcefield_Phi_FEM<Element>::addForce(const sofa::core::MechanicalParams *mparams, sofa::core::MultiVecDerivId fId) {
    if (mparams) {
        // Stores the identifier of the x position vector for later use in the stiffness matrix assembly.
        p_X_id = mparams->x();
    }

    Inherit::addForce(mparams, fId);
}

template <typename Element>
void HyperelasticForcefield_Phi_FEM<Element>::addForce(
    const sofa::core::MechanicalParams* mparams,
    sofa::core::objectmodel::Data<VecDeriv>& d_f,
    const sofa::core::objectmodel::Data<VecCoord>& d_x,
    const sofa::core::objectmodel::Data<VecDeriv>& d_v)
{
    using namespace sofa::core::objectmodel;
    using namespace sofa::helper;
    using namespace SofaCaribou::topology;

    SOFA_UNUSED(mparams);
    SOFA_UNUSED(d_v);

    if (!this->mstate)
        return;

    const auto grid = d_grid.get();

    const auto material = d_material.get();
    if (!material) {
        return;
    }

    if(!d_material->MaterialIsAvailable()) {
        msg_error() << "No material named " << d_material->getMaterialName() << " available";
        return;
    }

    //material->before_update();

    ReadAccessor<Data<VecCoord>> sofa_x = d_x;
    ReadAccessor<Data<VecCoord>> sofa_x0 = this->mstate->readRestPositions();
    WriteAccessor<Data<VecDeriv>> sofa_f = d_f;

    if (sofa_x.size() != sofa_f.size())
        return;
    const auto nb_nodes = sofa_x.size();
    const auto nb_elements = this->number_of_elements();

    if (nb_nodes == 0 || nb_elements == 0)
        return;

    Eigen::Map<const Eigen::Matrix<Real, Eigen::Dynamic, Dimension, Eigen::RowMajor>>    X       (sofa_x.ref().data()->data(),  nb_nodes, Dimension);
    Eigen::Map<const Eigen::Matrix<Real, Eigen::Dynamic, Dimension, Eigen::RowMajor>>    X0      (sofa_x0.ref().data()->data(), nb_nodes, Dimension);

    // Compute the displacement with respect to the rest position
    const auto u =  X - X0;

    // Get FEniCS F 
    const ufcx_integral *integral = material->FEniCS_F();


    // Get constants from the material
    const double constants_mooney[3] = {   
                                    material->getMooneyRivlinConstants()(0, 0), // C01
                                    material->getMooneyRivlinConstants()(0, 1),  // C10
                                    material->getMooneyRivlinConstants()(0, 2)   // K
                                };

    const double constants_ogden[9] = {
                                    material->getOgdenConstants()(0, 0), // bulk modulus
                                    material->getOgdenConstants()(0, 1), // a
                                    material->getOgdenConstants()(0, 2), // b
                                    material->getOgdenConstants()(0, 3), // a_f
                                    material->getOgdenConstants()(0, 4), // b_f
                                    material->getOgdenConstants()(0, 5), // a_s
                                    material->getOgdenConstants()(0, 6), // b_s
                                    material->getOgdenConstants()(0, 7), // a_fs
                                    material->getOgdenConstants()(0, 8), // b_fs
                                };

    const double constants_else[2] = {   
                                    material->getYoungModulusAndPoissonRatio()(0, 0), // Young Modulus
                                    material->getYoungModulusAndPoissonRatio()(0, 1)  // Poisson ratio
                                };
    const double* constants;
    if(material->getMaterialName() == "MooneyRivlin") {
        constants = constants_mooney;
    } else if(material->getMaterialName() == "Ogden") {
        constants = constants_ogden;
    } else {
        constants = constants_else;
    }
    
    sofa::helper::AdvancedTimer::stepBegin("HyperelasticForcefield_Phi_FEM::addForce");


    const auto cells = grid -> boundary_cells_indices();
    // const auto boundary = std::get<0>(cells);
    // const auto inside = std::get<1>(cells);
    // const auto outside = std::get<2>(cells);

    // for(UNSIGNED_INTEGER_TYPE index = 0; index < inside.size(); ++index) {
    // //for (std::size_t element_id = 0; element_id < nb_elements; ++element_id) {


    //     // Fetch the node indices of the element
    //     auto node_indices_ = this->topology()->domain()->element_indices(index);

                
    //     // Fetch the initial and current positions of the element's nodes
    //     Matrix<NumberOfNodesPerElement, Dimension> current_nodes_position;
    //     Matrix<NumberOfNodesPerElement, Dimension> coefficients;

    //     auto cell_index = inside[index];
    //     auto faces = grid -> get_faces_on_cell(cell_index); 

    //     /** Here we should add the computation of phi-fem on each cell 
    //     * For that, we need to check the type of each cell : if inside, outside or boundary 
    //     * And then apply phi-FEM depending on that 
    //     * If inside : apply phi-FEM with dx and no stab terms 
    //     * If boundary, integral on the elemnt and stab terms :
    //     * no problem for order 2 term (i think) because just a dx integral on the cell
    //     * but there will be a problem for the first order term : 
    //     * for the jump we need the value on the two cells and that is gonna be hard to compute i think
    //     */
    // }

    for (std::size_t element_id = 0; element_id < nb_elements; ++element_id) {


        // Fetch the node indices of the element
        auto node_indices = this->topology()->domain()->element_indices(element_id);

                
        // Fetch the initial and current positions of the element's nodes
        Matrix<NumberOfNodesPerElement, Dimension> current_nodes_position;
        Matrix<NumberOfNodesPerElement, Dimension> coefficients;

        for (std::size_t i = 0; i < NumberOfNodesPerElement; ++i) {
            current_nodes_position.row(i).noalias() = X0.row(node_indices[i]);
            coefficients.row(i).noalias() = u.row(node_indices[i]);

        }

        // Compute the nodal forces
        Matrix<NumberOfNodesPerElement, Dimension> nodal_forces;
        nodal_forces.fill(0);

        integral->tabulate_tensor_float64(nodal_forces.data(), coefficients.data(), constants, current_nodes_position.data(), nullptr, nullptr);

        for (size_t i = 0; i < NumberOfNodesPerElement; ++i) {
            for (size_t j = 0; j < Dimension; ++j) {
                sofa_f[node_indices[i]][j] -= nodal_forces(i,j);
            }
        }
    }

    sofa::helper::AdvancedTimer::stepEnd("HyperelasticForcefield_Phi_FEM::addForce");

    // This is the only I found to detect when a stiffness matrix reassembly is needed for calls to addDForce
    K_is_up_to_date = false;
    eigenvalues_are_up_to_date = false;

}


template <typename Element>
void HyperelasticForcefield_Phi_FEM<Element>::addDForce(
    const sofa::core::MechanicalParams* mparams,
    sofa::core::objectmodel::Data<VecDeriv>& d_df,
    const sofa::core::objectmodel::Data<VecDeriv>& d_dx)
{
    using namespace sofa::core::objectmodel;

    if (not K_is_up_to_date) {
        assemble_stiffness();
    }

    auto kFactor = static_cast<Real> (mparams->kFactorIncludingRayleighDamping(this->rayleighStiffness.getValue()));
    sofa::helper::ReadAccessor<Data<VecDeriv>> sofa_dx = d_dx;
    sofa::helper::WriteAccessor<Data<VecDeriv>> sofa_df = d_df;

    Eigen::Map<const Eigen::Matrix<Real, Eigen::Dynamic, 1>> DX   (&(sofa_dx[0][0]), sofa_dx.size()*3);
    Eigen::Map<Eigen::Matrix<Real, Eigen::Dynamic, 1>>       DF   (&(sofa_df[0][0]), sofa_df.size()*3);

    sofa::helper::AdvancedTimer::stepBegin("HyperelasticForcefield_Phi_FEM::addDForce");

    for (int k = 0; k < p_K.outerSize(); ++k) {
        for (typename Eigen::SparseMatrix<Real>::InnerIterator it(p_K, k); it; ++it) {
            const auto i = it.row();
            const auto j = it.col();
            const auto v = -1 * it.value() * kFactor;
            if (i != j) {
                DF[i] += v*DX[j];
                DF[j] += v*DX[i];
            } else {
                DF[i] += v*DX[i];
            }
        }
    }

    sofa::helper::AdvancedTimer::stepEnd("HyperelasticForcefield_Phi_FEM::addDForce");
}

template <typename Element>
void HyperelasticForcefield_Phi_FEM<Element>::addKToMatrix(
    sofa::defaulttype::BaseMatrix * matrix,
    SReal kFact, unsigned int & offset)
{
    if (not K_is_up_to_date) {
        assemble_stiffness();
    }

    sofa::helper::AdvancedTimer::stepBegin("HyperelasticForcefield_Phi_FEM::addKToMatrix");

    // K is symmetric, so we only stored "one side" of the matrix.
    // But to accelerate the computation, coefficients were not
    // stored only in the upper or lower triangular part, but instead
    // in whatever triangular part (upper or lower) the first node
    // index of the element was. This means that a coefficient (i,j)
    // might be in the lower triangular part, while (k,l) is in the
    // upper triangular part. But no coefficient will be both in the
    // lower AND the upper part.

    for (int k = 0; k < p_K.outerSize(); ++k) {
        for (typename Eigen::SparseMatrix<Real>::InnerIterator it(p_K, k); it; ++it) {
            const auto i = it.row();
            const auto j = it.col();
            const auto v = -1 * it.value() * kFact;
            if (i != j) {
                matrix->add(offset+i, offset+j, v);
                matrix->add(offset+j, offset+i, v);
            } else {
                matrix->add(offset+i, offset+i, v);
            }
        }
    }

    sofa::helper::AdvancedTimer::stepEnd("HyperelasticForcefield_Phi_FEM::addKToMatrix");
}

template <typename Element>
SReal HyperelasticForcefield_Phi_FEM<Element>::getPotentialEnergy (
    const sofa::core::MechanicalParams* mparams,
    const sofa::core::objectmodel::Data<VecCoord>& d_x) const {
    using namespace sofa::core::objectmodel;

    SOFA_UNUSED(mparams);

    if (!this->mstate)
        return 0.;

    const auto material = d_material.get();
    if (!material) {
        return 0;
    }

    if(!d_material->MaterialIsAvailable()) {
        msg_error()<< "No material named " << d_material->getMaterialName() << " available";
        return 0;
    }

    sofa::helper::ReadAccessor<Data<VecCoord>> sofa_x = d_x;
    sofa::helper::ReadAccessor<Data<VecCoord>> sofa_x0 = this->mstate->readRestPositions();

    if (sofa_x.size() != sofa_x0.size() )
        return 0.;

    const auto nb_nodes = sofa_x.size();
    const auto nb_elements = this->number_of_elements();

    if (nb_nodes == 0 || nb_elements == 0)
        return 0;
    
    if (Psi_is_up_to_date) {
        return Psi;
    }

    const Eigen::Map<const Eigen::Matrix<Real, Eigen::Dynamic, Dimension, Eigen::RowMajor>>    X       (sofa_x.ref().data()->data(),  nb_nodes, Dimension);
    const Eigen::Map<const Eigen::Matrix<Real, Eigen::Dynamic, Dimension, Eigen::RowMajor>>    X0      (sofa_x0.ref().data()->data(), nb_nodes, Dimension);

     // Compute the displacement with respect to the rest position
    const auto u =  X - X0;

    // Get FEniCS F 
    const ufcx_integral *integral = material->FEniCS_Pi();

    const double constants_mooney[3] = {   
                                    material->getMooneyRivlinConstants()(0, 0), // Young Modulus
                                    material->getMooneyRivlinConstants()(0, 1),  // Poisson ratiO
                                    material->getMooneyRivlinConstants()(0, 2) 
                                };

    const double constants_ogden[9] = {
                                    material->getOgdenConstants()(0, 0), // bulk modulus
                                    material->getOgdenConstants()(0, 1), // a
                                    material->getOgdenConstants()(0, 2), // b
                                    material->getOgdenConstants()(0, 3), // a_f
                                    material->getOgdenConstants()(0, 4), // b_f
                                    material->getOgdenConstants()(0, 5), // a_s
                                    material->getOgdenConstants()(0, 6), // b_s
                                    material->getOgdenConstants()(0, 7), // a_fs
                                    material->getOgdenConstants()(0, 8), // b_fs
                                };

    const double constants_else[2] = {
                                    material->getYoungModulusAndPoissonRatio()(0, 0), // Young Modulus
                                    material->getYoungModulusAndPoissonRatio()(0, 1)  // Poisson ratio
                                };
    const double* constants;
    if(material->getMaterialName() == "MooneyRivlin") {
        constants = constants_mooney;
    } else if(material->getMaterialName() == "Ogden") {
        constants = constants_ogden;
    } else {
        constants = constants_else;
    }

    SReal Psi = 0.;

    sofa::helper::AdvancedTimer::stepBegin("HyperelasticForcefield_Phi_FEM::getPotentialEnergy");

    for (std::size_t element_id = 0; element_id < nb_elements; ++element_id) {

        // Fetch the node indices of the element
        auto node_indices = this->topology()->domain()->element_indices(element_id);

        // Fetch the initial and current positions of the element's nodes
        Matrix<NumberOfNodesPerElement, Dimension> current_nodes_position;
        Matrix<NumberOfNodesPerElement, Dimension> coefficients;


        for (std::size_t i = 0; i < NumberOfNodesPerElement; ++i) {
            current_nodes_position.row(i).noalias() = X0.row(node_indices[i]);
            coefficients.row(i).noalias() = u.row(node_indices[i]);

        }

        // Compute the element energy
        Matrix<1, 1> element_energy;
        element_energy.fill(0);

        integral->tabulate_tensor_float64(element_energy.data(), coefficients.data(), constants, current_nodes_position.data(), nullptr, nullptr);
        Psi += element_energy[0];
    }
    sofa::helper::AdvancedTimer::stepEnd("HyperelasticForcefield_Phi_FEM::getPotentialEnergy");
    Psi_is_up_to_date = true;
    return Psi;
}


template <typename Element>
void HyperelasticForcefield_Phi_FEM<Element>::assemble_stiffness()
{
    assemble_stiffness(*this->mstate->read (p_X_id.getId(this->mstate)));
}

template<typename Element>
void HyperelasticForcefield_Phi_FEM<Element>::assemble_stiffness(const sofa::core::objectmodel::Data<VecCoord> & x) {
    using namespace sofa::core::objectmodel;

    const sofa::helper::ReadAccessor<Data<VecCoord>> sofa_x= x;
    const sofa::helper::ReadAccessor<Data<VecCoord>> sofa_x0 = this->mstate->readRestPositions();

    const auto nb_nodes = sofa_x.size();
    Eigen::Map<const Eigen::Matrix<Real, Eigen::Dynamic, Dimension, Eigen::RowMajor>>    X       (sofa_x.ref().data()->data(),  nb_nodes, Dimension);
    Eigen::Map<const Eigen::Matrix<Real, Eigen::Dynamic, Dimension, Eigen::RowMajor>>    X0      (sofa_x0.ref().data()->data(), nb_nodes, Dimension);


    assemble_stiffness(X, X0);
}

template<typename Element>
template<typename Derived>
void HyperelasticForcefield_Phi_FEM<Element>::assemble_stiffness(const Eigen::MatrixBase<Derived> & x, const Eigen::MatrixBase<Derived> & x0) {
    const auto material = d_material.get();

    [[maybe_unused]]
    const auto enable_multithreading = d_enable_multithreading.getValue();
    if (!material) {
        return;
    }

    if(!d_material->MaterialIsAvailable()) {
        msg_error() << "No material named " << d_material->getMaterialName() << " available";
        return;
    }

    const auto nb_elements = this->number_of_elements();
    const auto nb_nodes = x.rows();
    const auto nDofs = nb_nodes*Dimension;
    p_K.resize(nDofs, nDofs);

    ///< Triplets are used to store matrix entries before the call to 'compress'.
    /// Duplicates entries are summed up.
    std::vector<Eigen::Triplet<Real>> triplets;
    triplets.reserve(nDofs*24*2);

    sofa::helper::AdvancedTimer::stepBegin("HyperelasticForcefield_Phi_FEM::update_stiffness");

    const double constants_mooney[3] = {   
                                    material->getMooneyRivlinConstants()(0, 0), // Young Modulus
                                    material->getMooneyRivlinConstants()(0, 1),  // Poisson ratiO
                                    material->getMooneyRivlinConstants()(0, 2) 
                                };

    const double constants_ogden[9] = {
                                    material->getOgdenConstants()(0, 0), // bulk modulus
                                    material->getOgdenConstants()(0, 1), // a
                                    material->getOgdenConstants()(0, 2), // b
                                    material->getOgdenConstants()(0, 3), // a_f
                                    material->getOgdenConstants()(0, 4), // b_f
                                    material->getOgdenConstants()(0, 5), // a_s
                                    material->getOgdenConstants()(0, 6), // b_s
                                    material->getOgdenConstants()(0, 7), // a_fs
                                    material->getOgdenConstants()(0, 8), // b_fs
                                };

    const double constants_else[2] = {
                                    material->getYoungModulusAndPoissonRatio()(0, 0), // Young Modulus
                                    material->getYoungModulusAndPoissonRatio()(0, 1)  // Poisson ratio
                                };
    const double* constants;
    if(material->getMaterialName() == "MooneyRivlin") {
        constants = constants_mooney;
    } else if(material->getMaterialName() == "Ogden") {
        constants = constants_ogden;
    } else {
        constants = constants_else;
    }
    const auto u =  x - x0;

    const ufcx_integral *integral = material->FEniCS_J();

#pragma omp parallel for if (enable_multithreading)
    for (int element_id = 0; element_id < static_cast<int>(nb_elements); ++element_id) {
        // Fetch the node indices of the element
        auto node_indices = this->topology()->domain()->element_indices(element_id);

        // Fetch the current positions of the element's nodes
        Matrix<NumberOfNodesPerElement, Dimension> current_nodes_position;
        Matrix<NumberOfNodesPerElement, Dimension> coefficients;


        for (std::size_t i = 0; i < NumberOfNodesPerElement; ++i) {
            current_nodes_position.row(i).noalias() = x0.row(node_indices[i]).template cast<Real>();
            coefficients.row(i).noalias() = u.row(node_indices[i]).template cast<Real>();

        }
        
        Matrix<NumberOfNodesPerElement, Dimension> nodal_forces;
        nodal_forces.fill(0);
        using Stiffness = Eigen::Matrix<FLOATING_POINT_TYPE, NumberOfNodesPerElement*Dimension, NumberOfNodesPerElement*Dimension, Eigen::RowMajor>;
        Stiffness Ke = Stiffness::Zero();
        integral->tabulate_tensor_float64(Ke.data(), coefficients.data(), constants, current_nodes_position.data(), nullptr, nullptr);


#pragma omp critical
        for (std::size_t i = 0; i < NumberOfNodesPerElement; ++i) {
            // Node index of the ith node in the global stiffness matrix
            const auto x = static_cast<int>(node_indices[i]*Dimension);
            for (int m = 0; m < Dimension; ++m) {
                for (int n = m; n < Dimension; ++n) {
                    triplets.emplace_back(x+m, x+n, Ke(i*Dimension+m,i*Dimension+n));
                }
            }

            for (std::size_t j = i+1; j < NumberOfNodesPerElement; ++j) {
                // Node index of the jth node in the global stiffness matrix
                const auto y = static_cast<int>(node_indices[j]*Dimension);
                for (int m = 0; m < Dimension; ++m) {
                    for (int n = 0; n < Dimension; ++n) {
                        triplets.emplace_back(x+m, y+n, Ke(i*Dimension+m,j*Dimension+n));
                    }
                }
            }
        }
    }
    p_K.setFromTriplets(triplets.begin(), triplets.end());
    sofa::helper::AdvancedTimer::stepEnd("HyperelasticForcefield_Phi_FEM::update_stiffness");

    K_is_up_to_date = true;
    eigenvalues_are_up_to_date = false;
}



template <typename Element>
auto HyperelasticForcefield_Phi_FEM<Element>::eigenvalues() -> const Vector<Eigen::Dynamic> & {
    if (not eigenvalues_are_up_to_date) {
#ifdef EIGEN_USE_LAPACKE
        Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic> k (K());
        Eigen::SelfAdjointEigenSolver<Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic>> eigensolver(k, Eigen::EigenvaluesOnly);
#else
        Eigen::SelfAdjointEigenSolver<Eigen::SparseMatrix<Real>> eigensolver(K(), Eigen::EigenvaluesOnly);
#endif
        if (eigensolver.info() != Eigen::Success) {
            msg_error() << "Unable to find the eigen values of K.";
        }

        p_eigenvalues = eigensolver.eigenvalues();
        eigenvalues_are_up_to_date = true;
    }

    return p_eigenvalues;
}

template <typename Element>
auto HyperelasticForcefield_Phi_FEM<Element>::cond() -> Real {
    const auto & values = eigenvalues();
    const auto min = values.minCoeff();
    const auto max = values.maxCoeff();

    return min/max;
}


template <typename Element>
auto HyperelasticForcefield_Phi_FEM<Element>::Pi() -> SReal {
    if(Psi_is_up_to_date) {
        return Psi;
    }
    msg_warning() << "Potential Energy haven't been updated, no call for getPotentialEnergy() was detected";
    return 0;

}
} // namespace SofaCaribou::forcefield