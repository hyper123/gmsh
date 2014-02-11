#include "BasisGenerator.h"
#include "GroupOfJacobian.h"
#include "Quadrature.h"

#include "FormulationNeumann.h"

using namespace std;

FormulationNeumann::FormulationNeumann(const GroupOfElement& domain,
                                       const FunctionSpaceScalar& fs,
                                       double k){
  // Save Domain //
  goe = &domain;

  // Check GroupOfElement Stats: Uniform Mesh //
  pair<bool, size_t> uniform = domain.isUniform();
  size_t               eType = uniform.second;

  if(!uniform.first)
    throw Exception("FormulationNeumann needs a uniform mesh");

  // Wavenumber //
  this->k = k;

  // Save FunctionSpace & Get Basis //
  const Basis& basis = fs.getBasis(eType);
  const size_t order = basis.getOrder();
  fspace             = &fs;

  // Gaussian Quadrature //
  Quadrature gaussFF(eType, order, 2);
  const fullMatrix<double>& gC = gaussFF.getPoints();
  const fullVector<double>& gW = gaussFF.getWeights();

  // Local Terms //
  basis.preEvaluateFunctions(gC);

  GroupOfJacobian jac(domain, gC, "jacobian");

  localTerms = new TermFieldField(jac, basis, gW);
}

FormulationNeumann::~FormulationNeumann(void){
  delete localTerms;
}

Complex FormulationNeumann::
weak(size_t dofI, size_t dofJ, size_t elementId) const{
  return Complex(0, -1 * k * localTerms->getTerm(dofI, dofJ, elementId));
}

Complex FormulationNeumann::rhs(size_t equationI, size_t elementId) const{
  return Complex(0, 0);
}

const FunctionSpace& FormulationNeumann::field(void) const{
  return *fspace;
}

const FunctionSpace& FormulationNeumann::test(void) const{
  return *fspace;
}

const GroupOfElement& FormulationNeumann::domain(void) const{
  return *goe;
}
