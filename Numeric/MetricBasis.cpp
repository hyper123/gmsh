// Gmsh - Copyright (C) 1997-2013 C. Geuzaine, J.-F. Remacle
//
// See the LICENSE.txt file for license information. Please report all
// bugs and problems to the public mailing list <gmsh@geuz.org>.

#include "MetricBasis.h"
#include "BasisFactory.h"
#include "pointsGenerators.h"
#include "BasisFactory.h"
#include <queue>
#include "OS.h"
#include <sstream>

double MetricBasis::_tol = 1e-2;
int MetricBasis::_which = 0;

namespace {
  double cubicCardanoRoot(double p, double q)
  {
    double A = q*q/4 + p*p*p/27;
    if (A > 0) {
      double sq = std::sqrt(A);
      return std::pow(-q/2+sq, 1/3.) + std::pow(-q/2-sq, 1/3.);
    }
    else {
      double module = std::sqrt(-p*p*p/27);
      double ang = std::acos(-q/2/module);
      return 2 * std::pow(module, 1/3.) * std::cos(ang/3);
    }
  }

  int nChoosek(int n, int k)
  {
    if (n < k || k < 0) {
      Msg::Error("Wrong argument for combination. n %d k %d", n, k);
      return 1;
    }

    if (k > n/2) k = n-k;
    if (k == 1)
      return n;
    if (k == 0)
      return 1;

    int c = 1;
    for (int i = 1; i <= k; i++, n--) (c *= n) /= i;
    return c;
  }
}

MetricBasis::MetricBasis(int tag)
{
  const int type = ElementType::ParentTypeFromTag(tag);
  const int metOrder = metricOrder(tag);
  if (type == TYPE_HEX || type == TYPE_PRI) {
    int order = ElementType::OrderFromTag(tag);
    _jacobian = new JacobianBasis(tag, 3*order);
  }
  else if (type == TYPE_TET)
    _jacobian = BasisFactory::getJacobianBasis(tag);
  else
    Msg::Fatal("metric not implemented for element tag %d", tag);
  _gradients = BasisFactory::getGradientBasis(tag, metOrder);
  _bezier = BasisFactory::getBezierBasis(type, metOrder);

  _fillInequalities(metOrder);
  __TotSubdivision = 0;
}

double MetricBasis::boundMinR(MElement *el)
{
  MetricBasis *metric = (MetricBasis*)BasisFactory::getMetricBasis(el->getTypeForMSH());
  MetricData *md = NULL;
  fullMatrix<double> dummy;
  return metric->getBoundRmin(el, md, dummy);
}

double MetricBasis::sampleR(MElement *el, int order)
{
  MetricBasis *metric = (MetricBasis*)BasisFactory::getMetricBasis(el->getTypeForMSH());
  MetricData *md = NULL;
  fullMatrix<double> dummy;
  return metric->getMinR(el, md, order);
}

double MetricBasis::getMinR(MElement *el, MetricData *&md, int deg) const
{
  fullMatrix<double> samplingPoints;

  switch (el->getType()) {
    case TYPE_PNT :
      samplingPoints = gmshGeneratePointsLine(0);
      break;
    case TYPE_LIN :
      samplingPoints = gmshGeneratePointsLine(deg);
      break;
    case TYPE_TRI :
      samplingPoints = gmshGeneratePointsTriangle(deg,false);
      break;
    case TYPE_QUA :
      samplingPoints = gmshGeneratePointsQuadrangle(deg,false);
      break;
    case TYPE_TET :
      samplingPoints = gmshGeneratePointsTetrahedron(deg,false);
      break;
    case TYPE_PRI :
      samplingPoints = gmshGeneratePointsPrism(deg,false);
      break;
    case TYPE_HEX :
      samplingPoints = gmshGeneratePointsHexahedron(deg,false);
      break;
    case TYPE_PYR :
      samplingPoints = JacobianBasis::generateJacPointsPyramid(deg);
      break;
    default :
      Msg::Error("Unknown Jacobian function space for element type %d", el->getType());
      return -1;
  }

  if (!md) _getMetricData(el, md);

  static unsigned int aa = 200;
  bool write = false;
  if (md->_num < 100000 && ++aa < 200) {
    write = true;
    std::stringstream name;
    name << "HoleMetric_" << el->getNum() << "_";
    name << (md->_num % 10);
    name << (md->_num % 100)/10;
    name << (md->_num % 1000)/100;
    name << (md->_num % 10000)/1000;
    name << (md->_num % 100000)/10000;
    name << ".txt";
    ((MetricBasis*)this)->file.open(name.str().c_str(), std::fstream::out);

    {
      fullMatrix<double> *coeff = md->_metcoeffs;
      fullVector<double> *jac = md->_jaccoeffs;
      double minp, minpp, maxp, minJ2, maxJ2, minK, mina, maxa, beta, minq, maxq, maxa2, maxa3, maxK2, maxK3, RminBez, RminLag;
      minp = _minp(*coeff);
      minpp = _minp2(*coeff);
      maxp = _maxp(*coeff);
      minq = _minq(*coeff);
      maxq = _maxq(*coeff);
      _minMaxJacobianSqr(*jac, minJ2, maxJ2);
      _minJ2P3(*coeff, *jac, minK);
      _minMaxA(*coeff, mina, maxa);

      double phip, term1, dRda;
      _computeTermBeta(mina, minK, dRda, term1, phip);
      beta = -3 * mina*mina * term1 / dRda / 6;
      if (beta*minK-mina*mina*mina < 0) {
        _maxAstKneg(*coeff, *jac, minK, beta, maxa3);
        _maxAstKpos(*coeff, *jac, minK, beta, maxa2);
      }
      else {
        _maxAstKpos(*coeff, *jac, minK, beta, maxa3);
        _maxAstKneg(*coeff, *jac, minK, beta, maxa2);
        if (beta*minK-maxa3*maxa3*maxa3 < 0) {
          _maxAstKneg(*coeff, *jac, minK, beta, maxa3);
          _maxAstKpos(*coeff, *jac, minK, beta, maxa2);
        }
      }
      _maxKstAfast(*coeff, *jac, mina, beta, maxK2);
      _maxKstAsharp(*coeff, *jac, mina, beta, maxK3);

      /*if (md->_num == 22)
        _computeRmin(*coeff, *jac, RminLag, RminBez, 0, true);
      else*/
      _computeRmin(*coeff, *jac, RminLag, RminBez, 0, false);

      double betaOpt = beta, minaOpt = mina, maxaOpt = maxa3, RminBezOpt;
      {
        /*const */double phi = std::acos(.5*(minK-maxa3*maxa3*maxa3+3*maxa3))/3;
        RminBezOpt = (maxa3+2*std::cos(phi+2*M_PI/3))/(maxa3+2*std::cos(phi));
        RminBezOpt = std::sqrt(RminBezOpt);

        double RminBez0 = (mina+2*std::cos(phip+M_PI/3))/(mina+2*std::cos(phip-M_PI/3));
        RminBez0 = std::sqrt(RminBez0);
        double curmina = mina;
        double curmaxa = maxa3;
        while (std::min(RminLag, RminBez0)-RminBezOpt > MetricBasis::_tol) {
          minaOpt = (curmina + curmaxa) / 2;
          maxaOpt = curmina;
          while (maxaOpt < minaOpt) {
            _computeTermBeta(minaOpt, minK, dRda, term1, phip);
            betaOpt = -3 * minaOpt*minaOpt * term1 / dRda / 6;
            if (betaOpt*minK-minaOpt*minaOpt*minaOpt < 0)
              _maxAstKneg(*coeff, *jac, minK, betaOpt, maxaOpt);
            else {
              _maxAstKpos(*coeff, *jac, minK, betaOpt, maxaOpt);
              if (betaOpt*minK-maxaOpt*maxaOpt*maxaOpt < 0)
                _maxAstKneg(*coeff, *jac, minK, betaOpt, maxaOpt);
            }
            minaOpt = (curmina + minaOpt) / 2;
          }
          curmina = minaOpt;
          curmaxa = maxaOpt;
          phi = std::acos(.5*(minK-curmaxa*curmaxa*curmaxa+3*curmaxa))/3;
          RminBezOpt = (curmaxa+2*std::cos(phi+2*M_PI/3))/(curmaxa+2*std::cos(phi));
          phi = std::acos(.5*(minK-curmina*curmina*curmina+3*curmina))/3;
          RminBez0 = (curmina+2*std::cos(phi+2*M_PI/3))/(curmina+2*std::cos(phi));
          RminBezOpt = std::sqrt(RminBezOpt);
          RminBez0 = std::sqrt(RminBez0);
        }
      }

      ((MetricBasis*)this)->file << minK << " ";
      ((MetricBasis*)this)->file << maxJ2/minpp/minpp/minpp << " ";
      ((MetricBasis*)this)->file << mina << " " << maxa << " ";
      ((MetricBasis*)this)->file << beta << " ";
      ((MetricBasis*)this)->file << minp << " " << maxp << " ";
      ((MetricBasis*)this)->file << minJ2 << " " << maxJ2 << " ";
      ((MetricBasis*)this)->file << minpp << " ";
      ((MetricBasis*)this)->file << minq << " " << maxq << " ";
      ((MetricBasis*)this)->file << maxa2 << " ";
      ((MetricBasis*)this)->file << maxa3 << " ";
      ((MetricBasis*)this)->file << maxK2 << " ";
      ((MetricBasis*)this)->file << maxK3 << " ";
      ((MetricBasis*)this)->file << RminBez << " " << RminLag << " ";
      ((MetricBasis*)this)->file << betaOpt << " ";
      ((MetricBasis*)this)->file << minaOpt << " " << maxaOpt << std::endl;
    }
  }

  double uvw[3];
  double minmaxQ[2];
  uvw[0] = samplingPoints(0, 0);
  uvw[1] = samplingPoints(0, 1);
  uvw[2] = samplingPoints(0, 2);

  interpolate(el, md, uvw, minmaxQ, write);
  double min, max = min = std::sqrt(minmaxQ[0]/minmaxQ[1]);
  for (int i = 1; i < samplingPoints.size1(); ++i) {
    uvw[0] = samplingPoints(i, 0);
    uvw[1] = samplingPoints(i, 1);
    uvw[2] = samplingPoints(i, 2);
    interpolate(el, md, uvw, minmaxQ, write);
    double tmp = std::sqrt(minmaxQ[0]/minmaxQ[1]);
    min = std::min(min, tmp);
    max = std::max(max, tmp);
    //Msg::Info("%g (%g, %g)", tmp, min, max);
  }
  if (write) {
    ((MetricBasis*)this)->file.close();
  }
  return min;
}

