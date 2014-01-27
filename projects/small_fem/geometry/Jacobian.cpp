#include <sstream>

#include "ReferenceSpaceManager.h"
#include "Jacobian.h"

using namespace std;

const string Jacobian::jacobianString = string("jacobian");
const string Jacobian::invertString   = string("invert");
const string Jacobian::bothString     = string("both");

Jacobian::Jacobian(const MElement& element,
                   const fullMatrix<double>& point,
                   const string type){
  // Check //
  if(point.size2() != 3)
    throw
      Exception("Jacobian: point matrix is of size [N, %d] instead of [N, 3]",
                point.size2());
  // Save //
  this->element = &element;
  this->point   = &point;
  this->type    = type;

  // Set //
  jac    = NULL;
  invJac = NULL;

  // Compute //
  if(!type.compare(jacobianString))
    computeJacobians();

  else if (!type.compare(invertString))
    computeInvertFromScratch();

  else if (!type.compare(bothString)){
    computeJacobians();
    computeInvertFromJac();
  }

  else
    throw Exception("Unknown Jacobian type: %s (Types are '%s', '%s' and '%s')",
                    type.c_str(), jacobianString.c_str(),
                    invertString.c_str(), bothString.c_str());
}

Jacobian::~Jacobian(void){
  deleteJac();
  deleteInvertJac();
}

void Jacobian::deleteJac(void){
  // Check if NULL //
  if(!jac)
    return;

  // Delete //
  const size_t size = jac->size();

  for(size_t i = 0; i < size; i++){
    delete (*jac)[i]->first;
    delete (*jac)[i];
  }

  delete jac;

  // Set NULL //
  jac = NULL;
}

void Jacobian::deleteInvertJac(void){
  // Check if NULL //
  if(!invJac)
    return;

  // Delete //
  const size_t size = invJac->size();

  for(size_t i = 0; i < size; i++){
    delete (*invJac)[i]->first;
    delete (*invJac)[i];
  }

  delete invJac;

  // Set NULL //
  invJac = NULL;
}

void Jacobian::computeJacobians(void){
  // Init Jac //
  const size_t nPoint = point->size1();
  jac = new jac_t(nPoint) ;

  // Fill Jac //
  jac_pair*           tmp;
  fullMatrix<double>* mJac;

  for(size_t i = 0; i < nPoint; i++){
    tmp  = new jac_pair;
    mJac = new fullMatrix<double>(3, 3);

    tmp->second =
      ReferenceSpaceManager::getJacobian(*element,
                                         (*point)(i, 0),
                                         (*point)(i, 1),
                                         (*point)(i, 2),
                                         *mJac);
    tmp->first = mJac;
    (*jac)[i]  = tmp;
  }
}

void Jacobian::computeInvertFromJac(void){
  // Init InvJac //
  const size_t nPoint = point->size1();
  invJac = new jac_t(nPoint);

  // Fill InvJac //
  jac_pair*           tmp;
  fullMatrix<double>* mIJac;

  for(size_t i = 0; i < nPoint; i++){
    tmp   = new jac_pair;
    mIJac = new fullMatrix<double>(3, 3);

    naiveInvert(*(*jac)[i]->first, (*jac)[i]->second, *mIJac);

    tmp->first  = mIJac;
    tmp->second = (*jac)[i]->second;

    (*invJac)[i] = tmp;
  }
}

void Jacobian::computeInvertFromScratch(void){
  // Init InvJac //
  const size_t nPoint = point->size1();
  invJac = new jac_t(nPoint) ;

  // Fill InvJac //
  jac_pair*           tmp;
  fullMatrix<double>* mIJac;
  fullMatrix<double>  jac(3, 3);

  for(size_t i = 0; i < nPoint; i++){
    tmp   = new jac_pair;
    mIJac = new fullMatrix<double>(3, 3);

    tmp->second =
      ReferenceSpaceManager::getJacobian(*element,
                                         (*point)(i, 0),
                                         (*point)(i, 1),
                                         (*point)(i, 2),
                                         jac);
    naiveInvert(jac, tmp->second, *mIJac);

    tmp->first   = mIJac;
    (*invJac)[i] = tmp;
  }
}

