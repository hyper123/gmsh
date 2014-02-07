// Gmsh - Copyright (C) 1997-2014 C. Geuzaine, J.-F. Remacle
//
// See the LICENSE.txt file for license information. Please report all
// bugs and problems to the public mailing list <gmsh@geuz.org>.

#include "pointsGenerators.h"
#include "MTriangle.h"
#include "MQuadrangle.h"
#include "MTetrahedron.h"
#include "MHexahedron.h"
#include "MPrism.h"
#include "MPyramid.h"


// Points Generators

fullMatrix<double> gmshGeneratePointsLine(int order)
{
  fullMatrix<double> points = gmshGenerateMonomialsLine(order);
  if (order == 0) return points;

  points.scale(2./order);
  points.add(-1.);
  return points;
}

fullMatrix<double> gmshGeneratePointsTriangle(int order, bool serendip)
{
  fullMatrix<double> points = gmshGenerateMonomialsTriangle(order, serendip);
  if (order == 0) return points;

  points.scale(1./order);
  return points;
}

fullMatrix<double> gmshGeneratePointsQuadrangle(int order, bool serendip)
{
  fullMatrix<double> points = gmshGenerateMonomialsQuadrangle(order, serendip);

  if (order == 0) return points;

  points.scale(2./order);
  points.add(-1.);
  return points;
}

fullMatrix<double> gmshGeneratePointsTetrahedron(int order, bool serendip)
{
  fullMatrix<double> points = gmshGenerateMonomialsTetrahedron(order, serendip);

  if (order == 0) return points;

  points.scale(1./order);
  return points;
}

fullMatrix<double> gmshGeneratePointsHexahedron(int order, bool serendip)
{
  fullMatrix<double> points = gmshGenerateMonomialsHexahedron(order, serendip);

  if (order == 0) return points;

  points.scale(2./order);
  points.add(-1.);
  return points;
}

fullMatrix<double> gmshGeneratePointsPrism(int order, bool serendip)
{
  fullMatrix<double> points = gmshGenerateMonomialsPrism(order, serendip);

  if (order == 0) return points;

  fullMatrix<double> tmp;
  tmp.setAsProxy(points, 0, 2);
  tmp.scale(1./order);

  tmp.setAsProxy(points, 2, 1);
  tmp.scale(2./order);
  tmp.add(-1.);

  return points;
}

fullMatrix<double> gmshGeneratePointsPyramid(int order, bool serendip)
{
  fullMatrix<double> points = gmshGenerateMonomialsPyramid(order, serendip);

  if (order == 0) return points;

  for (int i = 0; i < points.size1(); ++i) {
    points(i, 2) = points(i, 2) / order;
    const double duv = -1. + points(i, 2);
    points(i, 0) = duv + points(i, 0) * 2. / order;
    points(i, 1) = duv + points(i, 1) * 2. / order;
  }
  return points;
}

// Monomials Generators

fullMatrix<double> gmshGenerateMonomialsLine(int order, bool serendip)
{
  fullMatrix<double> monomials(order + 1, 1);
  monomials(0,0) = 0;
  if (order > 0) {
    monomials(1, 0) = order;
    for (int i = 2; i < order + 1; i++) monomials(i, 0) = i-1;
  }
  return monomials;
}