bool MetricBasis::notStraight(MElement *el, double &metric, int deg) const
{
  fullMatrix<double> samplingPoints;

  switch (el->getType()) {
    case TYPE_PNT :
      samplingPoints = gmshGeneratePointsLine(0);
      break;
    case TYPE_LIN :
      samplingPoints = gmshGeneratePointsLine(deg);
      break;
    case TYPE_TRI :
      samplingPoints = gmshGeneratePointsTriangle(deg,false);
      break;
    case TYPE_QUA :
      samplingPoints = gmshGeneratePointsQuadrangle(deg,false);
      break;
    case TYPE_TET :
      samplingPoints = gmshGeneratePointsTetrahedron(deg,false);
      break;
    case TYPE_PRI :
      samplingPoints = gmshGeneratePointsPrism(deg,false);
      break;
    case TYPE_HEX :
      samplingPoints = gmshGeneratePointsHexahedron(deg,false);
      break;
    case TYPE_PYR :
      samplingPoints = JacobianBasis::generateJacPointsPyramid(deg);
      break;
    default :
      Msg::Error("Unknown Jacobian function space for element type %d", el->getType());
      return false;
  }

  MetricData *md;
  _getMetricData(el, md);

  double uvw[3];
  double minmaxQ[2];
  uvw[0] = samplingPoints(0, 0);
  uvw[1] = samplingPoints(0, 1);
  uvw[2] = samplingPoints(0, 2);

  interpolate(el, md, uvw, minmaxQ);
  double min, max = min = std::sqrt(minmaxQ[0]/minmaxQ[1]);
  for (int i = 1; i < samplingPoints.size1(); ++i) {
    uvw[0] = samplingPoints(i, 0);
    uvw[1] = samplingPoints(i, 1);
    uvw[2] = samplingPoints(i, 2);
    interpolate(el, md, uvw, minmaxQ);
    double tmp = std::sqrt(minmaxQ[0]/minmaxQ[1]);
    min = std::min(min, tmp);
    max = std::max(max, tmp);
    //Msg::Info("%g (%g, %g)", tmp, min, max);
  }

  if (max-min < max*1e-12) {
    metric = min;
    return false;
  }
  else {
    metric = -1;
    return true;
  }
}

double MetricBasis::getBoundRmin(MElement *el, MetricData *&md, fullMatrix<double> &lagCoeff)
{
  __curElem = el;
  int nSampPnts = _gradients->getNumSamplingPoints();
  int nMapping = _gradients->getNumMapNodes();
  fullMatrix<double> nodes(nMapping, 3);
  el->getNodesCoord(nodes);

  // Metric coefficients
  fullMatrix<double> metCoeffLag;

  switch (el->getDim()) {
  case 0 :
    return -1.;
  case 1 :
  case 2 :
    Msg::Fatal("not implemented");
    break;

  case 3 :
    {
      fullMatrix<double> dxyzdX(nSampPnts,3), dxyzdY(nSampPnts,3), dxyzdZ(nSampPnts,3);
      _gradients->getGradientsFromNodes(nodes, &dxyzdX, &dxyzdY, &dxyzdZ);

      metCoeffLag.resize(nSampPnts, 7);
      for (int i = 0; i < nSampPnts; i++) {
        const double &dxdX = dxyzdX(i,0), &dydX = dxyzdX(i,1), &dzdX = dxyzdX(i,2);
        const double &dxdY = dxyzdY(i,0), &dydY = dxyzdY(i,1), &dzdY = dxyzdY(i,2);
        const double &dxdZ = dxyzdZ(i,0), &dydZ = dxyzdZ(i,1), &dzdZ = dxyzdZ(i,2);
        const double dvxdX = dxdX*dxdX + dydX*dydX + dzdX*dzdX;
        const double dvxdY = dxdY*dxdY + dydY*dydY + dzdY*dzdY;
        const double dvxdZ = dxdZ*dxdZ + dydZ*dydZ + dzdZ*dzdZ;
        metCoeffLag(i, 0) = (dvxdX + dvxdY + dvxdZ) / 3;
        metCoeffLag(i, 1) = dvxdX - metCoeffLag(i, 0);
        metCoeffLag(i, 2) = dvxdY - metCoeffLag(i, 0);
        metCoeffLag(i, 3) = dvxdZ - metCoeffLag(i, 0);
        const double fact = std::sqrt(2);
        metCoeffLag(i, 4) = fact * (dxdX*dxdY + dydX*dydY + dzdX*dzdY);
        metCoeffLag(i, 5) = fact * (dxdZ*dxdY + dydZ*dydY + dzdZ*dzdY);
        metCoeffLag(i, 6) = fact * (dxdX*dxdZ + dydX*dydZ + dzdX*dzdZ);
      }
    }
    break;
  }

  lagCoeff = metCoeffLag;
  fullMatrix<double> *metCoeff;
  metCoeff = new fullMatrix<double>(nSampPnts, metCoeffLag.size2());
  _bezier->matrixLag2Bez.mult(metCoeffLag, *metCoeff);

  // Jacobian coefficients
  fullVector<double> jacLag(_jacobian->getNumJacNodes());
  fullVector<double> *jac = new fullVector<double>(_jacobian->getNumJacNodes());
  _jacobian->getSignedJacobian(nodes, jacLag);
  _jacobian->lag2Bez(jacLag, *jac);

  //
  double RminLag, RminBez;

  /*Msg::Info("----------------");
  Msg::Info("Jacobian");
  for (int i = 0; i < jac->size(); ++i) {
    Msg::Info("%g", (*jac)(i));
  }
  Msg::Info("----------------");
  Msg::Info("Metric");
  for (int i = 0; i < metCoeff->size1(); ++i) {
    Msg::Info("%g %g %g %g %g %g %g", (*metCoeff)(i, 0), (*metCoeff)(i, 1), (*metCoeff)(i, 2), (*metCoeff)(i, 3), (*metCoeff)(i, 4), (*metCoeff)(i, 5), (*metCoeff)(i, 6));
  }
  Msg::Info("----------------");*/

  _computeRmin(*metCoeff, *jac, RminLag, RminBez, 0);
  //Msg::Info("el %d", el->getNum());
  double mina, maxa;
  _minMaxA(*metCoeff, mina, maxa);
  static int cntRight = 0, cntTOT = 0;
  ++cntTOT;
  if (maxa-mina < 1e-10) {
    ++cntRight;
  }
  //Msg::Info("right %d/%d", cntRight, cntTOT);


    fullVector<double> *jjac = new fullVector<double>(*jac);
    fullMatrix<double> *mmet = new fullMatrix<double>(*metCoeff);
    /*for (int i = 0; i < jjac->size(); ++i) {
      Msg::Info(":%g", (*jjac)(i));
    }
    for (int i = 0; i < mmet->size1(); ++i) {
      Msg::Info(":%g | %g %g %g %g %g %g", (*mmet)(i, 0), (*mmet)(i, 1), (*mmet)(i, 2), (*mmet)(i, 3), (*mmet)(i, 4), (*mmet)(i, 5), (*mmet)(i, 6));
    }*/
    md = new MetricData(mmet, jjac, RminBez, 0, 0);
      //Msg::Info("+1 %d", md);

  if (RminLag-RminBez < MetricBasis::_tol) {
    //Msg::Info("RETURNING %g", RminBez);
    //Msg::Info("0 subdivision");
    return RminBez;
  }
  else {
    //MetricData md(metCoeff, jac, RminBez, 0);
    MetricData *md2 = new MetricData(metCoeff, jac, RminBez, 0, 0);
      //Msg::Info("+2 %d", md2);
    ((MetricBasis*)this)->__numSubdivision = 0;
    ((MetricBasis*)this)->__numSub.resize(20);
    for (unsigned int i = 0; i < __numSub.size(); ++i) ((MetricBasis*)this)->__numSub[i] = 0;
    ((MetricBasis*)this)->__maxdepth = 0;
    //double time = Cpu();
    static int maxsub = 0, elmax;
    double tt = _subdivideForRmin(md2, RminLag, MetricBasis::_tol, MetricBasis::_which);
    if (maxsub < __numSubdivision && tt > 10-10) {
      maxsub = __numSubdivision;
      elmax = el->getNum();
    }
    //Msg::Info("%d subdivisions (max %d, %d), el %d", __numSubdivision, maxsub, elmax, el->getNum());
    /*//Msg::Info("> computation time %g", Cpu() - time);
    Msg::Info("> maxDepth %d", __maxdepth);
    Msg::Info("> numSubdivision %d", __numSubdivision);
    int last = __numSub.size();
    while (--last > 0 && __numSub[last] == 0);
    for (unsigned int i = 0; i < last+1; ++i) {
      Msg::Info("> depth %d: %d", i, __numSub[i]);
    }
    Msg::Info("RETURNING %g after subdivision", tt);*/
    return tt;
  }
}

