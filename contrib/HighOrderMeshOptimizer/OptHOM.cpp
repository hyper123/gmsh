// Copyright (C) 2013 ULg-UCL
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, and/or sell copies of the
// Software, and to permit persons to whom the Software is furnished
// to do so, provided that the above copyright notice(s) and this
// permission notice appear in all copies of the Software and that
// both the above copyright notice(s) and this permission notice
// appear in supporting documentation.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE
// COPYRIGHT HOLDER OR HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR
// ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY
// DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
// WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
// ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
// OF THIS SOFTWARE.
//
// Please report all bugs and problems to the public mailing list
// <gmsh@geuz.org>.
//
// Contributors: Thomas Toulorge, Jonathan Lambrechts

#include <iostream>
#include <algorithm>
#include "OptHomMesh.h"
#include "OptHOM.h"
#include "OptHomRun.h"
#include "GmshMessage.h"
#include "GmshConfig.h"

#if defined(HAVE_BFGS)

#include "ap.h"
#include "alglibinternal.h"
#include "alglibmisc.h"
#include "linalg.h"
#include "optimization.h"

static inline double compute_f(double v, double barrier)
{
  if ((v > barrier && barrier < 1) || (v < barrier && barrier > 1)) {
    const double l = log((v - barrier) / (1 - barrier));
    const double m = (v - 1);
    return l * l + m * m;
  }
  else return 1.e300;
  // if (v < 1.) return pow(1.-v,powM);
  // if (v < 1.) return exp((long double)pow(1.-v,3));
  // else return pow(v-1.,powP);
}

static inline double compute_f1(double v, double barrier)
{
  if ((v > barrier && barrier < 1) || (v < barrier && barrier > 1)) {
    return 2 * (v - 1) + 2 * log((v - barrier) / (1 - barrier)) / (v - barrier);
  }
  else return barrier < 1 ? -1.e300 : 1e300;
  // if (v < 1.) return -powM*pow(1.-v,powM-1.);
  // if (v < 1.) return -3.*pow(1.-v,2)*exp((long double)pow(1.-v,3));
  // else return powP*pow(v-1.,powP-1.);
}

OptHOM::OptHOM(const std::map<MElement*,GEntity*> &element2entity,
               const std::set<MElement*> &els, std::set<MVertex*> & toFix,
               bool fixBndNodes, bool fastJacEval) :
  mesh(element2entity, els, toFix, fixBndNodes, fastJacEval)
{
  _optimizeMetricMin = false;
}

// Contribution of the element Jacobians to the objective function value and
// gradients (2D version)
bool OptHOM::addJacObjGrad(double &Obj, alglib::real_1d_array &gradObj)
{
  minJac = 1.e300;
  maxJac = -1.e300;

  for (int iEl = 0; iEl < mesh.nEl(); iEl++) {
    // Scaled Jacobians
    std::vector<double> sJ(mesh.nBezEl(iEl));
    // Gradients of scaled Jacobians
    std::vector<double> gSJ(mesh.nBezEl(iEl)*mesh.nPCEl(iEl));
    mesh.scaledJacAndGradients (iEl,sJ,gSJ);

    for (int l = 0; l < mesh.nBezEl(iEl); l++) {
      double f1 = compute_f1(sJ[l], jacBar);
      Obj += compute_f(sJ[l], jacBar);
      if (_optimizeBarrierMax) {
        Obj += compute_f(sJ[l], barrier_min);
        f1 += compute_f1(sJ[l], barrier_min);
      }
      for (int iPC = 0; iPC < mesh.nPCEl(iEl); iPC++)
        gradObj[mesh.indPCEl(iEl,iPC)] += f1*gSJ[mesh.indGSJ(iEl,l,iPC)];
      minJac = std::min(minJac, sJ[l]);
      maxJac = std::max(maxJac, sJ[l]);
    }
  }

  return true;
}

