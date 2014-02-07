// Gmsh - Copyright (C) 1997-2014 C. Geuzaine, J.-F. Remacle
//
// See the LICENSE.txt file for license information. Please report all
// bugs and problems to the public mailing list <gmsh@geuz.org>.

#ifndef _EIGEN_SOLVER_H_
#define _EIGEN_SOLVER_H_

#include <string>
#include <complex>
#include "GmshConfig.h"
#include "GmshMessage.h"
#include "dofManager.h"

#if defined(HAVE_SLEPC)

#include "linearSystemPETSc.h"

class eigenSolver{
 private:
  linearSystemPETSc<double> *_A, *_B;
  bool _hermitian;
  std::vector<std::complex<double> > _eigenValues;
  std::vector<std::vector<std::complex<double> > > _eigenVectors;
  void _try(int ierr) const;
 public:
  eigenSolver(dofManager<double> *manager, std::string A,
              std::string B="", bool hermitian=true);
  eigenSolver(linearSystemPETSc<double> *A, linearSystemPETSc<double>* B = NULL,
              bool hermitian=true);
  bool solve(int numEigenValues=0, std::string which="");
  int getNumEigenValues(){ return _eigenValues.size(); }
  std::complex<double> getEigenValue(int num){ return _eigenValues[num]; }
  std::vector<std::complex<double> > &getEigenVector(int num){ return _eigenVectors[num]; }
  void clear()
  {
    _eigenValues.clear();
    _eigenVectors.clear();
  };
  std::complex<double> getEigenVectorComp(int num, int com)
  {
    return _eigenVectors[num][com];
  };
};

#else

#include "linearSystemPETSc.h"

class eigenSolver{
 private:
  std::vector<std::complex<double> > _dummy;
 public:
  eigenSolver(dofManager<double> *manager, std::string A,
              std::string B="", bool hermitian=false){}
  eigenSolver(linearSystemPETSc<double> *A,linearSystemPETSc<double>* B = NULL,
              bool hermitian=false){}
  bool solve(int numEigenValues=0, std::string which="")
  {
    Msg::Error("Eigen solver requires SLEPc");
    return false;
  }
  int getNumEigenValues(){ return 0; }
  std::complex<double> getEigenValue(int num){ return 0.; }
  std::vector<std::complex<double> > &getEigenVector(int num){ return _dummy; }
  void clear(){}
  std::complex<double> getEigenVectorComp(int num, int com) { return 0.; }
};

#endif

#endif
