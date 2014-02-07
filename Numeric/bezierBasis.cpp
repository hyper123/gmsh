// Gmsh - Copyright (C) 1997-2014 C. Geuzaine, J.-F. Remacle
//
// See the LICENSE.txt file for license information. Please report all
// bugs and problems to the public mailing list <gmsh@geuz.org>.


#include "GmshDefines.h"
#include "GmshMessage.h"
#include <vector>
#include "polynomialBasis.h"
#include "pyramidalBasis.h"
#include "pointsGenerators.h"
#include "BasisFactory.h"
#include "Numeric.h"

namespace {
// Sub Control Points
  std::vector< fullMatrix<double> > generateSubPointsLine(int order)
  {
    std::vector< fullMatrix<double> > subPoints(2);

    subPoints[0] = gmshGenerateMonomialsLine(order);
    subPoints[0].scale(.5/order);

    subPoints[1].copy(subPoints[0]);
    subPoints[1].add(.5);

    return subPoints;
  }

  std::vector< fullMatrix<double> > generateSubPointsTriangle(int order)
  {
    std::vector< fullMatrix<double> > subPoints(4);
    fullMatrix<double> prox;

    subPoints[0] = gmshGenerateMonomialsTriangle(order);
    subPoints[0].scale(.5/order);

    subPoints[1].copy(subPoints[0]);
    prox.setAsProxy(subPoints[1], 0, 1);
    prox.add(.5);

    subPoints[2].copy(subPoints[0]);
    prox.setAsProxy(subPoints[2], 1, 1);
    prox.add(.5);

    subPoints[3].copy(subPoints[0]);
    subPoints[3].scale(-1.);
    subPoints[3].add(.5);

    return subPoints;
  }

  std::vector< fullMatrix<double> > generateSubPointsQuad(int order)
  {
    std::vector< fullMatrix<double> > subPoints(4);
    fullMatrix<double> prox;

    subPoints[0] = gmshGenerateMonomialsQuadrangle(order);
    subPoints[0].scale(.5/order);

    subPoints[1].copy(subPoints[0]);
    prox.setAsProxy(subPoints[1], 0, 1);
    prox.add(.5);

    subPoints[2].copy(subPoints[0]);
    prox.setAsProxy(subPoints[2], 1, 1);
    prox.add(.5);

    subPoints[3].copy(subPoints[1]);
    prox.setAsProxy(subPoints[3], 1, 1);
    prox.add(.5);

    return subPoints;
  }

  std::vector< fullMatrix<double> > generateSubPointsTetrahedron(int order)
  {
    std::vector< fullMatrix<double> > subPoints(8);
    fullMatrix<double> prox1;
    fullMatrix<double> prox2;

    subPoints[0] = gmshGenerateMonomialsTetrahedron(order);
    subPoints[0].scale(.5/order);

    subPoints[1].copy(subPoints[0]);
    prox1.setAsProxy(subPoints[1], 0, 1);
    prox1.add(.5);

    subPoints[2].copy(subPoints[0]);
    prox1.setAsProxy(subPoints[2], 1, 1);
    prox1.add(.5);

    subPoints[3].copy(subPoints[0]);
    prox1.setAsProxy(subPoints[3], 2, 1);
    prox1.add(.5);

    // u := .5-u-w
    // v := .5-v-w
    // w := w
    subPoints[4].copy(subPoints[0]);
    prox1.setAsProxy(subPoints[4], 0, 2);
    prox1.scale(-1.);
    prox1.add(.5);
    prox1.setAsProxy(subPoints[4], 0, 1);
    prox2.setAsProxy(subPoints[4], 2, 1);
    prox1.add(prox2, -1.);
    prox1.setAsProxy(subPoints[4], 1, 1);
    prox1.add(prox2, -1.);

    // u := u
    // v := .5-v
    // w := w+v
    subPoints[5].copy(subPoints[0]);
    prox1.setAsProxy(subPoints[5], 2, 1);
    prox2.setAsProxy(subPoints[5], 1, 1);
    prox1.add(prox2);
    prox2.scale(-1.);
    prox2.add(.5);

    // u := .5-u
    // v := v
    // w := w+u
    subPoints[6].copy(subPoints[0]);
    prox1.setAsProxy(subPoints[6], 2, 1);
    prox2.setAsProxy(subPoints[6], 0, 1);
    prox1.add(prox2);
    prox2.scale(-1.);
    prox2.add(.5);

    // u := u+w
    // v := v+w
    // w := .5-w
    subPoints[7].copy(subPoints[0]);
    prox1.setAsProxy(subPoints[7], 0, 1);
    prox2.setAsProxy(subPoints[7], 2, 1);
    prox1.add(prox2);
    prox1.setAsProxy(subPoints[7], 1, 1);
    prox1.add(prox2);
    prox2.scale(-1.);
    prox2.add(.5);


    return subPoints;
  }