fullMatrix<double> gmshGenerateMonomialsTriangle(int order, bool serendip)
{
  int nbMonomials = serendip ? 3 * order : (order + 1) * (order + 2) / 2;
  if (serendip && !order) nbMonomials = 1;
  fullMatrix<double> monomials(nbMonomials, 2);

  monomials(0, 0) = 0;
  monomials(0, 1) = 0;

  if (order > 0) {
    monomials(1, 0) = order;
    monomials(1, 1) = 0;

    monomials(2, 0) = 0;
    monomials(2, 1) = order;

    if (order > 1) {
      int index = 3;
      for (int iedge = 0; iedge < 3; ++iedge) {
        int i0 = MTriangle::edges_tri(iedge, 0);
        int i1 = MTriangle::edges_tri(iedge, 1);

        int u_0 = (monomials(i1,0)-monomials(i0,0)) / order;
        int u_1 = (monomials(i1,1)-monomials(i0,1)) / order;

        for (int i = 1; i < order; ++i, ++index) {
          monomials(index, 0) = monomials(i0, 0) + u_0 * i;
          monomials(index, 1) = monomials(i0, 1) + u_1 * i;
        }
      }
      if (!serendip && order > 2) {
        fullMatrix<double> inner = gmshGenerateMonomialsTriangle(order-3);
        inner.add(1);
        monomials.copy(inner, 0, nbMonomials - index, 0, 2, index, 0);
      }
    }
  }
  return monomials;
}

fullMatrix<double> gmshGenerateMonomialsQuadrangle(int order, bool forSerendipPoints)
{
  int nbMonomials = forSerendipPoints ? order*4 : (order+1)*(order+1);
  if (forSerendipPoints && !order) nbMonomials = 1;
  fullMatrix<double> monomials(nbMonomials, 2);

  monomials(0, 0) = 0;
  monomials(0, 1) = 0;

  if (order > 0) {
    monomials(1, 0) = order;
    monomials(1, 1) = 0;

    monomials(2, 0) = order;
    monomials(2, 1) = order;

    monomials(3, 0) = 0;
    monomials(3, 1) = order;

    if (order > 1) {
      int index = 4;
      for (int iedge = 0; iedge < 4; ++iedge) {
        int i0 = MQuadrangle::edges_quad(iedge, 0);
        int i1 = MQuadrangle::edges_quad(iedge, 1);

        int u_0 = (monomials(i1,0)-monomials(i0,0)) / order;
        int u_1 = (monomials(i1,1)-monomials(i0,1)) / order;

        for (int i = 1; i < order; ++i, ++index) {
          monomials(index, 0) = monomials(i0, 0) + u_0 * i;
          monomials(index, 1) = monomials(i0, 1) + u_1 * i;
        }
      }

      if (!forSerendipPoints) {
        fullMatrix<double> inner = gmshGenerateMonomialsQuadrangle(order-2);
        inner.add(1);
        monomials.copy(inner, 0, nbMonomials - index, 0, 2, index, 0);
      }
    }
  }
  return monomials;
}

/*
00 10 20 30 40 �..
01 11 21 31 41 �..
02 12
03 13
04 14
�. �.
*/

fullMatrix<double> gmshGenerateMonomialsQuadSerendipity(int order)
{
  int nbMonomials = order ? order*4 : 1;
  fullMatrix<double> monomials(nbMonomials, 2);

  monomials(0, 0) = 0;
  monomials(0, 1) = 0;

  if (order > 0) {
    monomials(1, 0) = 1;
    monomials(1, 1) = 0;

    monomials(2, 0) = 1;
    monomials(2, 1) = 1;

    monomials(3, 0) = 0;
    monomials(3, 1) = 1;

    if (order > 1) {
      int index = 3;
      for (int p = 2; p <= order; ++p) {
        monomials(++index, 0) = p;
        monomials(  index, 1) = 0;

        monomials(++index, 0) = p;
        monomials(  index, 1) = 1;

        monomials(++index, 0) = 1;
        monomials(  index, 1) = p;

        monomials(++index, 0) = 0;
        monomials(  index, 1) = p;
      }
    }
  }
  return monomials;
}