void MetricBasis::_fillInequalities(int metricOrder)
{
  int dimSimplex = _bezier->_dimSimplex;
  int dim = _bezier->getDim();
  fullMatrix<int> exp(_bezier->_exponents.size1(), _bezier->_exponents.size2());
  for (int i = 0; i < _bezier->_exponents.size1(); ++i) {
    for (int j = 0; j < _bezier->_exponents.size2(); ++j) {
      exp(i, j) = static_cast<int>(_bezier->_exponents(i, j) + .5);
    }
  }
  int ncoeff = _gradients->getNumSamplingPoints();

  int countP3 = 0, countJ2 = 0, countA = 0;
  for (int i = 0; i < ncoeff; i++) {
    for (int j = i; j < ncoeff; j++) {
      double num = 1, den = 1;
      {
        int compl1 = metricOrder;
        int compl2 = metricOrder;
        int compltot = 2*metricOrder;
        for (int k = 0; k < dimSimplex; k++) {
          num *= nChoosek(compl1, exp(i, k))
               * nChoosek(compl2, exp(j, k));
          den *= nChoosek(compltot, exp(i, k) + exp(j, k));
          compl1 -= exp(i, k);
          compl2 -= exp(j, k);
          compltot -= exp(i, k) + exp(j, k);
        }
        for (int k = dimSimplex; k < dim; k++) {
          num *= nChoosek(metricOrder, exp(i, k))
               * nChoosek(metricOrder, exp(j, k));
          den *= nChoosek(2*metricOrder, exp(i, k) + exp(j, k));
        }
      }

      if (i != j) num *= 2;

      ++countA;
      int hash = 0;
      for (int l = 0; l < dim; l++) {
        hash += (exp(i, l)+exp(j, l)) * pow_int(2*metricOrder+1, l);
      }
      _ineqA[hash].push_back(IneqData(num/den, i, j));

      for (int k = j; k < ncoeff; ++k) {
        double num = 1, den = 1;
        {
          int compl1 = metricOrder;
          int compl2 = metricOrder;
          int compl3 = metricOrder;
          int compltot = 3*metricOrder;
          for (int l = 0; l < dimSimplex; l++) {
            num *= nChoosek(compl1, exp(i, l))
                 * nChoosek(compl2, exp(j, l))
                 * nChoosek(compl3, exp(k, l));
            den *= nChoosek(compltot, exp(i, l) + exp(j, l) + exp(k, l));
            compl1 -= exp(i, l);
            compl2 -= exp(j, l);
            compl3 -= exp(k, l);
            compltot -= exp(i, l) + exp(j, l) + exp(k, l);
          }
          for (int l = dimSimplex; l < dim; l++) {
            num *= nChoosek(metricOrder, exp(i, l))
                 * nChoosek(metricOrder, exp(j, l))
                 * nChoosek(metricOrder, exp(k, l));
            den *= nChoosek(3*metricOrder, exp(i, l) + exp(j, l) + exp(k, l));
          }
        }

        if (i == j) {
          if (j != k) num *= 3;
        }
        else {
          if (j == k || i == k) {
            num *= 3;
          }
          else num *= 6;
        }

        ++countP3;
        int hash = 0;
        for (int l = 0; l < dim; l++) {
          hash += (exp(i, l)+exp(j, l)+exp(k, l)) * pow_int(3*metricOrder+1, l);
        }
        if (j == k && j != i)
          _ineqP3[hash].push_back(IneqData(num/den, k, j, i));
        else
          _ineqP3[hash].push_back(IneqData(num/den, i, j, k));
      }
    }
  }

  exp.resize(_jacobian->bezier->_exponents.size1(),
             _jacobian->bezier->_exponents.size2());
  for (int i = 0; i < _jacobian->bezier->_exponents.size1(); ++i) {
    for (int j = 0; j < _jacobian->bezier->_exponents.size2(); ++j) {
      exp(i, j) = static_cast<int>(_jacobian->bezier->_exponents(i, j) + .5);
    }
  }
  int njac = _jacobian->getNumJacNodes();
  for (int i = 0; i < njac; i++) {
    for (int j = i; j < njac; j++) {
      int order = metricOrder/2*3;
      double num = 1, den = 1;
      {
        int compl1 = order;
        int compl2 = order;
        int compltot = 2*order;
        for (int k = 0; k < dimSimplex; k++) {
          num *= nChoosek(compl1, exp(i, k))
               * nChoosek(compl2, exp(j, k));
          den *= nChoosek(compltot, exp(i, k) + exp(j, k));
          compl1 -= exp(i, k);
          compl2 -= exp(j, k);
          compltot -= exp(i, k) + exp(j, k);
        }
      }
      for (int k = dimSimplex; k < dim; k++) {
        num *= nChoosek(order, exp(i, k))
             * nChoosek(order, exp(j, k));
        den *= nChoosek(2*order, exp(i, k) + exp(j, k));
      }

      if (i != j) num *= 2;

      ++countJ2;
      int hash = 0;
      for (int k = 0; k < dim; k++) {
        hash += (exp(i, k)+exp(j, k)) * pow_int(2*order+1, k);
      }
      _ineqJ2[hash].push_back(IneqData(num/den, i, j));
    }
  }

  _lightenInequalities(countJ2, countP3, countA);

  /*Msg::Info("A : %d / %d", countA, ncoeff*(ncoeff+1)/2);
  Msg::Info("J2 : %d / %d", countJ2, njac*(njac+1)/2);
  Msg::Info("P3 : %d / %d", countP3, ncoeff*(ncoeff+1)*(ncoeff+2)/6);*/
}