  std::vector< fullMatrix<double> > generateSubPointsPrism(int order)
  {
    std::vector< fullMatrix<double> > subPoints(8);
    fullMatrix<double> prox;

    subPoints[0] = gmshGenerateMonomialsPrism(order);
    subPoints[0].scale(.5/order);

    subPoints[1].copy(subPoints[0]);
    prox.setAsProxy(subPoints[1], 0, 1);
    prox.add(.5);

    subPoints[2].copy(subPoints[0]);
    prox.setAsProxy(subPoints[2], 1, 1);
    prox.add(.5);

    subPoints[3].copy(subPoints[0]);
    prox.setAsProxy(subPoints[3], 0, 2);
    prox.scale(-1.);
    prox.add(.5);

    subPoints[4].copy(subPoints[0]);
    prox.setAsProxy(subPoints[4], 2, 1);
    prox.add(.5);

    subPoints[5].copy(subPoints[1]);
    prox.setAsProxy(subPoints[5], 2, 1);
    prox.add(.5);

    subPoints[6].copy(subPoints[2]);
    prox.setAsProxy(subPoints[6], 2, 1);
    prox.add(.5);

    subPoints[7].copy(subPoints[3]);
    prox.setAsProxy(subPoints[7], 2, 1);
    prox.add(.5);

    return subPoints;
  }

  std::vector< fullMatrix<double> > generateSubPointsHex(int order)
  {
    std::vector< fullMatrix<double> > subPoints(8);
    fullMatrix<double> prox;

    subPoints[0] = gmshGenerateMonomialsHexahedron(order);
    subPoints[0].scale(.5/order);

    subPoints[1].copy(subPoints[0]);
    prox.setAsProxy(subPoints[1], 0, 1);
    prox.add(.5);

    subPoints[2].copy(subPoints[0]);
    prox.setAsProxy(subPoints[2], 1, 1);
    prox.add(.5);

    subPoints[3].copy(subPoints[1]);
    prox.setAsProxy(subPoints[3], 1, 1);
    prox.add(.5);

    subPoints[4].copy(subPoints[0]);
    prox.setAsProxy(subPoints[4], 2, 1);
    prox.add(.5);

    subPoints[5].copy(subPoints[1]);
    prox.setAsProxy(subPoints[5], 2, 1);
    prox.add(.5);

    subPoints[6].copy(subPoints[2]);
    prox.setAsProxy(subPoints[6], 2, 1);
    prox.add(.5);

    subPoints[7].copy(subPoints[3]);
    prox.setAsProxy(subPoints[7], 2, 1);
    prox.add(.5);

    return subPoints;
  }

  std::vector< fullMatrix<double> > generateSubPointsPyr(int order)
  {
    if (order == 0) {
      std::vector< fullMatrix<double> > subPoints(4);
      fullMatrix<double> prox;

      subPoints[0] = JacobianBasis::generateJacMonomialsPyramid(order);
      subPoints[0].scale(.5/(order+2));

      subPoints[1].copy(subPoints[0]);
      prox.setAsProxy(subPoints[1], 0, 1);
      prox.add(.5);

      subPoints[2].copy(subPoints[0]);
      prox.setAsProxy(subPoints[2], 1, 1);
      prox.add(.5);

      subPoints[3].copy(subPoints[1]);
      prox.setAsProxy(subPoints[3], 1, 1);
      prox.add(.5);

      return subPoints;
    }

    std::vector< fullMatrix<double> > subPoints(8);
    fullMatrix<double> ref, prox;

    subPoints[0] = JacobianBasis::generateJacMonomialsPyramid(order);
    int nPts = subPoints[0].size1();
    for (int i = 0; i < nPts; ++i) {
      const double factor = .5 / (order + 2 - subPoints[0](i, 2));
      subPoints[0](i, 0) = subPoints[0](i, 0) * factor;
      subPoints[0](i, 1) = subPoints[0](i, 1) * factor;
      subPoints[0](i, 2) = subPoints[0](i, 2) * .5 / (order + 2);
    }

    subPoints[1].copy(subPoints[0]);
    prox.setAsProxy(subPoints[1], 0, 1);
    prox.add(.5);

    subPoints[2].copy(subPoints[0]);
    prox.setAsProxy(subPoints[2], 1, 1);
    prox.add(.5);

    subPoints[3].copy(subPoints[1]);
    prox.setAsProxy(subPoints[3], 1, 1);
    prox.add(.5);

    subPoints[4].copy(subPoints[0]);
    prox.setAsProxy(subPoints[4], 2, 1);
    prox.add(.5);

    subPoints[5].copy(subPoints[1]);
    prox.setAsProxy(subPoints[5], 2, 1);
    prox.add(.5);

    subPoints[6].copy(subPoints[2]);
    prox.setAsProxy(subPoints[6], 2, 1);
    prox.add(.5);

    subPoints[7].copy(subPoints[3]);
    prox.setAsProxy(subPoints[7], 2, 1);
    prox.add(.5);

    for (int i = 0; i < 8; ++i) {
      for (int j = 0; j < nPts; ++j) {
        const double factor = (1. - subPoints[i](j, 2));
        subPoints[i](j, 0) = subPoints[i](j, 0) * factor;
        subPoints[i](j, 1) = subPoints[i](j, 1) * factor;
      }
    }

    return subPoints;
  }