//KH : caveat : node coordinates are not yet coherent with node numbering associated
//              to numbering of principal vertices of face !!!!
fullMatrix<double> gmshGenerateMonomialsTetrahedron(int order, bool serendip)
{
  int nbMonomials = serendip ? 4 + (order - 1)*6 : (order + 1)*(order + 2)*(order + 3)/6;
  if (serendip && !order) nbMonomials = 1;
  fullMatrix<double> monomials(nbMonomials, 3);

  monomials(0, 0) = 0;
  monomials(0, 1) = 0;
  monomials(0, 2) = 0;

  if (order > 0) {
    monomials(1, 0) = order;
    monomials(1, 1) = 0;
    monomials(1, 2) = 0;

    monomials(2, 0) = 0;
    monomials(2, 1) = order;
    monomials(2, 2) = 0;

    monomials(3, 0) = 0;
    monomials(3, 1) = 0;
    monomials(3, 2) = order;

    // the template has been defined in table edges_tetra and faces_tetra (MElement.h)

    if (order > 1) {
      int index = 4;
      for (int iedge = 0; iedge < 6; ++iedge) {
        int i0 = MTetrahedron::edges_tetra(iedge, 0);
        int i1 = MTetrahedron::edges_tetra(iedge, 1);

        int u[3];
        u[0] = (monomials(i1,0)-monomials(i0,0)) / order;
        u[1] = (monomials(i1,1)-monomials(i0,1)) / order;
        u[2] = (monomials(i1,2)-monomials(i0,2)) / order;

        for (int i = 1; i < order; ++i, ++index) {
          monomials(index, 0) = monomials(i0, 0) + u[0] * i;
          monomials(index, 1) = monomials(i0, 1) + u[1] * i;
          monomials(index, 2) = monomials(i0, 2) + u[2] * i;
        }
      }

      if (!serendip && order > 2) {
        fullMatrix<double> dudv = gmshGenerateMonomialsTriangle(order - 3);
        dudv.add(1);

        for (int iface = 0; iface < 4; ++iface) {
          int i0 = MTetrahedron::faces_tetra(iface, 0);
          int i1 = MTetrahedron::faces_tetra(iface, 1);
          int i2 = MTetrahedron::faces_tetra(iface, 2);

          int u[3];
          u[0] = (monomials(i1, 0) - monomials(i0, 0)) / order;
          u[1] = (monomials(i1, 1) - monomials(i0, 1)) / order;
          u[2] = (monomials(i1, 2) - monomials(i0, 2)) / order;
          int v[3];
          v[0] = (monomials(i2, 0) - monomials(i0, 0)) / order;
          v[1] = (monomials(i2, 1) - monomials(i0, 1)) / order;
          v[2] = (monomials(i2, 2) - monomials(i0, 2)) / order;

          for (int i = 0; i < dudv.size1(); ++i, ++index) {
            monomials(index, 0) = monomials(i0, 0) + u[0] * dudv(i, 0) + v[0] * dudv(i, 1);
            monomials(index, 1) = monomials(i0, 1) + u[1] * dudv(i, 0) + v[1] * dudv(i, 1);
            monomials(index, 2) = monomials(i0, 2) + u[2] * dudv(i, 0) + v[2] * dudv(i, 1);
          }
        }

        if (order > 3) {
          fullMatrix<double> inner = gmshGenerateMonomialsTetrahedron(order - 4);
          inner.add(1);
          monomials.copy(inner, 0, nbMonomials - index, 0, 3, index, 0);
        }
      }
    }
  }
  return monomials;
}