void MetricBasis::_lightenInequalities(int &countj, int &countp, int &counta)
{
  double tol = .0;
  std::map<int, std::vector<IneqData> >::iterator it, itbeg[3], itend[3];

  int cnt[3] = {0,0,0};
  itbeg[0] = _ineqJ2.begin();
  itbeg[1] = _ineqP3.begin();
  itbeg[2] = _ineqA.begin();
  itend[0] = _ineqJ2.end();
  itend[1] = _ineqP3.end();
  itend[2] = _ineqA.end();
  for (int k = 0; k < 3; ++k) {
    it = itbeg[k];
    while (it != itend[k]) {
      std::sort(it->second.begin(), it->second.end(), gterIneq());

      double rmved = .0;
      while (it->second.size() && rmved + it->second.back().val <= tol) {
        rmved += it->second.back().val;
        it->second.pop_back();
        ++cnt[k];
      }
      const double factor = 1-rmved;
      for (unsigned int i = 0; i < it->second.size(); ++i) {
        it->second[i].val /= factor;
      }
      ++it;
    }
  }
  countj -= cnt[0];
  countp -= cnt[1];
  counta -= cnt[2];
}

void MetricBasis::interpolate(const MElement *el, const MetricData *md, const double *uvw, double *minmaxQ, bool write) const
{
  if (minmaxQ == NULL) {
    Msg::Error("Cannot write solution of interpolation");
    return;
  }

  int order = _bezier->getOrder();

  int dimSimplex = 0;
  fullMatrix<double> exponents;
  double bezuvw[3];
  switch (el->getType()) {
  case TYPE_PYR:
    bezuvw[0] = .5 * (1 + uvw[0]);
    bezuvw[1] = .5 * (1 + uvw[1]);
    bezuvw[2] = uvw[2];
    //_interpolateBezierPyramid(uvw, minmaxQ);
    return;

  case TYPE_HEX:
    bezuvw[0] = .5 * (1 + uvw[0]);
    bezuvw[1] = .5 * (1 + uvw[1]);
    bezuvw[2] = .5 * (1 + uvw[2]);
    dimSimplex = 0;
    exponents = gmshGenerateMonomialsHexahedron(order);
    break;

  case TYPE_TET:
    bezuvw[0] = uvw[0];
    bezuvw[1] = uvw[1];
    bezuvw[2] = uvw[2];
    dimSimplex = 3;
    exponents = gmshGenerateMonomialsTetrahedron(order);
    break;

  case TYPE_PRI:
    bezuvw[0] = uvw[0];
    bezuvw[1] = uvw[1];
    bezuvw[2] = .5 * (1 + uvw[2]);
    dimSimplex = 2;
    exponents = gmshGenerateMonomialsPrism(order);
    break;
  }

  int numCoeff = exponents.size1();
  int dim = exponents.size2();

  fullMatrix<double> metcoeffs = *md->_metcoeffs;
  fullVector<double> jaccoeffs = *md->_jaccoeffs;

  double *terms = new double[metcoeffs.size2()];
  for (int t = 0; t < metcoeffs.size2(); ++t) {
    terms[t] = 0;
    for (int i = 0; i < numCoeff; i++) {
      double dd = 1;
      double pointCompl = 1.;
      int exponentCompl = order;
      for (int k = 0; k < dimSimplex; k++) {
        dd *= nChoosek(exponentCompl, (int) exponents(i, k))
          * pow(bezuvw[k], exponents(i, k));
        pointCompl -= bezuvw[k];
        exponentCompl -= (int) exponents(i, k);
      }
      dd *= pow(pointCompl, exponentCompl);

      for (int k = dimSimplex; k < dim; k++)
        dd *= nChoosek(order, (int) exponents(i, k))
            * pow(bezuvw[k], exponents(i, k))
            * pow(1. - bezuvw[k], order - exponents(i, k));
      terms[t] += metcoeffs(i, t) * dd;
    }
  }

  switch (metcoeffs.size2()) {
  case 1:
    minmaxQ[0] = terms[0];
    minmaxQ[1] = terms[0];
    break;

  case 3:
  {
    double tmp = pow(terms[1], 2);
    tmp += pow(terms[2], 2);
    tmp = std::sqrt(tmp);
    minmaxQ[0] = terms[0] - tmp;
    minmaxQ[1] = terms[0] + tmp;
  }
    break;

  case 7:
  {
    double tmp = pow(terms[1], 2);
    tmp += pow(terms[2], 2);
    tmp += pow(terms[3], 2);
    tmp += pow(terms[4], 2);
    tmp += pow(terms[5], 2);
    tmp += pow(terms[6], 2);
    tmp = std::sqrt(tmp);
    double factor = std::sqrt(6)/3;
    if (tmp < 1e-3*terms[0]) {
      minmaxQ[0] = terms[0] - factor * tmp;
      minmaxQ[1] = terms[0] + factor * tmp;
    }
    else {
      double phi;
      //{
        fullMatrix<double> nodes(1, 3);
        nodes(0, 0) = uvw[0];
        nodes(0, 1) = uvw[1];
        nodes(0, 2) = uvw[2];

        fullMatrix<double> result;
        _jacobian->interpolate(jaccoeffs, nodes, result, true);
        phi = result(0, 0)*result(0, 0);
      //}
      phi -= terms[0]*terms[0]*terms[0];
      phi += .5*terms[0]*tmp*tmp;
      phi /= tmp*tmp*tmp;
      phi *= 3*std::sqrt(6);
      if (phi >  1) phi =  1;
      if (phi < -1) phi = -1;
      phi = std::acos(phi)/3;
      minmaxQ[0] = terms[0] + factor * tmp * std::cos(phi + 2*M_PI/3);
      minmaxQ[1] = terms[0] + factor * tmp * std::cos(phi);
      ((MetricBasis*)this)->file << terms[0] << " " << tmp/std::sqrt(6) << " " << result(0, 0) << std::endl;
    }
  }
  break;

  default:
    Msg::Error("Wrong number of functions for metric: %d",
               metcoeffs.size2());
  }

  delete[] terms;
}

int MetricBasis::metricOrder(int tag)
{
  const int parentType = ElementType::ParentTypeFromTag(tag);
  const int order = ElementType::OrderFromTag(tag);

  switch (parentType) {
    case TYPE_PNT : return 0;

    case TYPE_LIN : return order;

    case TYPE_TRI :
    case TYPE_TET : return 2*order-2;

    case TYPE_QUA :
    case TYPE_PRI :
    case TYPE_HEX :
    case TYPE_PYR : return 2*order;
    default :
      Msg::Error("Unknown element type %d, return order 0", parentType);
      return 0;
  }
}

