#ifndef _BASISLAGRANGE_H_
#define _BASISLAGRANGE_H_

#include "BasisLocal.h"
#include "FunctionSpaceScalar.h"
#include "FunctionSpaceVector.h"
#include "fullMatrix.h"
#include "polynomialBasis.h"

/**
   @interface BasisLagrange
   @brief Interface for Lagrange Basis

   This is an interface for Lagrange Basis.

   These Local Scalar Basis allow a Coefficient Matrix,
   and a Monomial Matrix, to be consulted.

   A vector from an Other Basis (set of Functions)
   can also be projected into a Lagrange Basis.
*/

class BasisLagrange: public BasisLocal{
 protected:
  polynomialBasis*        lBasis;   // Lagrange Basis
  fullMatrix<double>*     lPoint;   // Lagrange Points

  // PreEvaluation //
  mutable bool preEvaluated;
  mutable bool preEvaluatedGrad;

  mutable fullMatrix<double>* preEvaluatedFunction;
  mutable fullMatrix<double>* preEvaluatedGradFunction;

 public:
  virtual ~BasisLagrange(void);

  virtual void getFunctions(fullMatrix<double>& retValues,
                            const MElement& element,
                            double u, double v, double w) const;

  virtual void getFunctions(fullMatrix<double>& retValues,
                            size_t orientation,
                            double u, double v, double w) const;

  virtual void preEvaluateFunctions(const fullMatrix<double>& point) const;
  virtual void preEvaluateDerivatives(const fullMatrix<double>& point) const;

  virtual const fullMatrix<double>&
    getPreEvaluatedFunctions(const MElement& element) const;

  virtual const fullMatrix<double>&
    getPreEvaluatedDerivatives(const MElement& element) const;

  virtual const fullMatrix<double>&
    getPreEvaluatedFunctions(size_t orientation) const;

  virtual const fullMatrix<double>&
    getPreEvaluatedDerivatives(size_t orientation) const;

  const fullMatrix<double>& getCoefficient(void) const;
  const fullMatrix<double>& getMonomial(void) const;

  std::vector<double>
    project(const MElement& element,
            const std::vector<double>& coef,
            const FunctionSpaceScalar& fSpace);

  std::vector<fullVector<double> >
    project(const MElement& element,
            const std::vector<double>& coef,
            const FunctionSpaceVector& fSpace);

  virtual std::string toString(void) const;

 protected:
  BasisLagrange(void);
};


/**
   @internal
   @fn BasisLagrange::BasisLagrange

   Instanciates an new BasisLagrange
   @endinternal
   **

   @fn BasisLagrange::~BasisLagrange

   Deletes this BasisLagrange
   **

   @fn BasisLagrange::getCoefficient
   @return Returns the Coefficient Matrix
   **

   @fn BasisLagrange::getMonomial
   @return Returns the Monomial Matrix
   **

   @fn BasisLagrange::project(const MElement&, const std::vector<double>&, const FunctionSpaceScalar&)
   @param element A MElement
   @param coef A vector of coefficient associated
   to the given Element
   @param fSpace The (scalar) Function Space
   of the given Coefficients
   @return Projects the given Coefficients in this BasisLagrange
   **

   @fn BasisLagrange::project(const MElement&, const std::vector<double>&, const FunctionSpaceVector&)
   @param element A MElement
   @param coef A vector of coefficient associated
   to the given Element
   @param fSpace The (vectorial) Function Space
   of the given Coefficients
   @return Projects the given Coefficients in this BasisLagrange
 */

//////////////////////
// Inline Functions //
//////////////////////

inline const fullMatrix<double>& BasisLagrange::
getCoefficient(void) const{
  return lBasis->coefficients;
}

inline const fullMatrix<double>& BasisLagrange::
getMonomial(void) const{
  return lBasis->monomials;
}

#endif