fullMatrix<double> gmshGenerateMonomialsPrism(int order, bool forSerendipPoints)
{
  int nbMonomials = forSerendipPoints ? 6 + (order-1)*9 :
                                        (order + 1)*(order + 1)*(order + 2)/2;
  if (forSerendipPoints && !order) nbMonomials = 1;
  fullMatrix<double> monomials(nbMonomials, 3);

  monomials(0, 0) = 0;
  monomials(0, 1) = 0;
  monomials(0, 2) = 0;

  if (order > 0) {
    monomials(1, 0) = order;
    monomials(1, 1) = 0;
    monomials(1, 2) = 0;

    monomials(2, 0) = 0;
    monomials(2, 1) = order;
    monomials(2, 2) = 0;

    monomials(3, 0) = 0;
    monomials(3, 1) = 0;
    monomials(3, 2) = order;

    monomials(4, 0) = order;
    monomials(4, 1) = 0;
    monomials(4, 2) = order;

    monomials(5, 0) = 0;
    monomials(5, 1) = order;
    monomials(5, 2) = order;

    if (order > 1) {
      int index = 6;
      for (int iedge = 0; iedge < 9; ++iedge) {
        int i0 = MPrism::edges_prism(iedge, 0);
        int i1 = MPrism::edges_prism(iedge, 1);

        int u_1 = (monomials(i1,0)-monomials(i0,0)) / order;
        int u_2 = (monomials(i1,1)-monomials(i0,1)) / order;
        int u_3 = (monomials(i1,2)-monomials(i0,2)) / order;

        for (int i = 1; i < order; ++i, ++index) {
          monomials(index, 0) = monomials(i0, 0) + i * u_1;
          monomials(index, 1) = monomials(i0, 1) + i * u_2;
          monomials(index, 2) = monomials(i0, 2) + i * u_3;
        }
      }

      if (!forSerendipPoints) {
        fullMatrix<double> dudvQ = gmshGenerateMonomialsQuadrangle(order - 2);
        dudvQ.add(1);

        fullMatrix<double> dudvT;
        if (order > 2) {
          dudvT = gmshGenerateMonomialsTriangle(order - 3);
          dudvT.add(1);
        }

        for (int iface = 0; iface < 5; ++iface) {
          int i0, i1, i2;
          i0 = MPrism::faces_prism(iface, 0);
          i1 = MPrism::faces_prism(iface, 1);
          fullMatrix<double> dudv;
          if (MPrism::faces_prism(iface, 3) != -1) {
            i2 = MPrism::faces_prism(iface, 3);
            dudv.setAsProxy(dudvQ);
          }
          else if (order > 2) {
            i2 = MPrism::faces_prism(iface, 2);
            dudv.setAsProxy(dudvT);
          }
          else continue;

          int u[3];
          u[0] = (monomials(i1, 0) - monomials(i0, 0)) / order;
          u[1] = (monomials(i1, 1) - monomials(i0, 1)) / order;
          u[2] = (monomials(i1, 2) - monomials(i0, 2)) / order;
          int v[3];
          v[0] = (monomials(i2, 0) - monomials(i0, 0)) / order;
          v[1] = (monomials(i2, 1) - monomials(i0, 1)) / order;
          v[2] = (monomials(i2, 2) - monomials(i0, 2)) / order;

          for (int i = 0; i < dudv.size1(); ++i, ++index) {
            monomials(index, 0) = monomials(i0, 0) + u[0] * dudv(i, 0) + v[0] * dudv(i, 1);
            monomials(index, 1) = monomials(i0, 1) + u[1] * dudv(i, 0) + v[1] * dudv(i, 1);
            monomials(index, 2) = monomials(i0, 2) + u[2] * dudv(i, 0) + v[2] * dudv(i, 1);
          }
        }

        if (order > 2) {
          fullMatrix<double> triMonomials  = gmshGenerateMonomialsTriangle(order - 3);
          fullMatrix<double> lineMonomials = gmshGenerateMonomialsLine(order - 2);

          for (int i = 0; i < triMonomials.size1(); ++i) {
            for (int j = 0; j < lineMonomials.size1(); ++j, ++index) {
              monomials(index, 0) = 1 + triMonomials(i, 0);
              monomials(index, 1) = 1 + triMonomials(i, 1);
              monomials(index, 2) = 1 + lineMonomials(j, 0);
            }
          }
        }
      }
    }
  }
  return monomials;
}

