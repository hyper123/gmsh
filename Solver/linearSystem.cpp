#include "GmshConfig.h"
#include "linearSystemFull.h"
#include "linearSystemCSR.h"
#include "linearSystemGMM.h"
#include "linearSystemPETSc.h"

#include "Bindings.h"

void linearSystemBase::registerBindings(binding *b){
  methodBinding *cm;
  classBinding *cb = b->addClass<linearSystemBase>("linearSystemBase");
  cb->setDescription("Base class for linear systems");
  cm = cb->addMethod("setParameter", &linearSystemBase::setParameter);
  cm->setDescription("set linearSystem parameters");
  cm->setArgNames("key","value",NULL);
  
  cb = b->addClass<linearSystem<double> >("linearSystemDouble");
  cb->setDescription("An abstraction of a linear system Ax = b.");
  cm = cb->addMethod("systemSolve", &linearSystem<double>::systemSolve);
  cm->setDescription("compute x = A^{-1}b");
  cb->setParentClass<linearSystemBase>();
  
#ifdef HAVE_TAUCS
  cb = b->addClass<linearSystemCSRTaucs<double> >("linearSystemCSRTaucs");
  cb->setDescription("A linear system solver, based on TAUCS, and that is ok for SDP sparse matrices.");
  cm = cb->setConstructor<linearSystemCSRTaucs<double> >();
  cm->setDescription ("A new TAUCS<double> solver");
  cm->setArgNames(NULL);
  cm = cb->addMethod("getNNZ", &linearSystemCSRTaucs<double>::getNNZ);
  cm->setDescription("get the number of non zero entries");
  cm = cb->addMethod("getNbUnk", &linearSystemCSRTaucs<double>::getNbUnk);
  cm->setDescription("get the number of unknowns");

  cb->setParentClass<linearSystem<double> >();
#endif

  cb = b->addClass<linearSystemFull<double> >("linearSystemFull");
  cb->setDescription("A linear system solver, based on LAPACK (full matrices)");
  cm = cb->setConstructor<linearSystemFull<double> >();
  cm->setDescription ("A new Lapack based <double> solver");
  cm->setArgNames(NULL);
  cb->setParentClass<linearSystem<double> >();
  // block
  cb = b->addClass<linearSystem<fullMatrix<double> > >("linearSystemFullMatrixDouble");
  cb->setDescription("An abstraction of a linear system Ax = b.");
  cm = cb->addMethod("systemSolve", &linearSystem<fullMatrix<double> >::systemSolve);
  cm->setDescription("compute x = A^{-1}b");
  cb->setParentClass<linearSystemBase>();
#ifdef HAVE_PETSC
  linearSystemPETScRegisterBindings (b);
#endif
}

void linearSystemBase::setParameter (std::string key, std::string value) {
  if (isAllocated())
    Msg::Error("this system is already allocated, parameters cannot be set");
  _parameters[key] = value;
}