bool OptHOM::addMetricMinObjGrad(double &Obj, alglib::real_1d_array &gradObj)
{

  minJac = 1.e300;
  maxJac = -1.e300;

  for (int iEl = 0; iEl < mesh.nEl(); iEl++) {
    // Scaled Jacobians
    std::vector<double> sJ(mesh.nBezEl(iEl));
    // Gradients of scaled Jacobians
    std::vector<double> gSJ(mesh.nBezEl(iEl)*mesh.nPCEl(iEl));
    mesh.metricMinAndGradients (iEl,sJ,gSJ);

    for (int l = 0; l < mesh.nBezEl(iEl); l++) {
      Obj += compute_f(sJ[l], jacBar);
      const double f1 = compute_f1(sJ[l], jacBar);
      for (int iPC = 0; iPC < mesh.nPCEl(iEl); iPC++)
        gradObj[mesh.indPCEl(iEl,iPC)] += f1*gSJ[mesh.indGSJ(iEl,l,iPC)];
      minJac = std::min(minJac, sJ[l]);
      maxJac = std::max(maxJac, sJ[l]);
    }
  }
  return true;
}

// Contribution of the vertex distance to the objective function value and
// gradients (2D version)
bool OptHOM::addDistObjGrad(double Fact, double Fact2, double &Obj,
                            alglib::real_1d_array &gradObj)
{
  maxDist = 0;
  avgDist = 0;
  int nbBnd = 0;

  for (int iFV = 0; iFV < mesh.nFV(); iFV++) {
    const double Factor = invLengthScaleSq*(mesh.forced(iFV) ? Fact : Fact2);
    const double dSq = mesh.distSq(iFV), dist = sqrt(dSq);
    Obj += Factor * dSq;
    std::vector<double> gDSq(mesh.nPCFV(iFV));
    mesh.gradDistSq(iFV,gDSq);
    for (int iPC = 0; iPC < mesh.nPCFV(iFV); iPC++)
      gradObj[mesh.indPCFV(iFV,iPC)] += Factor*gDSq[iPC];
    maxDist = std::max(maxDist, dist);
    avgDist += dist;
    nbBnd++;
  }
  if (nbBnd != 0) avgDist /= nbBnd;

  return true;
}

void OptHOM::evalObjGrad(const alglib::real_1d_array &x, double &Obj,
                         alglib::real_1d_array &gradObj)
{
  mesh.updateMesh(x.getcontent());

  Obj = 0.;
  for (int i = 0; i < gradObj.length(); i++) gradObj[i] = 0.;

  addJacObjGrad(Obj, gradObj);
  addDistObjGrad(lambda, lambda2, Obj, gradObj);
  if(_optimizeMetricMin)
    addMetricMinObjGrad(Obj, gradObj);
  if ((minJac > barrier_min) && (maxJac < barrier_max || !_optimizeBarrierMax)) {
    Msg::Info("Reached %s (%g %g) requirements, setting null gradient",
              _optimizeMetricMin ? "svd" : "jacobian", minJac, maxJac);
    Obj = 0.;
    for (int i = 0; i < gradObj.length(); i++) gradObj[i] = 0.;
  }
}

void evalObjGradFunc(const alglib::real_1d_array &x, double &Obj,
                     alglib::real_1d_array &gradObj, void *HOInst)
{
  (static_cast<OptHOM*>(HOInst))->evalObjGrad(x, Obj, gradObj);
}

