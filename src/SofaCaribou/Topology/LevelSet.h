#pragma once

#include <SofaCaribou/config.h>
#include <SofaCaribou/Topology/IsoSurface.h>

DISABLE_ALL_WARNINGS_BEGIN
#include <sofa/defaulttype/VecTypes.h>
#include <sofa/core/objectmodel/BaseObject.h>
DISABLE_ALL_WARNINGS_END

#include <Eigen/Dense>
#include <math.h>

namespace SofaCaribou::topology {

class LevelSet : public IsoSurface<sofa::defaulttype::Vec3Types> {
    template < class T = void* >
    using Data = sofa::core::objectmodel::Data<T>;
    using Base = IsoSurface<sofa::defaulttype::Vec3Types>;
    using Base::Coord;
    using Base::Real;
public:

    LevelSet()
    {}

    inline Real iso_value(const Eigen::Matrix<Real, 3, 1> & X) const final {
        Real x = X[0];
        Real y = X[1];
        Real z = X[2];

        /** objective for this part : 
         * finally compute the signed distance between X and the real boundary of the domain
         * to generate a ficitious grid for every complex domain 
         */
        
        Real value(0);
        // Some examples of level-set functions 
       
        // sphere : 
        value = (x-0.5)*(x-0.5)*2 + (y-0.5)*(y-0.5)*3 + (z-0.5)*(z-0.5)*4 - 1.;
        
        // another sphere
        // value = (x-0.5)*(x-0.5) + (y-0.5)*(y-0.5) + (z-0.5)*(z-0.5) - 1./8.; 
        
        // "Swiss-cheese block"
        // value = (x*x+y*y-4)*(x*x+y*y-4) + (z*z-1)*(z*z-1) + (y*y+z*z-4)*(y*y+z*z-4) + (x*x-1)*(x*x-1) + (z*z+x*x-4)*(z*z+x*x-4) + (y*y-1)*(y*y-1) -15;
        
        // "doughnut"
        // value = (1.2 - sqrt(x*x + y*y))*(1.2 - sqrt(x*x + y*y)) + z*z - 0.3*0.3;
        
        return value;
    }


};

}