fullMatrix<double> gmshGenerateMonomialsPrismSerendipity(int order)
{
  int nbMonomials = order ? 6 + (order-1) * 9 : 1;
  fullMatrix<double> monomials(nbMonomials, 3);

  monomials(0, 0) = 0;
  monomials(0, 1) = 0;
  monomials(0, 2) = 0;

  if (order > 0) {
    monomials(1, 0) = 1;
    monomials(1, 1) = 0;
    monomials(1, 2) = 0;

    monomials(2, 0) = 0;
    monomials(2, 1) = 1;
    monomials(2, 2) = 0;

    monomials(3, 0) = 0;
    monomials(3, 1) = 0;
    monomials(3, 2) = 1;

    monomials(4, 0) = 1;
    monomials(4, 1) = 0;
    monomials(4, 2) = 1;

    monomials(5, 0) = 0;
    monomials(5, 1) = 1;
    monomials(5, 2) = 1;

    if (order > 1) {
      const int ind[7][3] = {
        {2, 0, 0},
        {2, 0, 1},

        {0, 2, 0},
        {0, 2, 1},

        {0, 0, 2},
        {1, 0, 2},
        {0, 1, 2}
      };
      int val[3] = {0, 1, -1};
      int index = 5;
      for (int p = 2; p <= order; ++p) {
        val[2] = p;
        for (int i = 0; i < 7; ++i) {
          monomials(++index, 0) = val[ind[i][0]];
          monomials(  index, 1) = val[ind[i][1]];
          monomials(  index, 2) = val[ind[i][2]];
        }
      }

      int val0 = 1;
      int val1 = order - 1;
      for (int p = 2; p <= order; ++p) {
        monomials(++index, 0) = val0;
        monomials(  index, 1) = val1;
        monomials(  index, 2) = 0;

        monomials(++index, 0) = val0;
        monomials(  index, 1) = val1;
        monomials(  index, 2) = 1;

        ++val0;
        --val1;
      }
    }
  }
  return monomials;
}

fullMatrix<double> gmshGenerateMonomialsHexahedron(int order, bool forSerendipPoints)
{
  int nbMonomials = forSerendipPoints ? 8 + (order-1)*12 :
                                        (order+1)*(order+1)*(order+1);
  if (forSerendipPoints && !order) nbMonomials = 1;
  fullMatrix<double> monomials(nbMonomials, 3);

  monomials(0, 0) = 0;
  monomials(0, 1) = 0;
  monomials(0, 2) = 0;

  if (order > 0) {
    monomials(1, 0) = order;
    monomials(1, 1) = 0;
    monomials(1, 2) = 0;

    monomials(2, 0) = order;
    monomials(2, 1) = order;
    monomials(2, 2) = 0;

    monomials(3, 0) = 0;
    monomials(3, 1) = order;
    monomials(3, 2) = 0;

    monomials(4, 0) = 0;
    monomials(4, 1) = 0;
    monomials(4, 2) = order;

    monomials(5, 0) = order;
    monomials(5, 1) = 0;
    monomials(5, 2) = order;

    monomials(6, 0) = order;
    monomials(6, 1) = order;
    monomials(6, 2) = order;

    monomials(7, 0) = 0;
    monomials(7, 1) = order;
    monomials(7, 2) = order;

    if (order > 1) {
      int index = 8;
      for (int iedge = 0; iedge < 12; ++iedge) {
        int i0 = MHexahedron::edges_hexa(iedge, 0);
        int i1 = MHexahedron::edges_hexa(iedge, 1);

        int u_1 = (monomials(i1,0)-monomials(i0,0)) / order;
        int u_2 = (monomials(i1,1)-monomials(i0,1)) / order;
        int u_3 = (monomials(i1,2)-monomials(i0,2)) / order;

        for (int i = 1; i < order; ++i, ++index) {
          monomials(index, 0) = monomials(i0, 0) + i * u_1;
          monomials(index, 1) = monomials(i0, 1) + i * u_2;
          monomials(index, 2) = monomials(i0, 2) + i * u_3;
        }
      }

      if (!forSerendipPoints) {
        fullMatrix<double> dudv = gmshGenerateMonomialsQuadrangle(order - 2);
        dudv.add(1);

        for (int iface = 0; iface < 6; ++iface) {
          int i0 = MHexahedron::faces_hexa(iface, 0);
          int i1 = MHexahedron::faces_hexa(iface, 1);
          int i3 = MHexahedron::faces_hexa(iface, 3);

          int u[3];
          u[0] = (monomials(i1, 0) - monomials(i0, 0)) / order;
          u[1] = (monomials(i1, 1) - monomials(i0, 1)) / order;
          u[2] = (monomials(i1, 2) - monomials(i0, 2)) / order;
          int v[3];
          v[0] = (monomials(i3, 0) - monomials(i0, 0)) / order;
          v[1] = (monomials(i3, 1) - monomials(i0, 1)) / order;
          v[2] = (monomials(i3, 2) - monomials(i0, 2)) / order;

          for (int i = 0; i < dudv.size1(); ++i, ++index) {
            monomials(index, 0) = monomials(i0, 0) + u[0] * dudv(i, 0) + v[0] * dudv(i, 1);
            monomials(index, 1) = monomials(i0, 1) + u[1] * dudv(i, 0) + v[1] * dudv(i, 1);
            monomials(index, 2) = monomials(i0, 2) + u[2] * dudv(i, 0) + v[2] * dudv(i, 1);
          }
        }

        fullMatrix<double> inner = gmshGenerateMonomialsHexahedron(order - 2);
        inner.add(1);
        monomials.copy(inner, 0, nbMonomials - index, 0, 3, index, 0);
      }
    }
  }
  return monomials;
}

