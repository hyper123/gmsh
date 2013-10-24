#include "Mesh.h"
#include "System.h"
#include "SmallFem.h"

#include "FormulationElementGradGrad.h"
#include "FormulationElementOneFormOneForm.h"
#include "FormulationElementCurlCurl.h"

using namespace std;

void compute(const Options& option){
  // Get Domains //
  Mesh msh(option.getValue("-msh")[0]);
  GroupOfElement domain = msh.getFromPhysical(7);

  if(domain.getNumber() != 1)
    throw
      Exception
      ("To comute the Stiffness Matrix, I need only one element (%d given)",
       domain.getNumber());

  // Get Parameter //
  size_t order       = atoi(option.getValue("-order")[0].c_str());
  size_t orientation = atoi(option.getValue("-orient")[0].c_str());

  // Assemble the asked matrix //
  string       problemName = option.getValue("-type")[0];
  Formulation* formulation;

  if(!problemName.compare("grad"))
    formulation =
      new FormulationElementGradGrad(domain, order, orientation);

  else if(!problemName.compare("curl"))
    formulation =
      new FormulationElementCurlCurl(domain, order, orientation);

  else if(!problemName.compare("one"))
    formulation =
      new FormulationElementOneFormOneForm(domain, order, orientation);

  else
    throw
      Exception
      ("I do not know what type of matrix '%s' is\nI know:%s%s%s\n",
       problemName.c_str(),
       " grad,"
       " one,"
       " curl");

  System sys(*formulation);

  sys.assemble();

  // Write Matrix //
  string matrixName = option.getValue("-name")[0];
  string fileName   = matrixName;
  fileName.append(".m");

  sys.writeMatrix(fileName, matrixName);

  // Delete //
  delete formulation;
}

int main(int argc, char** argv){
  SmallFem::Initialize(argc, argv);

  compute(SmallFem::getOptions());

  SmallFem::Finalize();
  return 0;
}