void OptHOM::recalcJacDist()
{
  maxDist = 0;
  avgDist = 0;
  int nbBnd = 0;

  for (int iFV = 0; iFV < mesh.nFV(); iFV++) {
    if (mesh.forced(iFV)) {
      double dSq = mesh.distSq(iFV);
      maxDist = std::max(maxDist, sqrt(dSq));
      avgDist += sqrt(dSq);
      nbBnd++;
    }
  }
  if (nbBnd != 0) avgDist /= nbBnd;

  minJac = 1.e300;
  maxJac = -1.e300;
  for (int iEl = 0; iEl < mesh.nEl(); iEl++) {
    // Scaled Jacobians
    std::vector<double> sJ(mesh.nBezEl(iEl));
    // (Dummy) gradients of scaled Jacobians
    std::vector<double> dumGSJ(mesh.nBezEl(iEl)*mesh.nPCEl(iEl));
    mesh.scaledJacAndGradients (iEl,sJ,dumGSJ);
    if(_optimizeMetricMin)
      mesh.metricMinAndGradients (iEl,sJ,dumGSJ);
    for (int l = 0; l < mesh.nBezEl(iEl); l++) {
      minJac = std::min(minJac, sJ[l]);
      maxJac = std::max(maxJac, sJ[l]);
    }
  }
}

void OptHOM::printProgress(const alglib::real_1d_array &x, double Obj)
{
  iter++;

  if (iter % progressInterv == 0) {
    Msg::Info("--> Iteration %3d --- OBJ %12.5E (relative decrease = %12.5E) "
              "-- minJ = %12.5E  maxJ = %12.5E Max D = %12.5E Avg D = %12.5E",
              iter, Obj, Obj/initObj, minJac, maxJac, maxDist, avgDist);
  }
}

void printProgressFunc(const alglib::real_1d_array &x, double Obj, void *HOInst)
{
  ((OptHOM*)HOInst)->printProgress(x,Obj);
}

void OptHOM::calcScale(alglib::real_1d_array &scale)
{
  scale.setlength(mesh.nPC());

  // Calculate scale
  for (int iFV = 0; iFV < mesh.nFV(); iFV++) {
    std::vector<double> scaleFV(mesh.nPCFV(iFV),1.);
    mesh.pcScale(iFV,scaleFV);
    for (int iPC = 0; iPC < mesh.nPCFV(iFV); iPC++)
      scale[mesh.indPCFV(iFV,iPC)] = scaleFV[iPC];
  }
}

void OptHOM::OptimPass(alglib::real_1d_array &x,
                       const alglib::real_1d_array &initGradObj, int itMax)
{
  static const double EPSG = 0.;
  static const double EPSF = 0.;
  static const double EPSX = 0.;
  static int OPTMETHOD = 1;

  Msg::Info("--- Optimization pass with initial jac. range (%g, %g), jacBar = %g",
            minJac, maxJac, jacBar);

  iter = 0;

  alglib::real_1d_array scale;
  calcScale(scale);

  int iterationscount = 0, nfev = 0, terminationtype = -1;
  if (OPTMETHOD == 1) {
    alglib::mincgstate state;
    alglib::mincgreport rep;
    try{
      mincgcreate(x, state);
      mincgsetscale(state,scale);
      mincgsetprecscale(state);
      mincgsetcond(state, EPSG, EPSF, EPSX, itMax);
      mincgsetxrep(state, true);
      alglib::mincgoptimize(state, evalObjGradFunc, printProgressFunc, this);
      mincgresults(state, x, rep);
    }
    catch(alglib::ap_error e){
      Msg::Error("%s", e.msg.c_str());
    }
    iterationscount = rep.iterationscount;
    nfev = rep.nfev;
    terminationtype = rep.terminationtype;
  }
  else {
    alglib::minlbfgsstate state;
    alglib::minlbfgsreport rep;
    try{
      minlbfgscreate(3, x, state);
      minlbfgssetscale(state,scale);
      minlbfgssetprecscale(state);
      minlbfgssetcond(state, EPSG, EPSF, EPSX, itMax);
      minlbfgssetxrep(state, true);
      alglib::minlbfgsoptimize(state, evalObjGradFunc, printProgressFunc, this);
      minlbfgsresults(state, x, rep);
    }
    catch(alglib::ap_error e){
      Msg::Error("%s", e.msg.c_str());
    }
    iterationscount = rep.iterationscount;
    nfev = rep.nfev;
    terminationtype = rep.terminationtype;
  }

  Msg::Info("Optimization finalized after %d iterations (%d function evaluations),",
            iterationscount, nfev);
  switch(int(terminationtype)) {
  case 1: Msg::Info("because relative function improvement is no more than EpsF"); break;
  case 2: Msg::Info("because relative step is no more than EpsX"); break;
  case 4: Msg::Info("because gradient norm is no more than EpsG"); break;
  case 5: Msg::Info("because the maximum number of steps was taken"); break;
  default: Msg::Info("with code %d", int(terminationtype)); break;
  }
}