fullMatrix<double> gmshGenerateMonomialsHexaSerendipity(int order)
{
  int nbMonomials = order ? 8 + (order-1) * 12 : 1;
  fullMatrix<double> monomials(nbMonomials, 3);

  monomials(0, 0) = 0;
  monomials(0, 1) = 0;
  monomials(0, 2) = 0;

  if (order > 0) {
    monomials(1, 0) = 1;
    monomials(1, 1) = 0;
    monomials(1, 2) = 0;

    monomials(2, 0) = 1;
    monomials(2, 1) = 1;
    monomials(2, 2) = 0;

    monomials(3, 0) = 0;
    monomials(3, 1) = 1;
    monomials(3, 2) = 0;

    monomials(4, 0) = 0;
    monomials(4, 1) = 0;
    monomials(4, 2) = 1;

    monomials(5, 0) = 1;
    monomials(5, 1) = 0;
    monomials(5, 2) = 1;

    monomials(6, 0) = 1;
    monomials(6, 1) = 1;
    monomials(6, 2) = 1;

    monomials(7, 0) = 0;
    monomials(7, 1) = 1;
    monomials(7, 2) = 1;

    if (order > 1) {
      const int ind[12][3] = {
        {2, 0, 0},
        {2, 0, 1},
        {2, 1, 1},
        {2, 1, 0},

        {0, 2, 0},
        {0, 2, 1},
        {1, 2, 1},
        {1, 2, 0},

        {0, 0, 2},
        {0, 1, 2},
        {1, 1, 2},
        {1, 0, 2}
      };
      int val[3] = {0, 1, -1};
      int index = 7;
      for (int p = 2; p <= order; ++p) {
        val[2] = p;
        for (int i = 0; i < 12; ++i) {
          monomials(++index, 0) = val[ind[i][0]];
          monomials(  index, 1) = val[ind[i][1]];
          monomials(  index, 2) = val[ind[i][2]];
        }
      }
    }
  }
  return monomials;
}