void MetricBasis::_computeRmin(
    const fullMatrix<double> &coeff, const fullVector<double> &jac,
    double &RminLag, double &RminBez,
    int depth, bool debug) const
{
  RminLag = 1.;

  for (int i = 0; i < _bezier->getNumLagCoeff(); ++i) {
    double q = coeff(i, 0);
    double p = 0;
    for (int k = 1; k < 7; ++k) {
      p += pow_int(coeff(i, k), 2);
    }
    p = std::sqrt(p/6);
    const double a = q/p;
    if (a > 1e4) {
      RminLag = std::min(RminLag, std::sqrt((a - std::sqrt(3)) / (a + std::sqrt(3))));
    }
    else {
      const double x = .5 * (jac(i)/p/p*jac(i)/p - a*a*a + 3*a);
      if (x >  1.1 || x < -1.1) {
        if (!depth) {
          Msg::Error("+ phi %g (jac %g, q %g, p %g)", x, jac(i), q, p);
          Msg::Info("%g + %g - %g = %g", jac(i)*jac(i)/p/p/p, .5 * q/p, q*q*q/p/p/p, (jac(i)*jac(i)+.5 * q*p*p-q*q*q)/p/p/p);
        }
        else if (depth == 1)
          Msg::Error("- phi %g @ %d(%d) (jac %g, q %g, p %g)", x, depth, i, jac(i), q, p);
      }

      double tmpR;
      if (x >=  1)
        tmpR = (a - 1) / (a + 2);
      else if (x <= -1)
        tmpR = (a - 2) / (a + 1);
      else {
        const double phi = std::acos(x)/3;
        tmpR = (a + 2*std::cos(phi + 2*M_PI/3)) / (a + 2*std::cos(phi));
      }
      if (tmpR < 0) {
        if (tmpR < -1e-7) Msg::Fatal("3 s normal ? %g (%g, %g, %g) or (%g, %g)",
            tmpR, p/std::sqrt(6), q, jac(i)*jac(i),
            q/p*std::sqrt(6), jac(i)*jac(i)/p/p/p*6*std::sqrt(6));
        else tmpR = 0;
      }
      RminLag = std::min(RminLag, std::sqrt(tmpR));
    }
  }

  //static int numtot = 0;
  //++numtot;
  double minK;
  _minJ2P3(coeff, jac, minK);
  if (minK < 1e-10) {
    RminBez = 0;
    return;
  }

  double mina, dummy;
  _minMaxA(coeff, mina, dummy);

  double term1, dRda, phip;
  _computeTermBeta(mina, minK, dRda, term1, phip);

  if (dRda < 0) {
    // TODO : better am ?
    double amApprox, da;
    {
      const double p = -3;
      double q = -minK - 2;
      const double a1 = cubicCardanoRoot(p, q);
      const double phim = std::acos(-1/a1) - M_PI/3;
      q = -minK + 2*std::cos(3*phim);
      amApprox = cubicCardanoRoot(p, q);
      if (minK < 10)
        da = -.3;
      else if (minK < 20)
        da = -.25;
      else if (minK < 35)
        da = -.2;
      else if (minK < 70)
        da = -.15;
      else if (minK < 175)
        da = -.1;
      else
        da = -.05;
    }

    double beta = -3 * mina*mina * term1 / dRda / 6;
    double maxa;
    if (beta*minK-mina*mina*mina < 0)
      _maxAstKneg(coeff, jac, minK, beta, maxa);
    else {
      _maxAstKpos(coeff, jac, minK, beta, maxa);
      if (maxa < amApprox && beta*minK-maxa*maxa*maxa < 0)
        _maxAstKneg(coeff, jac, minK, beta, maxa);
    }

    maxa = std::max(mina, maxa);
    if (amApprox*amApprox*amApprox+da < maxa*maxa*maxa) {
      // compute better am
      //
      double am0 = std::pow(amApprox*amApprox*amApprox+da, 1/3.);
      double am1 = std::pow(amApprox*amApprox*amApprox+da+.05, 1/3.);
      //double am0S = am0, am1S = am1;
      double am = (am0 + am1)/2;
      double R0 = _Rsafe(am0, minK);
      double R1 = _Rsafe(am1, minK);
      double Rnew = _Rsafe(am, minK);
      if (_chkaK(am0, minK)) Msg::Error("chk am0: %d (%g, %g)", _chkaK(am0, minK), am0, minK);
      if (_chkaK(am1, minK)) Msg::Error("chk am1: %d (%g, %g)", _chkaK(am1, minK), am1, minK);

      int cnt = 0;
      while (std::abs(R0-Rnew) > _tol*.01 || std::abs(R1-Rnew) > _tol*.01) {
        ++cnt;
        if (R0 > R1) {
          am0 = am;
          R0 = Rnew;
        }
        else {
          am1 = am;
          R1 = Rnew;
        }
        am = (am0 + am1)/2;
        Rnew = _Rsafe(am, minK);
      }
      /*static int maxcnt = 0, numcnt = 0, totcnt = 0;
      ++numcnt;
      totcnt += cnt;
      if (maxcnt < cnt) {
        maxcnt = cnt;
      }
      Msg::Info("maxcnt %d (num %d/%d=%g average %g)", maxcnt, numcnt, numtot, (double)numcnt/numtot, (double)totcnt/numcnt);
      double TESTdRda, dum0, dum1;
      _computeTermBeta(am0, minK, TESTdRda, dum0, dum1);
      if (TESTdRda > 1e12) Msg::Fatal("> 0 [%g %g %g] (%g, %g), %g -> [%g, %g] for el %d", R0, Rnew, R1, am, minK, amApprox, am0S, am1S, __curElem->getNum());
      _computeTermBeta(am1, minK, TESTdRda, dum0, dum1);
      if (TESTdRda < -1e12) Msg::Fatal("< 0 [%g %g %g] (%g, %g), %g -> [%g, %g] for el %d", R0, Rnew, R1, am, minK, amApprox, am0S, am1S, __curElem->getNum());*/
      if (am < maxa) {
        RminBez = _Rsafe(am, minK);
        //Msg::Info("cpt 1: %d (%g, %g, %g)", _chkaKR(am, minK, RminBez), am, minK, RminBez);
        if (_chkaKR(am, minK, RminBez)) Msg::Error("cpt 1: %d (%g, %g, %g)", _chkaKR(am, minK, RminBez), am, minK, RminBez);
        RminBez = std::sqrt(RminBez);
        return;
      }
    }

    RminBez = _Rsafe(maxa, minK);
    //Msg::Info("cpt 2: %d (%g, %g, %g)", _chkaKR(maxa, minK, RminBez), maxa, minK, RminBez);
    if (_chkaKR(maxa, minK, RminBez)) Msg::Error("cpt 2: %d (%g, %g, %g)", _chkaKR(maxa, minK, RminBez), maxa, minK, RminBez);
    RminBez = std::sqrt(RminBez);

    /*double RminBez0 = (mina+2*std::cos(phip+M_PI/3))/(mina+2*std::cos(phip-M_PI/3));
    RminBez0 = std::sqrt(RminBez0);
    double curmina = mina;
    double curmaxa = maxa;
      //Msg::Info(" ");
    while (std::min(RminLag, RminBez0)-RminBez > MetricBasis::_tol) {
      //Msg::Info("%g vs %g", RminBez0, RminBez);
      double a = (curmina + curmaxa) / 2;
      double newa = curmina;
      while (newa < a) {
        _computeTermBeta(a, minK, dRda, term1, phip, sqrt);
        beta = -3 * a*a * term1 / sqrt / dRda / 6;
        if (beta*minK-a*a*a < 0)
          _maxAstKneg(coeff, jac, minK, beta, newa);
        else {
          _maxAstKpos(coeff, jac, minK, beta, newa);
          if (newa < am && beta*minK-newa*newa*newa < 0)
            _maxAstKneg(coeff, jac, minK, beta, newa);
        }
        a = (curmina + a) / 2;
      }
      curmina = a;
      curmaxa = newa;
      phi = std::acos(.5*(minK-curmaxa*curmaxa*curmaxa+3*curmaxa))/3;
      RminBez = (curmaxa+2*std::cos(phi+2*M_PI/3))/(curmaxa+2*std::cos(phi));
      phi = std::acos(.5*(minK-curmina*curmina*curmina+3*curmina))/3;
      RminBez0 = (curmina+2*std::cos(phi+2*M_PI/3))/(curmina+2*std::cos(phi));
      RminBez = std::sqrt(RminBez);
      RminBez0 = std::sqrt(RminBez0);
    }*/
    return;
  }
  else if (term1 < 0) {
    double maxK;
    double beta = -3 * mina*mina * term1 / dRda / 6;
    if (beta*minK-mina*mina*mina > 0) Msg::Fatal("Arf pas prevu");
    //_maxKstAsharp(coeff, jac, mina, beta, maxK);
    _maxKstAfast(coeff, jac, mina, beta, maxK);
    const double x = .5*(maxK-mina*mina*mina+3*mina);
    const double phimin = std::acos(-1/mina) - M_PI/3;
    double myphi;
    int which = 0;
    double tmpphi;
    if (std::abs(x) > 1) {
      myphi = phimin;
      which = 2;
    }
    else {
      const double phimaxK = std::acos(x)/3;
      tmpphi = phimaxK;
      myphi = std::max(phimin, phimaxK);
      if (phimin > phimaxK)
        which = 2;
      else
        which = 1;
    }
    RminBez = (mina+2*std::cos(myphi+2*M_PI/3))/(mina+2*std::cos(myphi));
    //Msg::Info("cpt 3: %d", _chkaKR(mina, maxK, RminBez));
    int check;
    if (which == 1) {
      check = _chkaKR(mina, maxK, RminBez);
      if (check) {
        Msg::Error("cpt 3.1: %d (%g, %g, %g)", check, mina, maxK, RminBez);
        double Kphimin = 2 * std::cos(3*phimin) + mina*mina*mina - 3*mina;
        Msg::Info("%g->%g %g->%g", maxK, tmpphi/M_PI, Kphimin, phimin/M_PI);
      }
    }
    else {
      double Kphimin = 2 * std::cos(3*phimin) + mina*mina*mina - 3*mina;
      check = _chkaKR(mina, Kphimin, RminBez);
      if (check) Msg::Error("cpt 3.2: %d (%g, %g, %g)", check, mina, Kphimin, RminBez);
    }
    RminBez = std::sqrt(RminBez);
    return;
  }
  else {
    RminBez = (mina+2*std::cos(phip+M_PI/3))/(mina+2*std::cos(phip-M_PI/3));
    //Msg::Info("cpt 4: %d", _chkaKR(mina, minK, RminBez));
    if (_chkaKR(mina, minK, RminBez)) Msg::Error("cpt 4: %d (%g, %g, %g) dRda %g", _chkaKR(mina, minK, RminBez), mina, minK, RminBez, dRda);
    RminBez = std::sqrt(RminBez);
    return;
  }
}

