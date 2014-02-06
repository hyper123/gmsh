#include <iostream>

#include "Mesh.h"
#include "System.h"
#include "SystemHelper.h"

#include "FormulationSteadyWaveScalar.h"
#include "FormulationSteadyWaveVector.h"
#include "FormulationSteadyWaveVectorSlow.h"

#include "Timer.h"
#include "SmallFem.h"

using namespace std;

fullVector<double> fSourceVec(fullVector<double>& xyz){
  fullVector<double> res(3);

  res(0) = 0;
  res(1) = 1;
  res(2) = 0;

  return res;
}

fullVector<double> fWallVec(fullVector<double>& xyz){
  fullVector<double> res(3);

  res(0) = 0;
  res(1) = 0;
  res(2) = 0;

  return res;
}

double fSourceScal(fullVector<double>& xyz){
  return 1;
}

double fWallScal(fullVector<double>& xyz){
  return 0;
}

void compute(const Options& option){
  // Start Timer //
  Timer timer, assemble, solve;
  timer.start();

  // Get Domains //
  Mesh msh(option.getValue("-msh")[1]);
  GroupOfElement volume = msh.getFromPhysical(7);
  GroupOfElement source = msh.getFromPhysical(5);
  GroupOfElement wall   = msh.getFromPhysical(6);

  // Full Domain //
  GroupOfElement domain(msh);
  domain.add(volume);
  domain.add(source);
  domain.add(wall);

  // Get Parameters //
  const double k     = atof(option.getValue("-k")[1].c_str());
  const size_t order = atoi(option.getValue("-o")[1].c_str());

  // Chose write formulation for Steady Wave and boundary condition //
  FunctionSpace*       fs   = NULL;
  FunctionSpaceScalar* fsS = NULL;
  FunctionSpaceVector* fsV = NULL;
  Formulation<double>* wave = NULL;
  System<double>*      sys  = NULL;

  assemble.start();

  if(option.getValue("-type")[1].compare("vector") == 0){
    fs   = new FunctionSpaceVector(domain, order);
    fsV  = static_cast<FunctionSpaceVector*>(fs);
    wave = new FormulationSteadyWaveVector(volume, *fsV, k);
    sys  = new System<double>;

    sys->addFormulation(*wave);

    SystemHelper<double>::dirichlet(*sys, *fsV, source, fSourceVec);
    SystemHelper<double>::dirichlet(*sys, *fsV, wall,   fWallVec);
    cout << "Vectorial ";
  }

  else if(option.getValue("-type")[1].compare("slow") == 0){
    fs   = new FunctionSpaceVector(domain, order);
    fsV  = static_cast<FunctionSpaceVector*>(fs);
    wave = new FormulationSteadyWaveVectorSlow(volume, *fsV, k);
    sys  = new System<double>;

    sys->addFormulation(*wave);

    SystemHelper<double>::dirichlet(*sys, *fsV, source, fSourceVec);
    SystemHelper<double>::dirichlet(*sys, *fsV, wall,   fWallVec);
    cout << "Slow Vectorial ";
  }

  else if(option.getValue("-type")[1].compare("scalar") == 0){
    fs   = new FunctionSpaceScalar(domain, order);
    fsS  = static_cast<FunctionSpaceScalar*>(fs);
    wave = new FormulationSteadyWaveScalar<double>(volume, *fsS, k);
    sys  = new System<double>;

    sys->addFormulation(*wave);

    SystemHelper<double>::dirichlet(*sys, *fsS, source, fSourceScal);
    SystemHelper<double>::dirichlet(*sys, *fsS, wall,   fWallScal);
    cout << "Scalar ";
  }

  else
    throw Exception("No -type given");

  cout << "Steady Wave (Order: "  << order
       << " --- Wavenumber: "     << k
       << "): " << sys->getSize() << endl;

  // Assemble and solve //
  sys->assemble();
  assemble.stop();
  cout << "Assembled: " << assemble.time() << assemble.unit()
       << endl << flush;

  solve.start();
  sys->solve();
  solve.stop();
  cout << "Solved: " << solve.time() << solve.unit()
       << endl << flush;

  // Write Sol //
  try{
    option.getValue("-nopos");
  }
  catch(...){
    FEMSolution<double> feSol;
    sys->getSolution(feSol);
    feSol.write("swave");
  }

  // Clean //
  delete sys;
  delete wave;
  delete fs;

  // Timer -- Finalize -- Return //
  timer.stop();

  cout << "Elapsed Time: " << timer.time()
       << " s"             << endl;
}

int main(int argc, char** argv){
  // Init SmallFem //
  SmallFem::Keywords("-msh,-o,-k,-nopos,-type");
  SmallFem::Initialize(argc, argv);

  compute(SmallFem::getOptions());

  SmallFem::Finalize();
  return 0;
}