int OptHOM::optimize(double weightFixed, double weightFree, double b_min,
                     double b_max, bool optimizeMetricMin, int pInt,
                     int itMax, int optPassMax)
{
  barrier_min = b_min;
  barrier_max = b_max;
  progressInterv = pInt;
//  powM = 4;
//  powP = 3;

  _optimizeMetricMin = optimizeMetricMin;
  // Set weights & length scale for non-dimensionalization
  lambda = weightFixed;
  lambda2 = weightFree;
  std::vector<double> dSq(mesh.nEl());
  mesh.distSqToStraight(dSq);
  const double maxDSq = *max_element(dSq.begin(),dSq.end());
  if (maxDSq < 1.e-10) {                                        // Length scale for non-dim. distance
    std::vector<double> sSq(mesh.nEl());
    mesh.elSizeSq(sSq);
    const double maxSSq = *max_element(sSq.begin(),sSq.end());
    invLengthScaleSq = 1./maxSSq;
  }
  else invLengthScaleSq = 1./maxDSq;

  // Set initial guess
  alglib::real_1d_array x;
  x.setlength(mesh.nPC());
  mesh.getUvw(x.getcontent());

  // Calculate initial performance
  recalcJacDist();
  initMaxDist = maxDist;
  initAvgDist = avgDist;

  const double jacBarStart = (minJac > 0.) ? 0.9*minJac : 1.1*minJac;
  jacBar = jacBarStart;

  _optimizeBarrierMax = false;
  // Calculate initial objective function value and gradient
  initObj = 0.;
  alglib::real_1d_array gradObj;
  gradObj.setlength(mesh.nPC());
  for (int i = 0; i < mesh.nPC(); i++) gradObj[i] = 0.;
  evalObjGrad(x, initObj, gradObj);

  Msg::Info("Start optimizing %d elements (%d vertices, %d free vertices, %d variables) "
            "with min barrier %g and max. barrier %g", mesh.nEl(), mesh.nVert(),
            mesh.nFV(), mesh.nPC(), barrier_min, barrier_max);

  int ITER = 0;
  bool minJacOK = true;
  while (minJac < barrier_min) {
    const double startMinJac = minJac;
    OptimPass(x, gradObj, itMax);
    recalcJacDist();
    jacBar = (minJac > 0.) ? 0.9*minJac : 1.1*minJac;
    if (ITER ++ > optPassMax) {
      minJacOK = (minJac > barrier_min);
      break;
    }
    if (fabs((minJac-startMinJac)/startMinJac) < 0.01) {
      Msg::Info("Stagnation in minJac detected, stopping optimization");
      minJacOK = false;
      break;
    }
  }

  ITER = 0;
  if (minJacOK && (!_optimizeMetricMin)) {
    _optimizeBarrierMax = true;
    jacBar =  1.1 * maxJac;
    while (maxJac > barrier_max ) {
      const double startMaxJac = maxJac;
      OptimPass(x, gradObj, itMax);
      recalcJacDist();
      jacBar =  1.1 * maxJac;
      if (ITER ++ > optPassMax) break;
      if (fabs((maxJac-startMaxJac)/startMaxJac) < 0.01) {
        Msg::Info("Stagnation in maxJac detected, stopping optimization");
        break;
      }
    }
  }

  Msg::Info("Optimization done Range (%g,%g)", minJac, maxJac);

  if (minJac > barrier_min && maxJac < barrier_max) return 1;
  if (minJac > 0.0) return 0;
  return -1;
}

#endif