void MetricBasis::_computeRmax(
    const fullMatrix<double> &coeff, const fullVector<double> &jac,
    double &RmaxLag) const
{
  RmaxLag = 0.;

  for (int i = 0; i < _bezier->getNumLagCoeff(); ++i) {
    double q = coeff(i, 0);
    double p = 0;
    for (int k = 1; k < 7; ++k) {
      p += pow_int(coeff(i, k), 2);
    }
    p = std::sqrt(p/6);
    const double a = q/p;
    if (a > 1e4) {
      RmaxLag = std::max(RmaxLag, std::sqrt((a - std::sqrt(3)) / (a + std::sqrt(3))));
    }
    else {
      const double x = .5 * (jac(i)/p/p*jac(i)/p - a*a*a + 3*a);

      double tmpR;
      if (x >=  1)
        tmpR = (a - 1) / (a + 2);
      else if (x <= -1)
        tmpR = (a - 2) / (a + 1);
      else {
        const double phi = std::acos(x)/3;
        tmpR = (a + 2*std::cos(phi + 2*M_PI/3)) / (a + 2*std::cos(phi));
      }
      if (tmpR < 0) {
        if (tmpR < -1e-7) Msg::Fatal("3 s normal ? %g (%g, %g, %g) or (%g, %g)",
            tmpR, p/std::sqrt(6), q, jac(i)*jac(i),
            q/p*std::sqrt(6), jac(i)*jac(i)/p/p/p*6*std::sqrt(6));
        else tmpR = 0;
      }
      RmaxLag = std::max(RmaxLag, std::sqrt(tmpR));
    }
  }
}

double MetricBasis::_subdivideForRmin(
    MetricData *md, double RminLag, double tol, int which) const
{
  std::priority_queue<MetricData*, std::vector<MetricData*>, lessMinB> subdomains;
  const int numCoeff = md->_metcoeffs->size2();
  const int numMetPnts = md->_metcoeffs->size1();
  const int numJacPnts = md->_jaccoeffs->size();
  const int numSub = _jacobian->getNumDivisions();
  subdomains.push(md);

  static unsigned int aa = 200;
  //bool write = false;
  if (++aa < 200) {
    getMinR(__curElem, md, 16);
  }

  std::vector<fullVector<double>*> trash;

  //Msg::Info("lagrange %g", RminLag);

  while (RminLag - subdomains.top()->_RminBez > tol && subdomains.size() < 25000) {
    //Msg::Info("%g - %g > %g && %d < %d", RminLag, subdomains.top()->_RminBez, tol, subdomains.size(), pow_int(8,8));
    fullMatrix<double> *subcoeffs, *coeff;
    fullVector<double> *subjac, *jac;

    MetricData *current = subdomains.top();
    subcoeffs = new fullMatrix<double>(numSub*numMetPnts, numCoeff);
    subjac = new fullVector<double>(numSub*numJacPnts);
    _bezier->subDivisor.mult(*current->_metcoeffs, *subcoeffs);
    _jacobian->subdivideBezierCoeff(*current->_jaccoeffs, *subjac);
    int depth = current->_depth;
    int num = current->_num;
      //Msg::Info("d %d RminBez %g / %g", depth, current->_RminBez, RminLag);

    //Msg::Info("delete %d (%d)", current, depth);
    //Msg::Info(" ");
    delete current;
    subdomains.pop();

    ++((MetricBasis*)this)->__numSubdivision;
    ++((MetricBasis*)this)->__TotSubdivision;
    ++((MetricBasis*)this)->__numSub[depth];
    ((MetricBasis*)this)->__maxdepth = std::max(__maxdepth, depth+1);
      //Msg::Info("subdividing %d", current);

    for (int i = 0; i < numSub; ++i) {
      coeff = new fullMatrix<double>(numMetPnts, numCoeff);
      coeff->copy(*subcoeffs, i * numMetPnts, numMetPnts, 0, numCoeff, 0, 0);
      jac = new fullVector<double>;
      jac->setAsProxy(*subjac, i * numJacPnts, numJacPnts);
      double minLag, minBez;
      _computeRmin(*coeff, *jac, minLag, minBez, depth+1);
      //Msg::Info("new RminBez %g", minBez);
      RminLag = std::min(RminLag, minLag);
      int newNum = num + (i+1) * pow_int(10, depth);
      MetricData *metData = new MetricData(coeff, jac, minBez, depth+1, newNum);

      if (aa < 200) {
        getMinR(__curElem, metData, 16);
      }

      //Msg::Info("    %g (%d)", minLag, metData);
      //Msg::Info("+4 %d", metData);
      subdomains.push(metData);
    }
    trash.push_back(subjac);
    delete subcoeffs;

    /*for (unsigned int i = 0; i < vect.size(); ++i) {
      Msg::Info("v %g", vect[i]->_RminBez);
    }
    Msg::Info("top %g (RminLag %g)", subdomains.top()->_RminBez, RminLag);
    return 0;*/
    //Msg::Info("RminLag %g - RminBez %g  @ %d", RminLag, subdomains.top()->_RminBez, subdomains.top()->_depth);
  }
  //Msg::Info("%g - %g = %g >? %g", RminLag, subdomains.top()->_RminBez, RminLag - subdomains.top()->_RminBez, tol);
  //Msg::Info("%d <? %d", subdomains.size(), 25000);

  md = subdomains.top();
  double ans = md->_RminBez;
  if (_chknumber(ans)) Msg::Error("ISNAN %d", subdomains.size());

  while (subdomains.size()) {
    md = subdomains.top();
    subdomains.pop();
    //Msg::Info("del %d", md);
    //Msg::Info(" ");
    delete md;
  }
  for (unsigned int i = 0; i < trash.size(); ++i) {
    delete trash[i];
  }

  //Msg::Info("bez%g lag%g", ans, RminLag);
  return ans;
}

void MetricBasis::_computeTermBeta(double &a, double &K,
                                   double &dRda, double &term1,
                                   double &phip) const
{
  double x0 = .5 * (K - a*a*a + 3*a);
  double sin, sqrt;
  if (x0 > 1) {
    const double p = -3;
    double q = -K + 2;
    a = cubicCardanoRoot(p, q);

    x0 = 1;
    phip = M_PI / 3;
    term1 = 1 + .5 * a;
    sin = std::sqrt(3) / 2;
    sqrt = 0;
  }
  else if (x0 < -1) {
    K = -2 + a*a*a - 3*a;

    x0 = -1;
    phip = 2 * M_PI / 3;
    term1 = 1 - .5 * a;
    sin = std::sqrt(3) / 2;
    sqrt = 0;
  }
  else {
    phip = (std::acos(x0) + M_PI) / 3;
    term1 = 1 + a * std::cos(phip);
    sin = std::sin(phip);
    sqrt = std::sqrt(1-x0*x0);
  }
  dRda = sin * sqrt + .5 * term1 * (1-a*a);
}