void Jacobian::naiveInvert(const fullMatrix<double>& A,
                           double detA,
                           fullMatrix<double>& invA){
  invA(0, 0) = (A(1, 1) * A(2, 2) - A(1, 2) * A(2, 1)) / detA;
  invA(0, 1) = (A(0, 2) * A(2, 1) - A(0, 1) * A(2, 2)) / detA;
  invA(0, 2) = (A(0, 1) * A(1, 2) - A(0, 2) * A(1, 1)) / detA;

  invA(1, 0) = (A(1, 2) * A(2, 0) - A(1, 0) * A(2, 2)) / detA;
  invA(1, 1) = (A(0, 0) * A(2, 2) - A(0, 2) * A(2, 0)) / detA;
  invA(1, 2) = (A(0, 2) * A(1, 0) - A(0, 0) * A(1, 2)) / detA;

  invA(2, 0) = (A(1, 0) * A(2, 1) - A(1, 1) * A(2, 0)) / detA;
  invA(2, 1) = (A(0, 1) * A(2, 0) - A(0, 0) * A(2, 1)) / detA;
  invA(2, 2) = (A(0, 0) * A(1, 1) - A(0, 1) * A(1, 0)) / detA;
}

string Jacobian::toString(void) const{
  stringstream stream;
  size_t size = point->size1();

  stream << std::scientific;

  stream << "Element " << element->getNum()
         << " Jacobian" << endl;

  for(size_t i = 0; i < size; i++){
    stream << "   " << "Point ("
           << (*point)(i, 0) << ", "
           << (*point)(i, 1) << ", "
           << (*point)(i, 2) << ")"
           << endl;

    if(jac){
      stream << "   " << "   " << "Jacobian Matrix" << endl;
      stream << "   " << "   " << "   "
             << (*(*jac)[i]->first)(0, 0) << "\t"
             << (*(*jac)[i]->first)(0, 1) << "\t"
             << (*(*jac)[i]->first)(0, 2) << endl;
      stream << "   " << "   " << "   "
             << (*(*jac)[i]->first)(1, 0) << "\t"
             << (*(*jac)[i]->first)(1, 1) << "\t"
             << (*(*jac)[i]->first)(1, 2) << endl;
      stream << "   " << "   " << "   "
             << (*(*jac)[i]->first)(2, 0) << "\t"
             << (*(*jac)[i]->first)(2, 1) << "\t"
             << (*(*jac)[i]->first)(0, 2) << endl;
      stream << endl;
    }

    if(invJac){
      stream << "   " << "   " << "Invert Jacobian Matrix" << endl;
      stream << "   " << "   " << "   "
             << (*(*invJac)[i]->first)(0, 0) << "\t"
             << (*(*invJac)[i]->first)(0, 1) << "\t"
             << (*(*invJac)[i]->first)(0, 2) << endl;
      stream << "   " << "   " << "   "
             << (*(*invJac)[i]->first)(1, 0) << "\t"
             << (*(*invJac)[i]->first)(1, 1) << "\t"
             << (*(*invJac)[i]->first)(1, 2) << endl;
      stream << "   " << "   " << "   "
             << (*(*invJac)[i]->first)(2, 0) << "\t"
             << (*(*invJac)[i]->first)(2, 1) << "\t"
             << (*(*invJac)[i]->first)(0, 2) << endl;
      stream << endl;
    }

    if(jac)
      stream << "   " << "   " << "Jacobian Matrix Determinant: "
             << (*jac)[i]->second << endl;

    else if(invJac)
      stream << "   " << "   " << "Jacobian Matrix Determinant: "
             << (*invJac)[i]->second << endl;
  }

  return stream.str();
}
