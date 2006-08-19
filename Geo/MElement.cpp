// $Id: MElement.cpp,v 1.10 2006-08-19 18:48:06 geuzaine Exp $
//
// Copyright (C) 1997-2006 C. Geuzaine, J.-F. Remacle
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
// 
// Please report all bugs and problems to <gmsh@geuz.org>.

#include <math.h>
#include "MElement.h"
#include "GEntity.h"
#include "Numeric.h"

int MElement::_globalNum = 0;

static double dist(MVertex *v1, MVertex *v2)
{
  double dx = v1->x() - v2->x();
  double dy = v1->y() - v2->y();
  double dz = v1->z() - v2->z();
  return sqrt(dx * dx + dy * dy + dz * dz);
}

double MElement::minEdge()
{
  double m = 1.e25;
  for(int i = 0; i < getNumEdges(); i++){
    MEdge e = getEdge(i);
    m = std::min(m, dist(e.getVertex(0), e.getVertex(1)));
  }
  return m;
}

double MElement::maxEdge()
{
  double m = 0.;
  for(int i = 0; i < getNumEdges(); i++){
    MEdge e = getEdge(i);
    m = std::max(m, dist(e.getVertex(0), e.getVertex(1)));
  }
  return m;
}

double MElement::rhoShapeMeasure()
{
  double min = minEdge();
  double max = maxEdge();
  if(max)
    return min / max;
  else
    return 0.;
}

double MTetrahedron::gammaShapeMeasure()
{
  double p0[3] = { _v[0]->x(), _v[0]->y(), _v[0]->z() };
  double p1[3] = { _v[1]->x(), _v[1]->y(), _v[1]->z() };
  double p2[3] = { _v[2]->x(), _v[2]->y(), _v[2]->z() };
  double p3[3] = { _v[3]->x(), _v[3]->y(), _v[3]->z() };
  double s1 = fabs(triangle_area(p0, p1, p2));
  double s2 = fabs(triangle_area(p0, p2, p3));
  double s3 = fabs(triangle_area(p0, p1, p3));
  double s4 = fabs(triangle_area(p1, p2, p3));
  double rhoin = 3. * fabs(getVolume()) / (s1 + s2 + s3 + s4);
  return 12. * rhoin / (sqrt(6.) * maxEdge());
}

double MTetrahedron::etaShapeMeasure()
{
  double lij2 = 0.;
  for(int i = 0; i <= 3; i++) {
    for(int j = i + 1; j <= 3; j++) {
      double lij = dist(_v[i], _v[j]);
      lij2 += lij * lij;
    }
  }
  double v = fabs(getVolume());
  return 12. * pow(0.9 * v * v, 1./3.) / lij2;
}

SPoint3 MElement::barycenter()
{
  SPoint3 p(0., 0., 0.);
  int n = getNumVertices();
  for(int i = 0; i < n; i++) {
    MVertex *v = getVertex(i);
    p[0] += v->x();
    p[1] += v->y();
    p[2] += v->z();
  }
  p[0] /= (double)n;
  p[1] /= (double)n;
  p[2] /= (double)n;
  return p;
}

void MElement::writeMSH(FILE *fp, double version, bool binary, int num, 
			int elementary, int physical)
{
  // if necessary, change the ordering of the vertices to get positive
  // volume
  setVolumePositive();

  int n = getNumVertices();
  int type = getTypeForMSH();

  if(!binary){
    fprintf(fp, "%d %d", num ? num : _num, type);
    if(version < 2.0)
      fprintf(fp, " %d %d %d", physical, elementary, n);
    else
      fprintf(fp, " 3 %d %d %d", physical, elementary, _partition);
  }
  else{
    int tags[4] = {num ? num : _num, physical, elementary, _partition};
    fwrite(tags, sizeof(int), 4, fp);
  }

  int verts[30];

  if(physical >= 0){
    for(int i = 0; i < n; i++)
      verts[i] = getVertex(i)->getNum();
  }
  else{
    int nn = n - getNumEdgeVertices() - getNumFaceVertices() - getNumVolumeVertices();
    int j = 0;
    for(int i = 0; i < nn; i++)
      verts[j++] = getVertex(nn - i - 1)->getNum();
    int ne = getNumEdgeVertices();
    for(int i = 0; i < ne; i++)
      verts[j++] = getVertex(nn + ne - i - 1)->getNum();
    int nf = getNumFaceVertices();
    for(int i = 0; i < nf; i++)
      verts[j++] = getVertex(nn + ne + nf - i - 1)->getNum();
    int nv = getNumVolumeVertices();
    for(int i = 0; i < nv; i++)
      verts[j++] = getVertex(n - i - 1)->getNum();
  }

  if(!binary){
    for(int i = 0; i < n; i++)
      fprintf(fp, " %d", verts[i]);
    fprintf(fp, "\n");
  }
  else{
    fwrite(verts, sizeof(int), n, fp);
  }
}