void MetricBasis::_getMetricData(MElement *el, MetricData *&md) const
{
  int nSampPnts = _gradients->getNumSamplingPoints();
  int nMapping = _gradients->getNumMapNodes();
  fullMatrix<double> nodes(nMapping, 3);
  el->getNodesCoord(nodes);

  // Metric coefficients
  fullMatrix<double> metCoeffLag;

  switch (el->getDim()) {
  case 0 :
    md = NULL;
    return;
  case 1 :
  case 2 :
    Msg::Fatal("not implemented");
    break;

  case 3 :
    {
      fullMatrix<double> dxyzdX(nSampPnts,3), dxyzdY(nSampPnts,3), dxyzdZ(nSampPnts,3);
      _gradients->getGradientsFromNodes(nodes, &dxyzdX, &dxyzdY, &dxyzdZ);

      metCoeffLag.resize(nSampPnts, 7);
      for (int i = 0; i < nSampPnts; i++) {
        const double &dxdX = dxyzdX(i,0), &dydX = dxyzdX(i,1), &dzdX = dxyzdX(i,2);
        const double &dxdY = dxyzdY(i,0), &dydY = dxyzdY(i,1), &dzdY = dxyzdY(i,2);
        const double &dxdZ = dxyzdZ(i,0), &dydZ = dxyzdZ(i,1), &dzdZ = dxyzdZ(i,2);
        const double dvxdX = dxdX*dxdX + dydX*dydX + dzdX*dzdX;
        const double dvxdY = dxdY*dxdY + dydY*dydY + dzdY*dzdY;
        const double dvxdZ = dxdZ*dxdZ + dydZ*dydZ + dzdZ*dzdZ;
        metCoeffLag(i, 0) = (dvxdX + dvxdY + dvxdZ) / 3;
        metCoeffLag(i, 1) = dvxdX - metCoeffLag(i, 0);
        metCoeffLag(i, 2) = dvxdY - metCoeffLag(i, 0);
        metCoeffLag(i, 3) = dvxdZ - metCoeffLag(i, 0);
        const double fact = std::sqrt(2);
        metCoeffLag(i, 4) = fact * (dxdX*dxdY + dydX*dydY + dzdX*dzdY);
        metCoeffLag(i, 5) = fact * (dxdZ*dxdY + dydZ*dydY + dzdZ*dzdY);
        metCoeffLag(i, 6) = fact * (dxdX*dxdZ + dydX*dydZ + dzdX*dzdZ);
      }
    }
    break;
  }

  fullMatrix<double> *metCoeff;
  metCoeff = new fullMatrix<double>(nSampPnts, metCoeffLag.size2());
  _bezier->matrixLag2Bez.mult(metCoeffLag, *metCoeff);

  // Jacobian coefficients
  fullVector<double> jacLag(_jacobian->getNumJacNodes());
  fullVector<double> *jac = new fullVector<double>(_jacobian->getNumJacNodes());
  _jacobian->getSignedJacobian(nodes, jacLag);
  _jacobian->lag2Bez(jacLag, *jac);

  md = new MetricData(metCoeff, jac, -1, 0, 0);
}

double MetricBasis::_minp2(const fullMatrix<double> &coeff) const
{
  double min = 1e10;
  std::map<int, std::vector<IneqData> >::const_iterator it = _ineqA.begin();
  while (it != _ineqA.end()) {
    double val = 0;
    for (unsigned int k = 0; k < it->second.size(); ++k) {
      const int i = it->second[k].i;
      const int j = it->second[k].j;
      double tmp = 0;
      for (int l = 1; l < 7; ++l) {
        tmp += coeff(i, l) * coeff(j, l);
      }
      val += it->second[k].val * tmp;
    }
    min = std::min(val, min);
    ++it;
  }

  return min > 0 ? std::sqrt(min/6) : 0;
}

double MetricBasis::_minp(const fullMatrix<double> &coeff) const
{
  fullMatrix<double> minmaxCoeff(2, 6);
  for (int j = 0; j < 6; ++j) {
    minmaxCoeff(0, j) = coeff(0, j+1);
    minmaxCoeff(1, j) = coeff(0, j+1);
  }

  for (int i = 1; i < coeff.size1(); ++i) {
    for (int j = 0; j < 6; ++j) {
      minmaxCoeff(0, j) = std::min(coeff(i, j+1), minmaxCoeff(0, j));
      minmaxCoeff(1, j) = std::max(coeff(i, j+1), minmaxCoeff(1, j));
    }
  }

  double ans = 0;
  for (int j = 0; j < 6; ++j) {
    if (minmaxCoeff(0, j) * minmaxCoeff(1, j) > 0) {
      ans += minmaxCoeff(0, j) > 0 ?
          pow_int(minmaxCoeff(0, j), 2) :
          pow_int(minmaxCoeff(1, j), 2);
    }
  }
  return std::sqrt(ans/6);
}

double MetricBasis::_minq(const fullMatrix<double> &coeff) const
{
  double ans = coeff(0, 0);
  for (int i = 1; i < coeff.size1(); ++i) {
    if (ans > coeff(i, 0)) ans = coeff(i, 0);
  }
  return ans;
}

double MetricBasis::_maxp(const fullMatrix<double> &coeff) const
{
  double ans = 0;
  for (int i = 0; i < coeff.size1(); ++i) {
    double tmp = 0;
    for (int j = 1; j < 7; ++j) {
      tmp += pow_int(coeff(i, j), 2);
    }
    ans = std::max(ans, tmp);
  }
  return std::sqrt(ans/6);
}

double MetricBasis::_maxq(const fullMatrix<double> &coeff) const
{
  double ans = coeff(0, 0);
  for (int i = 1; i < coeff.size1(); ++i) {
    if (ans < coeff(i, 0)) ans = coeff(i, 0);
  }
  return ans;
}

void MetricBasis::_minMaxA(
    const fullMatrix<double> &coeff, double &min, double &max) const
{
  min = 1e10;
  max = 0;
  std::map<int, std::vector<IneqData> >::const_iterator it = _ineqA.begin();
  while (it != _ineqA.end()) {
    double num = 0;
    double den = 0;
    for (unsigned int k = 0; k < it->second.size(); ++k) {
      const int i = it->second[k].i;
      const int j = it->second[k].j;
      double tmp = 0;
      for (int l = 1; l < 7; ++l) {
        tmp += coeff(i, l) * coeff(j, l);
      }
      den += it->second[k].val * tmp;
      num += it->second[k].val * coeff(i, 0) * coeff(j, 0);
    }
    double val = num/den;
    min = std::min(val, min);
    max = std::max(val, max);
    ++it;
  }
  min *= 6;
  max *= 6;

  min = min > 1 ? std::sqrt(min) : 1;
  max = std::sqrt(max);
}

void MetricBasis::_minMaxJacobianSqr(
    const fullVector<double> &jac, double &min, double &max) const
{
  static int a = 1;
  if (++a == 1) {
    for (int i = 1; i < jac.size(); ++i) {
      Msg::Info("<%g>", jac(i));
    }
  }
  min = max = jac(0);
  for (int i = 1; i < jac.size(); ++i) {
    if (min > jac(i)) min = jac(i);
    if (max < jac(i)) max = jac(i);
  }

  if (a == 1) {
      Msg::Info("%g %g", min, max);
  }

  if (min*max < 0) {
    max = max > -min ? max*max : min*min;
    min = 0;
  }
  else {
    if (max > 0) {
      max = max*max;
      min = min*min;
    }
    else {
      double tmp = max;
      max = min*min;
      min = tmp*tmp;
    }
  }
}

