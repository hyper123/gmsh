#ifndef _FORMULATIONEMDA_H_
#define _FORMULATIONEMDA_H_

#include <complex>
#include "FunctionSpaceScalar.h"
#include "TermFieldField.h"
#include "Formulation.h"

/**
   @class FormulationEMDA
   @brief EMDA Formulation for DDM

   EMDA Formulation for DDM
 */

class FormulationEMDA: public Formulation<std::complex<double> >{
 private:
  // Wavenumber & Chi //
  double k;
  double chi;

  // Function Space & Basis //
  const FunctionSpaceScalar* fspace;
  const Basis*                basis;

  // Domain //
  const GroupOfElement* goe;

  // Local Terms (Projection u_i*u_j) //
  TermFieldField* localTerms;

  // Quadrature //
  fullMatrix<double>* gC;
  fullVector<double>* gW;
  GroupOfJacobian*    jac;

  // DDM //
  const std::map<Dof, std::complex<double> >* ddmDof;

 public:
  FormulationEMDA(const GroupOfElement& goe,
                  const FunctionSpaceScalar& fs,
                  double k,
                  double chi,
                  const std::map<Dof, std::complex<double> >& ddmDof);

  virtual ~FormulationEMDA(void);

  virtual std::complex<double>
    weak(size_t dofI, size_t dofJ, size_t elementId) const;

  virtual std::complex<double>
    rhs(size_t equationI, size_t elementId)          const;

  virtual const FunctionSpace&  field(void)  const;
  virtual const FunctionSpace&  test(void)   const;
  virtual const GroupOfElement& domain(void) const;

 private:
  std::complex<double>
    interpolate(const MElement& element, const fullVector<double>& xyz) const;
};

/**
   @fn FormulationEMDA::FormulationEMDA
   @param goe A GroupOfElement
   @param k A real number
   @param chi A real number
   @param order A natural number
   @param ddmDof A map with the DDM Dof%s and their associated values

   Instantiates a new FormulationEMDA of the given order, wavenumber (k),
   real shift (chi) and ddm Dof%s

   The given GroupOfElement will be used as the geomtrical domain
   **

   @fn FormulationEMDA::~FormulationEMDA
   Deletes this FormulationEMDA
*/

#endif
