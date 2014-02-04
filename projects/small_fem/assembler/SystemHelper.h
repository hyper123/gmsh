#ifndef _SYSTEMHELPER_H_
#define _SYSTEMHELPER_H_

/**
   @class SystemHelper
   @brief A bunch of helping class methods for linear systems

   A bunch of helping class methods for linear systems
*/

#include "FunctionSpaceScalar.h"
#include "FunctionSpaceVector.h"

#include "SystemAbstract.h"
#include "GroupOfElement.h"

template<typename scalar>
class SystemHelper{
 public:
   SystemHelper(void);
  ~SystemHelper(void);

  static void dirichlet(SystemAbstract<scalar>& sys,
                        const GroupOfElement& goe,
                        scalar (*f)(fullVector<double>& xyz));

  static void dirichlet(SystemAbstract<scalar>& sys,
                        const FunctionSpaceScalar& fs,
                        const GroupOfElement& goe,
                        scalar (*f)(fullVector<double>& xyz));

  static void dirichlet(SystemAbstract<scalar>& sys,
                        const GroupOfElement& goe,
                        fullVector<scalar> (*f)(fullVector<double>& xyz));

  static void dirichlet(SystemAbstract<scalar>& sys,
                        const FunctionSpaceVector& fs,
                        const GroupOfElement& goe,
                        fullVector<scalar> (*f)(fullVector<double>& xyz));
};

/**
   @fn SystemHelper::SystemHelper
   Instantiates a new SystemHelper (unneeded since it has only class methods)
   **

   @fn SystemHelper::~SystemHelper
   Deletes this SystemHelper
   **

   @fn SystemHelper::dirichlet(SystemAbstract<scalar>&, GroupOfElement&, scalar (*f)(fullVector<double>& xyz))
   @param sys A SystemAbstract
   @param goe A GroupOfElement
   @param f A scalar function

   Imposes on the given SystemAbstract a dirichlet condition
   on the given GroupOfElement and with the given function
   **

   @fn SystemHelper::dirichlet(SystemAbstract<scalar>&, GroupOfElement&, fullVector<scalar> (*f)(fullVector<double>& xyz))
   @param sys A SystemAbstract
   @param goe A GroupOfElement
   @param f A vectorial function

   Imposes on the given SystemAbstract a dirichlet condition
   on the given GroupOfElement and with the given function
 */

//////////////////////////////////////
// Templates Implementations:       //
// Inclusion compilation model      //
//                                  //
// Damn you gcc: we want 'export' ! //
//////////////////////////////////////

#include "SystemHelperInclusion.h"

#endif