void MetricBasis::_minJ2P3(const fullMatrix<double> &coeff,
    const fullVector<double> &jac, double &min) const
{
  fullVector<double> r(coeff.size1());
  for (int i = 0; i < coeff.size1(); ++i) {
    r(i) = 0;
    for (int l = 1; l < 7; ++l) {
      r(i) += coeff(i, l) * coeff(i, l);
    }
    r(i) = std::sqrt(r(i)/6);
  }

  min = 1e10;
  std::map<int, std::vector<IneqData> >::const_iterator itJ, itP;
  itJ = _ineqJ2.begin();
  itP = _ineqP3.begin();

  if (_ineqP3.size() != _ineqJ2.size()) Msg::Fatal("sizes P3 %d, J2 %d", _ineqP3.size(), _ineqJ2.size());
  //Msg::Warning("sizes %d %d", _ineqJ2.size(), _ineqP3.size());
  int count = 0;
  while (itJ != _ineqJ2.end() && itP != _ineqP3.end()) {
    if (count >= (int)_ineqJ2.size()) Msg::Fatal("aaargh");
    if (itJ->first != itP->first) Msg::Fatal("not same hash %d %d", itJ->first, itP->first);

    double num = 0;
    //Msg::Info("sizej %d", itJ->second.size());
    for (unsigned int l = 0; l < itJ->second.size(); ++l) {
      const int i = itJ->second[l].i;
      const int j = itJ->second[l].j;
      num += itJ->second[l].val * jac(i) * jac(j);
    }

    double den = 0;
    //Msg::Info("sizep %d", itP->second.size());
    for (unsigned int l = 0; l < itP->second.size(); ++l) {
      const int i = itP->second[l].i;
      const int j = itP->second[l].j;
      const int k = itP->second[l].k;
      //Msg::Info("i%d j%d k%d", i, j, k);
      if (l>=itP->second.size()) Msg::Error("l %d/%d", l, itP->second.size());
      if (i>=r.size() || j>=r.size()||k>=r.size() ) Msg::Fatal("i%d j%d k%d /%d (%dl%d)", i, j, k, r.size(), count, l);
      den += itP->second[l].val * r(i) * r(j) * r(k);
    }
    //Msg::Info("%g/%g = %g", num, den, num/den);
    min = std::min(min, num/den);
    ++itJ;
    ++itP;
    ++count;
  }
  min = std::max(min, 0.);
}

void MetricBasis::_maxAstKpos(const fullMatrix<double> &coeff,
    const fullVector<double> &jac, double minK, double beta, double &maxa) const
{
  fullVector<double> P(coeff.size1());
  for (int i = 0; i < coeff.size1(); ++i) {
    P(i) = 0;
    for (int l = 1; l < 7; ++l) {
      P(i) += coeff(i, l) * coeff(i, l);
    }
    P(i) = std::sqrt(P(i)/6);
  }

  double min = 1e10;

  std::map<int, std::vector<IneqData> >::const_iterator itJ, itP;
  itJ = _ineqJ2.begin();
  itP = _ineqP3.begin();

  while (itJ != _ineqJ2.end() && itP != _ineqP3.end()) {
    double num = 0, den = 0;
    for (unsigned int l = 0; l < itJ->second.size(); ++l) {
      const int i = itJ->second[l].i;
      const int j = itJ->second[l].j;
      num += itJ->second[l].val * jac(i) * jac(j);
    }
    num *= beta;
    for (unsigned int l = 0; l < itP->second.size(); ++l) {
      const int i = itP->second[l].i;
      const int j = itP->second[l].j;
      const int k = itP->second[l].k;
      num -= itP->second[l].val * coeff(i, 0) * coeff(j, 0) * coeff(k, 0);
      den += itP->second[l].val * P(i) * P(j) * P(k);
    }
    min = std::min(min, num/den);
    ++itJ;
    ++itP;
  }

  maxa = std::pow(beta*minK-min, 1/3.);
}

void MetricBasis::_maxAstKneg(const fullMatrix<double> &coeff,
    const fullVector<double> &jac, double minK, double beta, double &maxa) const
{
  fullVector<double> P(coeff.size1());
  fullMatrix<double> Q(coeff.size1(), coeff.size1());
  for (int i = 0; i < coeff.size1(); ++i) {
    P(i) = 0;
    for (int l = 1; l < 7; ++l) {
      P(i) += coeff(i, l) * coeff(i, l);
    }
    P(i) = std::sqrt(P(i)/6);
    for (int j = 0; j < coeff.size1(); ++j) {
      Q(i, j) = 0;
      for (int l = 1; l < 7; ++l) {
        Q(i, j) += coeff(i, l) * coeff(j, l);
      }
      Q(i, j) /= 6;
    }
  }

  double min = 1e10;

  std::map<int, std::vector<IneqData> >::const_iterator itJ, itP;
  itJ = _ineqJ2.begin();
  itP = _ineqP3.begin();

  while (itJ != _ineqJ2.end() && itP != _ineqP3.end()) {
    double num = 0, den = 0;
    for (unsigned int l = 0; l < itJ->second.size(); ++l) {
      const int i = itJ->second[l].i;
      const int j = itJ->second[l].j;
      num += itJ->second[l].val * jac(i) * jac(j);
    }
    num *= beta;
    for (unsigned int l = 0; l < itP->second.size(); ++l) {
      const int i = itP->second[l].i;
      const int j = itP->second[l].j;
      const int k = itP->second[l].k;
      num -= itP->second[l].val * coeff(i, 0) * coeff(j, 0) * coeff(k, 0);
      double tmp = P(i) * Q(j, k);
      tmp = std::min(tmp, P(j) * Q(i, k));
      tmp = std::min(tmp, P(k) * Q(i, j));
      den += itP->second[l].val * tmp;
    }
    min = std::min(min, num/den);
    ++itJ;
    ++itP;
  }

  maxa = std::pow(beta*minK-min, 1/3.);
}

void MetricBasis::_maxKstAfast(const fullMatrix<double> &coeff,
    const fullVector<double> &jac, double mina, double beta, double &maxK) const
{
  fullVector<double> r(coeff.size1());
  for (int i = 0; i < coeff.size1(); ++i) {
    r(i) = 0;
    for (int l = 1; l < 7; ++l) {
      r(i) += coeff(i, l) * coeff(i, l);
    }
    r(i) = std::sqrt(r(i)/6);
  }

  double min = 1e10;

  std::map<int, std::vector<IneqData> >::const_iterator itJ, itP;
  itJ = _ineqJ2.begin();
  itP = _ineqP3.begin();

  while (itJ != _ineqJ2.end() && itP != _ineqP3.end()) {
    double num = 0, den = 0;
    for (unsigned int l = 0; l < itJ->second.size(); ++l) {
      const int i = itJ->second[l].i;
      const int j = itJ->second[l].j;
      num -= itJ->second[l].val * jac(i) * jac(j);
    }
    num *= beta;
    for (unsigned int l = 0; l < itP->second.size(); ++l) {
      const int i = itP->second[l].i;
      const int j = itP->second[l].j;
      const int k = itP->second[l].k;
      num += itP->second[l].val * coeff(i, 0) * coeff(j, 0) * coeff(k, 0);
      den += itP->second[l].val * r(i) * r(j) * r(k);
    }
    min = std::min(min, num/den);
    ++itJ;
    ++itP;
  }

  maxK = 1/beta*(mina*mina*mina-min);
}

void MetricBasis::_maxKstAsharp(const fullMatrix<double> &coeff,
    const fullVector<double> &jac, double mina, double beta, double &maxK) const
{
  fullVector<double> P(coeff.size1());
  fullMatrix<double> Q(coeff.size1(), coeff.size1());
  for (int i = 0; i < coeff.size1(); ++i) {
    P(i) = 0;
    for (int l = 1; l < 7; ++l) {
      P(i) += coeff(i, l) * coeff(i, l);
    }
    P(i) = std::sqrt(P(i)/6);
    for (int j = 0; j < coeff.size1(); ++j) {
      Q(i, j) = 0;
      for (int l = 1; l < 7; ++l) {
        Q(i, j) += coeff(i, l) * coeff(j, l);
      }
      Q(i, j) /= 6;
    }
  }

  double min = 1e10;

  std::map<int, std::vector<IneqData> >::const_iterator itJ, itP;
  itJ = _ineqJ2.begin();
  itP = _ineqP3.begin();

  while (itJ != _ineqJ2.end() && itP != _ineqP3.end()) {
    double num = 0, den = 0;
    for (unsigned int l = 0; l < itJ->second.size(); ++l) {
      const int i = itJ->second[l].i;
      const int j = itJ->second[l].j;
      num -= itJ->second[l].val * jac(i) * jac(j);
    }
    num *= beta;
    for (unsigned int l = 0; l < itP->second.size(); ++l) {
      const int i = itP->second[l].i;
      const int j = itP->second[l].j;
      const int k = itP->second[l].k;
      num += itP->second[l].val * coeff(i, 0) * coeff(j, 0) * coeff(k, 0);
      if (j == k)
        den += itP->second[l].val * Q(i,i) * P(i);
      else
        den += itP->second[l].val * 1/3*(Q(i,j)*P(k)+Q(i,k)*P(j)+Q(k,j)*P(i));
    }
    min = std::min(min, num/den);
    ++itJ;
    ++itP;
  }

  maxK = 1/beta*(mina*mina*mina-min);
}
