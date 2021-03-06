#include <iostream>

#include "SmallFem.h"

#include "System.h"
#include "SystemHelper.h"
#include "Interpolator.h"

#include "FormulationHelper.h"
#include "FormulationSommerfeld.h"
#include "FormulationSteadyWave.h"

using namespace std;

static const int    scal = 0;
static const int    vect = 1;
static       double k;

Complex fSourceScal(fullVector<double>& xyz){
  const double ky = 1 * M_PI / 1;
  const double kz = 1 * M_PI / 1;

  return Complex(sin(ky * xyz(1)) * sin(kz * xyz(2)), 0);
}

Complex fZeroScal(fullVector<double>& xyz){
  return Complex(0, 0);
}

fullVector<Complex> fSourceVect(fullVector<double>& xyz){
  const Complex I = Complex(0, 1);

  const double ky = 1 * M_PI / 1;
  const double kz = 1 * M_PI / 1;
  const double kc = sqrt((ky * ky) + (kz * kz));

  Complex beta;
  if((k * k) - (kc * kc) >= 0)
    beta = Complex(sqrt((k * k) - (kc * kc)), 0);

  else
    beta = Complex(0, -1 * sqrt((kc * kc) - (k * k)));

  fullVector<Complex> tmp(3);
  /*
  tmp(0) = Complex(                    sin(ky * xyz(1)) * sin(kz * xyz(2)),0);
  tmp(1) = I * beta * ky / (kc * kc) * cos(ky * xyz(1)) * sin(kz * xyz(2));
  tmp(2) = I * beta * kz / (kc * kc) * cos(kz * xyz(2)) * sin(ky * xyz(1));
  */
  tmp(0) = Complex(0, 0);
  tmp(1) = Complex(1, 0);
  tmp(2) = Complex(0, 0);

  return tmp;
}

fullVector<Complex> fZeroVect(fullVector<double>& xyz){
  fullVector<Complex> tmp(3);

  tmp(0) = Complex(0, 0);
  tmp(1) = Complex(0, 0);
  tmp(2) = Complex(0, 0);

  return tmp;
}

void compute(const Options& option){
  // Get Type //
  int type;
  if(option.getValue("-type")[1].compare("scalar") == 0){
    cout << "Scalar Waveguide" << endl << flush;
    type = scal;
  }

  else if(option.getValue("-type")[1].compare("vector") == 0){
    cout << "Vetorial Waveguide" << endl << flush;
    type = vect;
  }

  else
    throw Exception("Bad -type: %s", option.getValue("-type")[1].c_str());

  // Get Parameters //
  const size_t nDom  = atoi(option.getValue("-n")[1].c_str());
  k                  = atof(option.getValue("-k")[1].c_str());
  const size_t order = atoi(option.getValue("-o")[1].c_str());

  cout << "Wavenumber: " << k     << endl
       << "Order:      " << order << endl
       << "# Domain:   " << nDom  << endl << flush;

  // Get Domains //
  Mesh msh(option.getValue("-msh")[1]);
  GroupOfElement   source(msh.getFromPhysical(1 * nDom + 1));
  GroupOfElement     zero(msh.getFromPhysical(2 * nDom + 2));
  GroupOfElement infinity(msh.getFromPhysical(2 * nDom + 1));
  GroupOfElement   volume(msh);

  vector<GroupOfElement*> perVolume(nDom);
  for(size_t i = 0; i < nDom; i++){
    perVolume[i] = new GroupOfElement(msh.getFromPhysical(i + 1));
    volume.add(*perVolume[i]);
  }

  // Full Domain //
  vector<const GroupOfElement*> domain(4);
  domain[0] = &volume;
  domain[1] = &source;
  domain[2] = &zero;
  domain[3] = &infinity;

  // Function Space //
  FunctionSpace* fs = NULL;

  if(type == scal)
    fs = new FunctionSpaceScalar(domain, order);
  else
    fs = new FunctionSpaceVector(domain, order);

  // Steady Wave Formulation //
  FormulationSteadyWave<Complex> wave(volume,   *fs, k);
  FormulationSommerfeld    sommerfeld(infinity, *fs, k);

  // Solve //
  System<Complex> system;
  system.addFormulation(wave);
  system.addFormulation(sommerfeld);

  // Constraint
  if(fs->isScalar()){
    SystemHelper<Complex>::dirichlet(system, *fs, zero  , fZeroScal);
    SystemHelper<Complex>::dirichlet(system, *fs, source, fSourceScal);
  }
  else{
    SystemHelper<Complex>::dirichlet(system, *fs, zero  , fZeroVect);
    SystemHelper<Complex>::dirichlet(system, *fs, source, fSourceVect);
  }

  // Assemble
  system.assemble();
  cout << "Assembled: " << system.getSize() << endl << flush;

  // Sove
  system.solve();
  cout << "Solved!" << endl << flush;

  // Draw Solution //
  try{
    option.getValue("-nopos");
  }

  catch(...){
    cout << "Writing solution..." << endl << flush;

    stringstream stream;
    try{
      vector<string> name = option.getValue("-name");
      stream << name[1];
    }
    catch(...){
      stream << "waveguide";
    }

    try{
      // Get Visu Mesh //
      vector<string> visuStr = option.getValue("-interp");
      Mesh           visuMsh(visuStr[1]);

      // Get Solution //
      map<Dof, Complex> sol;

      FormulationHelper::initDofMap(*fs, volume, sol);
      system.getSolution(sol, 0);

      // Interoplate //
      for(size_t i = 0; i < nDom; i++){
        // GroupOfElement to interoplate on
        GroupOfElement visuGoe(visuMsh.getFromPhysical(i + 1));

        // Interpolation
        stringstream name;
        name << stream.str() << i << ".dat";

        map<const MVertex*, vector<Complex> > map;
        Interpolator<Complex>::interpolate(*perVolume[i], visuGoe, *fs,sol,map);
        Interpolator<Complex>::write(name.str(), map);
      }
    }

    catch(...){
      FEMSolution<Complex> feSol;
      system.getSolution(feSol, *fs, volume);
      feSol.write(stream.str());
    }
  }

  // Clean //
  for(size_t i = 0; i < nDom; i++)
    delete perVolume[i];

  delete fs;
}

int main(int argc, char** argv){
  // Init SmallFem //
  SmallFem::Keywords("-msh,-o,-k,-n,-type,-interp,-name,-nopos,");
  SmallFem::Initialize(argc, argv);

  compute(SmallFem::getOptions());

  SmallFem::Finalize();
  return 0;
}
