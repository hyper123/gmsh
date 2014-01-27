#include "BasisGenerator.h"
#include "GroupOfJacobian.h"
#include "Quadrature.h"

#include "FormulationEigenFrequencyVector.h"

using namespace std;

const double FormulationEigenFrequencyVector::cSquare = 1;

FormulationEigenFrequencyVector::
FormulationEigenFrequencyVector(GroupOfElement& goe,
                                size_t order){
  // Function Space & Basis //
  basis  = BasisGenerator::generate(goe.get(0).getType(),
                                    1, order, "hierarchical");

  fspace = new FunctionSpaceVector(goe, *basis);

  // Gaussian Quadrature //
  Quadrature gaussCurlCurl(goe.get(0).getType(), order - 1, 2);
  Quadrature gaussFF(goe.get(0).getType(), order, 2);

  const fullMatrix<double>& gC1 = gaussCurlCurl.getPoints();
  const fullVector<double>& gW1 = gaussCurlCurl.getWeights();

  const fullMatrix<double>& gC2 = gaussFF.getPoints();
  const fullVector<double>& gW2 = gaussFF.getWeights();

  // Local Terms //
  basis->preEvaluateDerivatives(gC1);
  basis->preEvaluateFunctions(gC2);

  GroupOfJacobian jac1(goe, gC1, "jacobian");
  GroupOfJacobian jac2(goe, gC2, "invert");

  localTerms1 = new TermCurlCurl(jac1, *basis, gW1);
  localTerms2 = new TermGradGrad(jac2, *basis, gW2);
}

FormulationEigenFrequencyVector::~FormulationEigenFrequencyVector(void){
  delete basis;
  delete fspace;

  delete localTerms1;
  delete localTerms2;
}

std::complex<double>
FormulationEigenFrequencyVector::weak(size_t dofI, size_t dofJ,
                                      size_t elementId) const{

  return std::complex<double>
    (localTerms1->getTerm(dofI, dofJ, elementId), 0);
}

std::complex<double>
FormulationEigenFrequencyVector::weakB(size_t dofI, size_t dofJ,
                                       size_t elementId) const{

  return std::complex<double>
    (localTerms2->getTerm(dofI, dofJ, elementId) / cSquare, 0);
}

std::complex<double>
FormulationEigenFrequencyVector::rhs(size_t dofI,
                                     size_t elementId) const{
  return std::complex<double>(0, 0);
}

bool FormulationEigenFrequencyVector::isGeneral(void) const{
  return true;
}

const FunctionSpace& FormulationEigenFrequencyVector::fs(void) const{
  return *fspace;
}
