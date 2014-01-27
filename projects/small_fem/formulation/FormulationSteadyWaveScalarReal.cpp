#include "BasisGenerator.h"
#include "GroupOfJacobian.h"
#include "Quadrature.h"

#include "Exception.h"
#include "FormulationSteadyWaveScalar.h"

using namespace std;

template<>
FormulationSteadyWaveScalar<double>::
FormulationSteadyWaveScalar(GroupOfElement& goe, double k, size_t order){
  // Can't have 0th order //
  if(order == 0)
    throw
      Exception("Can't have a Scalar SteadyWave formulation of order 0");

  // Wavenumber Squared //
  kSquare = k * k;

  // Function Space & Basis//
  basis  = BasisGenerator::generate(goe.get(0).getType(),
                                    0, order, "hierarchical");

  fspace = new FunctionSpaceScalar(goe, *basis);

  // Gaussian Quadrature //
  Quadrature gaussGradGrad(goe.get(0).getType(), order - 1, 2);
  Quadrature gaussFF(goe.get(0).getType(), order, 2);

  const fullMatrix<double>& gC1 = gaussGradGrad.getPoints();
  const fullVector<double>& gW1 = gaussGradGrad.getWeights();

  const fullMatrix<double>& gC2 = gaussFF.getPoints();
  const fullVector<double>& gW2 = gaussFF.getWeights();

  // Local Terms //
  basis->preEvaluateDerivatives(gC1);
  basis->preEvaluateFunctions(gC2);

  GroupOfJacobian jac1(goe, gC1, "invert");
  GroupOfJacobian jac2(goe, gC2, "jacobian");

  localTerms1 = new TermGradGrad(jac1, *basis, gW1);
  localTerms2 = new TermFieldField(jac2, *basis, gW2);
}

template<>
FormulationSteadyWaveScalar<double>::~FormulationSteadyWaveScalar(void){
  delete basis;
  delete fspace;

  delete localTerms1;
  delete localTerms2;
}

template<>
double FormulationSteadyWaveScalar<double>::weak(size_t dofI, size_t dofJ,
                                                 size_t elementId) const{
  return
    localTerms1->getTerm(dofI, dofJ, elementId) -
    localTerms2->getTerm(dofI, dofJ, elementId) * kSquare;
}

template<>
double FormulationSteadyWaveScalar<double>::rhs(size_t equationI,
                                                size_t elementId) const{
  return 0;
}

template<>
bool FormulationSteadyWaveScalar<double>::isGeneral(void) const{
  return false;
}

template<>
double FormulationSteadyWaveScalar<double>::weakB(size_t dofI,
                                                  size_t dofJ,
                                                  size_t elementId) const{
  return 0;
}

template<>
const FunctionSpace& FormulationSteadyWaveScalar<double>::fs(void) const{
  return *fspace;
}
