// Gmsh - Copyright (C) 1997-2009 C. Geuzaine, J.-F. Remacle
//
// See the LICENSE.txt file for license information. Please report all
// bugs and problems to <gmsh@geuz.org>.

#ifndef _FINITE_ELEMENT_H_
#define _FINITE_ELEMENT_H_

#include "Plugin.h"

extern "C"
{
  GMSH_Plugin *GMSH_RegisterFiniteElementPlugin();
}

class GMSH_FiniteElementPlugin : public GMSH_PostPlugin
{
 public:
  GMSH_FiniteElementPlugin(){}
  std::string getName() const { return "FiniteElement"; }
  std::string getHelp() const;
  int getNbOptions() const;
  StringXNumber* getOption(int iopt);  
  int getNbOptionsStr() const;
  StringXString* getOptionStr(int iopt);  
  PView *execute(PView *);
};

#endif
