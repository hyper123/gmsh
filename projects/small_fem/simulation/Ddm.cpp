#include <cmath>
#include <iostream>

#include "SmallFem.h"

#include "SolverDDM.h"

#include "DDMContextEMDA.h"
#include "DDMContextOO2.h"
#include "DDMContextOSRC.h"

#include "System.h"
#include "SystemHelper.h"

#include "FormulationHelper.h"

#include "FormulationOO2.h"
#include "FormulationEMDA.h"
#include "FormulationOSRC.h"
#include "FormulationSommerfeld.h"
#include "FormulationSteadyWave.h"
#include "FormulationUpdateEMDA.h"
#include "FormulationUpdateOO2.h"
#include "FormulationUpdateOSRC.h"

using namespace std;

Complex fSource(fullVector<double>& xyz){
  return Complex(1, 0);
}

void compute(const Options& option){
  // MPI //
  int numProcs;
  int myId;
  MPI_Comm_size(MPI_COMM_WORLD,&numProcs);
  MPI_Comm_rank(MPI_COMM_WORLD,&myId);

  // Get Parameters //
  const string ddmType = option.getValue("-ddm")[1];
  const double k       = atof(option.getValue("-k")[1].c_str());
  const size_t order   = atoi(option.getValue("-o")[1].c_str());
  const size_t maxIt   = atoi(option.getValue("-max")[1].c_str());

  // DDM Formulations //
  const string emdaType("emda");
  const string oo2Type("oo2");
  const string osrcType("osrc");

  // Variables
  const double Pi = atan(1.0) * 4;
  double lc       = 0;
  double chi      = 0;
  Complex ooA     = 0;
  Complex ooB     = 0;
  int NPade       = 0;
  Complex keps    = 0;

  // EMDA Stuff
  if(ddmType == emdaType)
    chi = atof(option.getValue("-chi")[1].c_str()) * k;

  // OO2 Stuff
  if(ddmType == oo2Type){
    lc = atof(option.getValue("-lc")[1].c_str());

    double ooXsiMin = 0;
    double ooXsiMax = Pi / lc;
    double ooDeltaK = Pi / .06;

    double tmp0 =
      (k * k - ooXsiMin * ooXsiMin) * (k * k - (k - ooDeltaK) * (k - ooDeltaK));

    double tmp1 =
      (ooXsiMax * ooXsiMax - k * k) * ((k + ooDeltaK) * (k + ooDeltaK) - k * k);

    Complex ooAlpha = pow(Complex(tmp0, 0), 0.25) * Complex(0, 1);
    Complex ooBeta  = pow(Complex(tmp1, 0), 0.25);

    ooA = -(ooAlpha * ooBeta - k * k) / (ooAlpha + ooBeta);
    ooB = Complex(-1, 0) / (ooAlpha + ooBeta);
  }

  // OSRC Stuff
  if(ddmType == osrcType){
    double ck = atof(option.getValue("-ck")[1].c_str());
    NPade     = atoi(option.getValue("-pade")[1].c_str());
    keps      = k + Complex(0, k * ck);
  }

  // Get Domains //
  Mesh msh(option.getValue("-msh")[1]);
  GroupOfElement volume(msh);
  GroupOfElement source(msh);
  GroupOfElement infinity(msh);
  GroupOfElement ddmBorder(msh);

  if(myId == 0){
    volume.add(msh.getFromPhysical(7));
    source.add(msh.getFromPhysical(5));
    infinity.add(msh.getFromPhysical(61));
    ddmBorder.add(msh.getFromPhysical(4));
  }

  else{
    volume.add(msh.getFromPhysical(8));
    //No source//
    infinity.add(msh.getFromPhysical(62));
    ddmBorder.add(msh.getFromPhysical(4));
  }

  // Full Domain //
  GroupOfElement domain(msh);
  domain.add(volume);
  domain.add(source);
  domain.add(infinity);
  domain.add(ddmBorder);

  // Function Space //
  FunctionSpaceScalar                fs(domain, order);
  vector<const FunctionSpaceScalar*> phi(NPade); // OSRC

  for(int j = 0; j < NPade; j++)
    phi[j] = new FunctionSpaceScalar(ddmBorder, order);

  // Steady Wave Formulation //
  FormulationSteadyWave<Complex> wave(volume, fs, k);
  FormulationSommerfeld          sommerfeld(infinity, fs, k);

  // DDM Solution Map //
  map<Dof, Complex> ddmG;
  map<Dof, Complex> rhsG;
  FormulationHelper::initDofMap(fs, ddmBorder, ddmG);
  FormulationHelper::initDofMap(fs, ddmBorder, rhsG);

  // Ddm Formulation //
  DDMContext*         context = NULL;
  Formulation<Complex>*   ddm = NULL;
  Formulation<Complex>* upDdm = NULL;

  if(ddmType == emdaType){
    context = new DDMContextEMDA(ddmBorder, fs, k, chi);
    context->setDDMDofs(ddmG);

    ddm     = new FormulationEMDA(static_cast<DDMContextEMDA&>(*context));
    upDdm   = new FormulationUpdateEMDA(static_cast<DDMContextEMDA&>(*context));
  }

  else if(ddmType == oo2Type){
    context = new DDMContextOO2(ddmBorder, fs, ooA, ooB);
    context->setDDMDofs(ddmG);

    ddm     = new FormulationOO2(static_cast<DDMContextOO2&>(*context));
    upDdm   = new FormulationUpdateOO2(static_cast<DDMContextOO2&>(*context));
  }

  else if(ddmType == osrcType){
    context = new DDMContextOSRC(ddmBorder, fs, phi, k, keps, NPade);
    context->setDDMDofs(ddmG);

    ddm     = new FormulationOSRC(static_cast<DDMContextOSRC&>(*context));
    upDdm   = new FormulationUpdateOSRC(static_cast<DDMContextOSRC&>(*context));
  }

  else
    throw Exception("Unknown %s DDM border term", ddmType.c_str());

  ddm->update();

  // Solve Non homogenous problem //
  System<Complex> nonHomogenous;
  nonHomogenous.addFormulation(wave);
  nonHomogenous.addFormulation(sommerfeld);
  nonHomogenous.addFormulation(*ddm);

  // Constraint
  if(myId == 0)
    SystemHelper<Complex>::dirichlet(nonHomogenous, fs, source, fSource);

  // Assemble & Solve
  nonHomogenous.assemble();
  nonHomogenous.solve();

  // Solve Non homogenous DDM problem //
  context->setSystem(nonHomogenous);
  upDdm->update(); // update volume solution (at DDM border)

  System<Complex> nonHomogenousDDM;
  nonHomogenousDDM.addFormulation(*upDdm);

  nonHomogenousDDM.assemble();
  nonHomogenousDDM.solve();
  nonHomogenousDDM.getSolution(rhsG, 0);

  // DDM Solver //
  SolverDDM solver(wave, sommerfeld, source, *context, *ddm, *upDdm, rhsG);

  solver.solve(maxIt);

  // Full Problem //
  solver.getSolution(ddmG);

  context->setDDMDofs(ddmG);
  ddm->update();

  System<Complex> full;
  full.addFormulation(wave);
  full.addFormulation(sommerfeld);
  full.addFormulation(*ddm);

  // Constraint
  SystemHelper<Complex>::dirichlet(full, fs, source, fSource);

  full.assemble();
  full.solve();

  full.getSolution(ddmG, 0);

  // Draw Solution //
  stringstream stream;
  stream << "ddm" << myId;

  FEMSolution<Complex> feSol;
  full.getSolution(feSol, fs, volume);
  feSol.write(stream.str());

  // Clean //
  delete ddm;
  delete upDdm;
  delete context;

  for(int j = 0; j < NPade; j++)
    delete phi[j];
}

int main(int argc, char** argv){
  // Init SmallFem //
  SmallFem::Keywords("-msh,-o,-k,-max,-ddm,-chi,-lc,-ck,-pade");
  SmallFem::Initialize(argc, argv);

  compute(SmallFem::getOptions());

  SmallFem::Finalize();
  return 0;
}
