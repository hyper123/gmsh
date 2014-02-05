#ifndef _INTERPOLATOR_H_
#define _INTERPOLATOR_H_

#include <map>

#include "GroupOfElement.h"
#include "FunctionSpace.h"
#include "fullMatrix.h"

/**
   @class Interpolator
   @brief Interpolating method for FEM Solutions

   This class allows the interpolation of FEM Solutions.

   @todo
   NEED TO BE MERGED WITH FEMSOLUTION: NEED HO PVIEW WITH ARBITRARY BASIS
 */

template <typename scalar>
class Interpolator{
 public:
   Interpolator(void);
  ~Interpolator(void);

  static void interpolate(const GroupOfElement& goe,
                          const FunctionSpace& fs,
                          const std::map<Dof, scalar>& coef,
                          const fullMatrix<double>& point,
                          fullMatrix<scalar>& values);
 private:
  static void interpolate(const MElement& element,
                          const FunctionSpace& fs,
                          const std::vector<scalar>& coef,
                          const fullVector<double>& xyz,
                          fullVector<scalar>& value);
};


/**
   @fn Interpolator::Interpolator
   Instanciate a new Interpolator
   (this is not required, since Interpolator has only one class method)
   **

   @fn Interpolator::~Interpolator
   Deletes this Interpolator
   (this is not required, since Interpolator has only one class method)
   **

   @fn Interpolator::interpolate
   @param fs A FunctionSpace
   @param dofM A DofManager
   @param coef A vector of coefficients, solution of a FEM problem
   @param point A set of point coordinates (3D):
   one point per row and one coordinate per column
   @param values A matrix

   Interpolate the given finite element solution (coef) onto the given points.
   The domain of the solution is given by FunctionSpace::getSupport(),
   and each Dof of this domain is numbered in the given DofManager.
   An entry in the DofManager corresponds to the same entry in coef.

   The interpolated values are stored in values:
   @li Each row is point
   @li Each column is a coordinate of the solution
   (1 for scalar problems and 3 for vectorial ones)
 */

//////////////////////////////////////
// Templates Implementations:       //
// Inclusion compilation model      //
//                                  //
// Damn you gcc: we want 'export' ! //
//////////////////////////////////////

#include "InterpolatorInclusion.h"

#endif