void MElement::writePOS(FILE *fp, double scalingFactor, int elementary)
{
  int n = getNumVertices();
  double gamma = gammaShapeMeasure();
  double eta = etaShapeMeasure();
  double rho = rhoShapeMeasure();

  fprintf(fp, "%s(", getStringForPOS());
  for(int i = 0; i < n; i++){
    if(i) fprintf(fp, ",");
    fprintf(fp, "%g,%g,%g", getVertex(i)->x() * scalingFactor, 
	    getVertex(i)->y() * scalingFactor, getVertex(i)->z() * scalingFactor);
  }
  fprintf(fp, "){");
  for(int i = 0; i < n; i++)
    fprintf(fp, "%d,", elementary);
  for(int i = 0; i < n; i++)
    fprintf(fp, "%d,", getNum());
  for(int i = 0; i < n; i++)
    fprintf(fp, "%g,", gamma);
  for(int i = 0; i < n; i++)
    fprintf(fp, "%g,", eta);
  for(int i = 0; i < n; i++){
    if(i == n - 1)
      fprintf(fp, "%g", rho);
    else
      fprintf(fp, "%g,", rho);
  }
  fprintf(fp, "};\n");
}

void MElement::writeSTL(FILE *fp, bool binary, double scalingFactor)
{
  if(getNumEdges() != 3 && getNumEdges() != 4) return;
  int qid[3] = {0, 2, 3};
  SVector3 n = getFace(0).normal();
  if(!binary){
    fprintf(fp, "facet normal %g %g %g\n", n[0], n[1], n[2]);
    fprintf(fp, "  outer loop\n");
    for(int j = 0; j < 3; j++)
      fprintf(fp, "    vertex %g %g %g\n", 
	      getVertex(j)->x() * scalingFactor, 
	      getVertex(j)->y() * scalingFactor, 
	      getVertex(j)->z() * scalingFactor);
    fprintf(fp, "  endloop\n");
    fprintf(fp, "endfacet\n");
    if(getNumVertices() == 4){
      fprintf(fp, "facet normal %g %g %g\n", n[0], n[1], n[2]);
      fprintf(fp, "  outer loop\n");
      for(int j = 0; j < 3; j++)
	fprintf(fp, "    vertex %g %g %g\n", 
		getVertex(qid[j])->x() * scalingFactor, 
		getVertex(qid[j])->y() * scalingFactor, 
		getVertex(qid[j])->z() * scalingFactor);
      fprintf(fp, "  endloop\n");
      fprintf(fp, "endfacet\n");
    }
  }
  else{
    char data[50];
    float *coords = (float*)data;
    coords[0] = n[0];
    coords[1] = n[1];
    coords[2] = n[2];
    for(int j = 0; j < 3; j++){
      coords[3 + 3 * j] = getVertex(j)->x() * scalingFactor;
      coords[3 + 3 * j + 1] = getVertex(j)->y() * scalingFactor;
      coords[3 + 3 * j + 2] = getVertex(j)->z() * scalingFactor;
    }
    fwrite(data, sizeof(char), 50, fp);
    if(getNumVertices() == 4){
      for(int j = 0; j < 3; j++){
	coords[3 + 3 * j] = getVertex(qid[j])->x() * scalingFactor;
	coords[3 + 3 * j + 1] = getVertex(qid[j])->y() * scalingFactor;
	coords[3 + 3 * j + 2] = getVertex(qid[j])->z() * scalingFactor;
      }
      fwrite(data, sizeof(char), 50, fp);
    }
  }
}

void MElement::writeVRML(FILE *fp)
{
  for(int i = 0; i < getNumVertices(); i++)
    fprintf(fp, "%d,", getVertex(i)->getNum() - 1);
  fprintf(fp, "-1,\n");
}

void MElement::writeUNV(FILE *fp, int elementary)
{
  // if necessary, change the ordering of the vertices to get positive
  // volume
  setVolumePositive();

  int n = getNumVertices();
  int type = getTypeForUNV();

  fprintf(fp, "%10d%10d%10d%10d%10d%10d\n",
	  _num, type, elementary, elementary, 7, n);
  if(type == 21 || type == 24) // BEAM or BEAM2
    fprintf(fp, "%10d%10d%10d\n", 0, 0, 0);
  for(int k = 0; k < n; k++) {
    fprintf(fp, "%10d", getVertex(k)->getNum());
    if(k % 8 == 7)
      fprintf(fp, "\n");
  }
  if(n - 1 % 8 != 7)
    fprintf(fp, "\n");
}

void MElement::writeMESH(FILE *fp, int elementary)
{
  for(int i = 0; i < getNumVertices(); i++)
    fprintf(fp, " %d", getVertex(i)->getNum());
  fprintf(fp, " %d\n", elementary);
}