fullMatrix<double> gmshGenerateMonomialsPyramid(int order, bool forSerendipPoints)
{
  int nbMonomials = forSerendipPoints ? 5 + (order-1)*8 :
                                        (order+1)*((order+1)+1)*(2*(order+1)+1)/6;
  if (forSerendipPoints && !order) nbMonomials = 1;
  fullMatrix<double> monomials(nbMonomials, 3);

  monomials(0, 0) = 0;
  monomials(0, 1) = 0;
  monomials(0, 2) = 0;

  if (order > 0) {
    monomials(1, 0) = order;
    monomials(1, 1) = 0;
    monomials(1, 2) = 0;

    monomials(2, 0) = order;
    monomials(2, 1) = order;
    monomials(2, 2) = 0;

    monomials(3, 0) = 0;
    monomials(3, 1) = order;
    monomials(3, 2) = 0;

    monomials(4, 0) = 0;
    monomials(4, 1) = 0;
    monomials(4, 2) = order;

    if (order > 1) {
      int index = 5;
      for (int iedge = 0; iedge < 8; ++iedge) {
        int i0 = MPyramid::edges_pyramid(iedge, 0);
        int i1 = MPyramid::edges_pyramid(iedge, 1);

        int u_1 = (monomials(i1,0)-monomials(i0,0)) / order;
        int u_2 = (monomials(i1,1)-monomials(i0,1)) / order;
        int u_3 = (monomials(i1,2)-monomials(i0,2)) / order;

        for (int i = 1; i < order; ++i, ++index) {
          monomials(index, 0) = monomials(i0, 0) + i * u_1;
          monomials(index, 1) = monomials(i0, 1) + i * u_2;
          monomials(index, 2) = monomials(i0, 2) + i * u_3;
        }
      }

      if (!forSerendipPoints) {
        fullMatrix<double> dudvQ = gmshGenerateMonomialsQuadrangle(order - 2);
        dudvQ.add(1);

        fullMatrix<double> dudvT;
        if (order > 2) {
          dudvT = gmshGenerateMonomialsTriangle(order - 3);
          dudvT.add(1);
        }

        for (int iface = 0; iface < 5; ++iface) {
          int i0, i1, i2;
          i0 = MPyramid::faces_pyramid(iface, 0);
          i1 = MPyramid::faces_pyramid(iface, 1);
          fullMatrix<double> dudv;
          if (MPyramid::faces_pyramid(iface, 3) != -1) {
            i2 = MPyramid::faces_pyramid(iface, 3);
            dudv.setAsProxy(dudvQ);
          }
          else if (order > 2) {
            i2 = MPyramid::faces_pyramid(iface, 2);
            dudv.setAsProxy(dudvT);
          }
          else continue;

          int u[3];
          u[0] = (monomials(i1, 0) - monomials(i0, 0)) / order;
          u[1] = (monomials(i1, 1) - monomials(i0, 1)) / order;
          u[2] = (monomials(i1, 2) - monomials(i0, 2)) / order;
          int v[3];
          v[0] = (monomials(i2, 0) - monomials(i0, 0)) / order;
          v[1] = (monomials(i2, 1) - monomials(i0, 1)) / order;
          v[2] = (monomials(i2, 2) - monomials(i0, 2)) / order;

          for (int i = 0; i < dudv.size1(); ++i, ++index) {
            monomials(index, 0) = monomials(i0, 0) + u[0] * dudv(i, 0) + v[0] * dudv(i, 1);
            monomials(index, 1) = monomials(i0, 1) + u[1] * dudv(i, 0) + v[1] * dudv(i, 1);
            monomials(index, 2) = monomials(i0, 2) + u[2] * dudv(i, 0) + v[2] * dudv(i, 1);
          }
        }

        if (order > 2) {
          fullMatrix<double> inner = gmshGenerateMonomialsPyramid(order - 3);
          inner.add(1);
          monomials.copy(inner, 0, nbMonomials - index, 0, 3, index, 0);
        }
      }
    }
  }
  return monomials;
}

