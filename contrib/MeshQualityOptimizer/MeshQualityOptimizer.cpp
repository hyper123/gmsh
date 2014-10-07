// TODO: Copyright

//#include "GModel.h"
#include "GEntity.h"
#include "MElement.h"
#include "MTriangle.h"
#include "MQuadrangle.h"
#include "qualityMeasures.h"
#include "MeshOptCommon.h"
#include "MeshOptObjContribFunc.h"
#include "MeshOptObjContrib.h"
#include "MeshOptObjContribScaledNodeDispSq.h"
#include "MeshQualityObjContribNCJ.h"
#include "MeshQualityObjContribInvCond.h"
#include "MeshOptimizer.h"
#include "MeshQualityOptimizer.h"


struct QualPatchDefParameters : public MeshOptParameters::PatchDefParameters
{
  QualPatchDefParameters(const MeshQualOptParameters &p);
  virtual ~QualPatchDefParameters() {}
  virtual double elBadness(MElement *el);
  virtual double maxDistance(MElement *el);
private:
//  double NCJMin, NCJMax;
  double invCondMin;
  double distanceFactor;
};


QualPatchDefParameters::QualPatchDefParameters(const MeshQualOptParameters &p)
{
//  NCJMin = p.minTargetNCJ;
//  NCJMax = (p.maxTargetNCJ > 0.) ? p.maxTargetNCJ : 1.e300;
  invCondMin = p.minTargetInvCond;
  strategy = (p.strategy == 1) ? MeshOptParameters::STRAT_ONEBYONE :
                                        MeshOptParameters::STRAT_CONNECTED;
  minLayers = (p.dim == 3) ? 1 : 0;
  maxLayers = p.nbLayers;
  distanceFactor = p.distanceFactor;
  if (strategy == MeshOptParameters::STRAT_CONNECTED)
    weakMerge = (p.strategy == 2);
  else {
    maxAdaptPatch = p.maxAdaptBlob;
    maxLayersAdaptFact = p.adaptBlobLayerFact;
    distanceAdaptFact = p.adaptBlobDistFact;
  }
}


double QualPatchDefParameters::elBadness(MElement *el) {
//  double valMin, valMax;
//  switch(el->getType()) {                                                 // TODO: Complete with other types?
//  case TYPE_TRI: {
//     qmTriangle::NCJRange(static_cast<MTriangle*>(el), valMin, valMax);
//    break;
//  }
//  case TYPE_QUA: {
//    qmQuadrangle::NCJRange(static_cast<MQuadrangle*>(el), valMin, valMax);
//    break;
//  }
//  }
  const JacobianBasis *jac = el->getJacobianFuncSpace();
  fullMatrix<double> nodesXYZ(el->getNumShapeFunctions(), 3);
  el->getNodesCoord(nodesXYZ);
  fullVector<double> invCond(jac->getNumJacNodes());
  jac->getInvCond(nodesXYZ, invCond);
  const double valMin = *std::min_element(invCond.getDataPtr(),
                          invCond.getDataPtr()+jac->getNumJacNodes());
  double badness = std::min(valMin-invCondMin, 0.);
  return badness;
}


double QualPatchDefParameters::maxDistance(MElement *el) {
  return distanceFactor * el->getOuterRadius();
}


void MeshQualityOptimizer(GModel *gm, MeshQualOptParameters &p)
{
  Msg::StatusBar(true, "Optimizing mesh quality...");

  MeshOptParameters par;
  par.dim = p.dim;
  par.onlyVisible = p.onlyVisible;
  par.fixBndNodes = p.fixBndNodes;
  QualPatchDefParameters patchDef(p);
  par.patchDef = &patchDef;
  par.optDisplay = 30;
  par.verbose = 4;

  ObjContribScaledNodeDispSq<ObjContribFuncSimple> nodeDistFunc(p.weightFixed, p.weightFree);
//  ObjContribNCJ<ObjContribFuncBarrierMovMin> minNCJBarFunc(1.);
//  minNCJBarFunc.setTarget(p.minTargetNCJ, 1.);
////  minNCJBarFunc.setTarget(p.minTargetNCJ, 0.866025404);
//  ObjContribNCJ<ObjContribFuncBarrierFixMinMovMax> minMaxNCJBarFunc(1.);
//  minMaxNCJBarFunc.setTarget(p.maxTargetNCJ, 1.);
  ObjContribInvCond<ObjContribFuncBarrierMovMin> minInvCondBarFunc(1.);
  minInvCondBarFunc.setTarget(p.minTargetInvCond, 1.);

  MeshOptParameters::PassParameters minJacPass;
  minJacPass.barrierIterMax = p.optPassMax;
  minJacPass.optIterMax = p.itMax;
  minJacPass.contrib.push_back(&nodeDistFunc);
//  minJacPass.contrib.push_back(&minNCJBarFunc);
  minJacPass.contrib.push_back(&minInvCondBarFunc);
  par.pass.push_back(minJacPass);

//  if (p.maxTargetNCJ > 0.) {
//    MeshOptParameters::PassParameters minMaxJacPass;
//    minMaxJacPass.barrierIterMax = p.optPassMax;
//    minMaxJacPass.optIterMax = p.itMax;
//    minMaxJacPass.contrib.push_back(&nodeDistFunc);
//    minMaxJacPass.contrib.push_back(&minMaxNCJBarFunc);
//    par.pass.push_back(minMaxJacPass);
//  }

  meshOptimizer(gm, par);

  p.CPU = par.CPU;
//  p.minNCJ = minMaxNCJBarFunc.getMin();
//  p.maxNCJ = minMaxNCJBarFunc.getMax();
  p.minInvCond = minInvCondBarFunc.getMin();
  p.maxInvCond = minInvCondBarFunc.getMax();

  Msg::StatusBar(true, "Done optimizing high order mesh (%g s)", p.CPU);
}