  // Matrices generation
  int nChoosek(int n, int k)
  {
    if (n < k || k < 0) {
      Msg::Error("Wrong argument for combination.");
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

  fullMatrix<double> generateBez2LagMatrix
    (const fullMatrix<double> &exponent, const fullMatrix<double> &point,
     int order, int dimSimplex)
  {
    if(exponent.size1() != point.size1() || exponent.size2() != point.size2()){
      Msg::Fatal("Wrong sizes for bez2lag matrix generation %d %d -- %d %d",
        exponent.size1(),point.size1(),
        exponent.size2(),point.size2());
      return fullMatrix<double>(1, 1);
    }

    int ndofs = exponent.size1();
    int dim = exponent.size2();

    fullMatrix<double> bez2Lag(ndofs, ndofs);
    for (int i = 0; i < ndofs; i++) {
      for (int j = 0; j < ndofs; j++) {
        double dd = 1.;

        {
          double pointCompl = 1.;
          int exponentCompl = order;
          for (int k = 0; k < dimSimplex; k++) {
            dd *= nChoosek(exponentCompl, (int) exponent(i, k))
              * pow(point(j, k), exponent(i, k));
            pointCompl -= point(j, k);
            exponentCompl -= (int) exponent(i, k);
          }
          dd *= pow(pointCompl, exponentCompl);
        }

        for (int k = dimSimplex; k < dim; k++)
          dd *= nChoosek(order, (int) exponent(i, k))
              * pow(point(j, k), exponent(i, k))
              * pow(1. - point(j, k), order - exponent(i, k));

        bez2Lag(j, i) = dd;
      }
    }
    return bez2Lag;
  }

  fullMatrix<double> generateBez2LagMatrixPyramid
    (const fullMatrix<double> &exponent, const fullMatrix<double> &point,
     int order)
  {
    if(exponent.size1() != point.size1() || exponent.size2() != point.size2() ||
        exponent.size2() != 3){
      Msg::Fatal("Wrong sizes for bez2lag matrix generation %d %d -- %d %d",
        exponent.size1(),point.size1(),
        exponent.size2(),point.size2());
      return fullMatrix<double>(1, 1);
    }

    int ndofs = exponent.size1();

    fullMatrix<double> bez2Lag(ndofs, ndofs);
    for (int i = 0; i < ndofs; i++) {
      for (int j = 0; j < ndofs; j++) {
        bez2Lag(i, j) =
            nChoosek(order, exponent(j, 2))
            * pow(point(i, 2), exponent(j, 2)) //FIXME use pow_int
            * pow(1. - point(i, 2), order - exponent(j, 2));

        double p = order + 2 - exponent(j, 2);
        double denom = 1. - point(i, 2);
        bez2Lag(i, j) *=
              nChoosek(p, exponent(j, 0))
            * nChoosek(p, exponent(j, 1))
            * pow(point(i, 0) / denom, exponent(j, 0))
            * pow(point(i, 1) / denom, exponent(j, 1))
            * pow(1. - point(i, 0) / denom, p - exponent(j, 0))
            * pow(1. - point(i, 1) / denom, p - exponent(j, 1));
      }
    }
    return bez2Lag;
  }

  fullMatrix<double> generateSubDivisor
    (const fullMatrix<double> &exponents, const std::vector< fullMatrix<double> > &subPoints,
     const fullMatrix<double> &lag2Bez, int order, int dimSimplex)
  {
    if (exponents.size1() != lag2Bez.size1() || exponents.size1() != lag2Bez.size2()){
      Msg::Fatal("Wrong sizes for Bezier Divisor %d %d -- %d %d",
        exponents.size1(), lag2Bez.size1(),
        exponents.size1(), lag2Bez.size2());
      return fullMatrix<double>(1, 1);
    }

    int nbPts = lag2Bez.size1();
    int nbSubPts = nbPts * subPoints.size();

    fullMatrix<double> intermediate2(nbPts, nbPts);
    fullMatrix<double> subDivisor(nbSubPts, nbPts);

    for (unsigned int i = 0; i < subPoints.size(); i++) {
      fullMatrix<double> intermediate1 =
        generateBez2LagMatrix(exponents, subPoints[i], order, dimSimplex);
      lag2Bez.mult(intermediate1, intermediate2);
      subDivisor.copy(intermediate2, 0, nbPts, 0, nbPts, i*nbPts, 0);
    }
    return subDivisor;
  }

  fullMatrix<double> generateSubDivisorPyramid
    (const fullMatrix<double> &exponents, const std::vector< fullMatrix<double> > &subPoints,
     const fullMatrix<double> &lag2Bez, int order)
  {
    if (exponents.size1() != lag2Bez.size1() || exponents.size1() != lag2Bez.size2()){
      Msg::Fatal("Wrong sizes for Bezier Divisor %d %d -- %d %d",
        exponents.size1(), lag2Bez.size1(),
        exponents.size1(), lag2Bez.size2());
      return fullMatrix<double>(1, 1);
    }

    int nbPts = lag2Bez.size1();
    int nbSubPts = nbPts * subPoints.size();

    fullMatrix<double> intermediate2(nbPts, nbPts);
    fullMatrix<double> subDivisor(nbSubPts, nbPts);

    for (unsigned int i = 0; i < subPoints.size(); i++) {
      fullMatrix<double> intermediate1 =
        generateBez2LagMatrixPyramid(exponents, subPoints[i], order);
      lag2Bez.mult(intermediate1, intermediate2);
      subDivisor.copy(intermediate2, 0, nbPts, 0, nbPts, i*nbPts, 0);
    }
    return subDivisor;
  }
}

void bezierBasis::_construct(int parentType, int p)
{
  order = p;
  fullMatrix<double> exponents;
  std::vector< fullMatrix<double> > subPoints;

  if (parentType == TYPE_PYR) {
    dim = 3;
    numLagCoeff = 8;
    exponents = JacobianBasis::generateJacMonomialsPyramid(order);
    subPoints = generateSubPointsPyr(order);

    numDivisions = static_cast<int>(subPoints.size());

    fullMatrix<double> bezierPoints = exponents;
    bezierPoints.scale(1. / (order + 2));

    matrixBez2Lag = generateBez2LagMatrixPyramid(exponents, bezierPoints, order);
    matrixBez2Lag.invert(matrixLag2Bez);
    subDivisor = generateSubDivisorPyramid(exponents, subPoints, matrixLag2Bez, order);
    return;
  }

  int dimSimplex;
  switch (parentType) {
    case TYPE_PNT :
      dim = 0;
      numLagCoeff = 1;
      dimSimplex = 0;
      exponents = gmshGenerateMonomialsLine(0);
      subPoints.push_back(gmshGeneratePointsLine(0));
      break;
    case TYPE_LIN : {
      dim = 1;
      numLagCoeff = 2;
      dimSimplex = 0;
      exponents = gmshGenerateMonomialsLine(order);
      subPoints = generateSubPointsLine(order);
      break;
    }
    case TYPE_TRI : {
      dim = 2;
      numLagCoeff = 3;
      dimSimplex = 2;
      exponents = gmshGenerateMonomialsTriangle(order);
      subPoints = generateSubPointsTriangle(order);
      break;
    }
    case TYPE_QUA : {
      dim = 2;
      numLagCoeff = 4;
      dimSimplex = 0;
      exponents = gmshGenerateMonomialsQuadrangle(order);
      subPoints = generateSubPointsQuad(order);
      break;
    }
    case TYPE_TET : {
      dim = 3;
      numLagCoeff = 4;
      dimSimplex = 3;
      exponents = gmshGenerateMonomialsTetrahedron(order);
      subPoints = generateSubPointsTetrahedron(order);
      break;
    }
    case TYPE_PRI : {
      dim = 3;
      numLagCoeff = 6;
      dimSimplex = 2;
      exponents = gmshGenerateMonomialsPrism(order);
      subPoints = generateSubPointsPrism(order);
      break;
    }
    case TYPE_HEX : {
      dim = 3;
      numLagCoeff = 8;
      dimSimplex = 0;
      exponents = gmshGenerateMonomialsHexahedron(order);
      subPoints = generateSubPointsHex(order);
      break;
    }
    default : {
      Msg::Error("Unknown function space of parentType %d : "
          "reverting to TET_1", parentType);
      dim = 3;
      order = 0;
      numLagCoeff = 4;
      dimSimplex = 3;
      exponents = gmshGenerateMonomialsTetrahedron(order);
      subPoints = generateSubPointsTetrahedron(order);
      break;
    }
  }
  numDivisions = static_cast<int>(subPoints.size());

  fullMatrix<double> bezierPoints = exponents;
  bezierPoints.scale(1./order);

  matrixBez2Lag = generateBez2LagMatrix(exponents, bezierPoints, order, dimSimplex);
  matrixBez2Lag.invert(matrixLag2Bez);
  subDivisor = generateSubDivisor(exponents, subPoints, matrixLag2Bez, order, dimSimplex);
}
