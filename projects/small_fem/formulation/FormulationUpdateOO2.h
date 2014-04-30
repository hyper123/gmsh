#ifndef _FORMULATIONUPDATEOO2_H_
#define _FORMULATIONUPDATEOO2_H_

#include "SmallFem.h"
#include "FunctionSpaceScalar.h"
#include "TermProjectionField.h"
#include "TermProjectionGrad.h"
#include "TermFieldField.h"
#include "FormulationBlock.h"

#include "DDMContext.h"

/**
   @class FormulationUpdateOO2
   @brief Update Formulation for FormulationOO2

   Update Formulation for FormulationOO2
 */

class FormulationUpdateOO2: public FormulationBlock<Complex>{
 private:
  // a & b //
  Complex a;
  Complex b;

  // Function Space & Domain //
  const FunctionSpace*  fspace;
  const GroupOfElement* ddomain;

  // Local Terms //
  TermFieldField<double>*       lGout;
  TermProjectionField<Complex>* lGin;
  TermProjectionField<Complex>* lU;
  TermProjectionGrad<Complex>*  lDU;

 public:
  FormulationUpdateOO2(DDMContext& context);

  virtual ~FormulationUpdateOO2(void);

  virtual Complex weak(size_t dofI, size_t dofJ, size_t elementId) const;
  virtual Complex rhs(size_t equationI, size_t elementId)          const;

  virtual const FunctionSpace&  field(void)  const;
  virtual const FunctionSpace&  test(void)   const;
  virtual const GroupOfElement& domain(void) const;

  virtual bool isBlock(void) const;
};

/**
   @fn FormulationUpdateOO2::FormulationUpdateOO2
   @param context A DDMContext

   Instantiates a new FormulationUpdateOO2 with the given DDMContext
   **

   @fn FormulationUpdateOO2::~FormulationUpdateOO2
   Deletes this FormulationUpdateOO2
*/

#endif
