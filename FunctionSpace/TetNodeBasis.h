#ifndef _TETNODEBASIS_H_
#define _TETNODEBASIS_H_

#include "BasisHierarchical0Form.h"

/**
   @class TetNodeBasis
   @brief A Node Basis for Tetrahedra

   This class can instantiate a Node-Based Basis
   (high or low order) for Tetrahedra.

   It uses
   <a href="http://www.hpfem.jku.at/publications/szthesis.pdf">Zaglmayr's</a>
   Basis for high order Polynomial%s generation.@n
 */

class TetNodeBasis: public BasisHierarchical0Form{
 public:
  //! @param order The order of the Basis
  //!
  //! Returns a new Node-Basis for Tetrahedra of the given order
  TetNodeBasis(size_t order);

  //! Deletes this Basis
  //!
  virtual ~TetNodeBasis(void);
};

#endif
