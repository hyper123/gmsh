// Gmsh - Copyright (C) 1997-2012 C. Geuzaine, J.-F. Remacle
//
// See the LICENSE.txt file for license information. Please report all
// bugs and problems to <gmsh@geuz.org>.

#include <stack>
#include "GmshConfig.h"
#include "meshGFaceElliptic.h"
#include "qualityMeasures.h"
#include "GFace.h"
#include "GEdge.h"
#include "GEdgeCompound.h"
#include "GVertex.h"
#include "GModel.h"
#include "MVertex.h"
#include "MTriangle.h"
#include "MQuadrangle.h"
#include "MLine.h"
#include "BackgroundMesh.h"
#include "Numeric.h"
#include "GmshMessage.h"
#include "Generator.h"
#include "Context.h"
#include "OS.h"
#include "SVector3.h"
#include "SPoint3.h"
#include "fullMatrix.h"
#include "CenterlineField.h"
#if defined(HAVE_ANN)
#include <ANN/ANN.h>
#endif

#define TRAN_QUAD(c1,c2,c3,c4,s1,s2,s3,s4,u,v) \
    (1.-u)*c4+u*c2+(1.-v)*c1+v*c3-((1.-u)*(1.-v)*s1+u*(1.-v)*s2+u*v*s3+(1.-u)*v*s4)


static void printQuads(GFace *gf, fullMatrix<SPoint2> uv, std::vector<MQuadrangle*> quads,  int iter){

  char name[234];
  sprintf(name,"quadUV_%d_%d.pos", gf->tag(), iter);
  FILE *f = fopen(name,"w");
  fprintf(f,"View \"%s\" {\n",name);

  for (unsigned int i = 0; i < uv.size1(); i++)
    for (unsigned int j = 0; j < uv.size2(); j++)
      fprintf(f,"SP(%g,%g,%g) {%d};\n", uv(i,j).x(), uv(i,j).y(), 0.0, i);

  fprintf(f,"};\n");
  fclose(f);

  char name3[234];
  sprintf(name3,"quadXYZ_%d_%d.pos", gf->tag(), iter);
  FILE *f3 = fopen(name3,"w");
  fprintf(f3,"View \"%s\" {\n",name3);
  for (unsigned int i = 0; i < quads.size(); i++){
    quads[i]->writePOS(f3,true,false,false,false,false,false);
  }
  fprintf(f3,"};\n");
  fclose(f3);

}

static void printParamGrid(GFace *gf, std::vector<MVertex*> vert1, std::vector<MVertex*> vert2,
			   std::vector<MVertex*> e01, std::vector<MVertex*> e10,
			   std::vector<MVertex*> e23, std::vector<MVertex*> e32,
			   std::vector<MVertex*> e02, std::vector<MVertex*> e13,
			   std::vector<MQuadrangle*> quads)
{

  std::vector<SPoint2> p1,p2;
  for (unsigned int i = 0; i< vert1.size(); i++){
    SPoint2 pi;
    reparamMeshVertexOnFace(vert1[i], gf, pi);
    p1.push_back(pi);
  }
  for (unsigned int j = 0; j< vert2.size(); j++){
    SPoint2 pj;
    reparamMeshVertexOnFace(vert2[j], gf, pj);
    p2.push_back(pj);
  }


  char name[234];
  sprintf(name,"paramGrid_%d.pos", gf->tag());
  FILE *f = fopen(name,"w");
  fprintf(f,"View \"%s\" {\n",name);

  for (unsigned int i = 0; i < p1.size(); i++)
    fprintf(f,"SP(%g,%g,%g) {%d};\n", p1[i].x(), p1[i].y(), 0.0, i);

  for (unsigned int j = 0; j < p2.size(); j++)
    fprintf(f,"SP(%g,%g,%g) {%d};\n", p2[j].x(), p2[j].y(), 0.0, 100+j);

  fprintf(f,"};\n");
  fclose(f);

  char name2[234];
  sprintf(name2,"paramEdges_%d.pos", gf->tag());
  FILE *f2 = fopen(name2,"w");
  fprintf(f2,"View \"%s\" {\n",name2);

  for (unsigned int i = 0; i < e01.size(); i++){
     SPoint2 pi; reparamMeshVertexOnFace(e01[i], gf, pi);
    fprintf(f2,"SP(%g,%g,%g) {%d};\n", pi.x(), pi.y(), 0.0, 1);
  }

  for (unsigned int i = 0; i < e10.size(); i++){
    SPoint2 pi; reparamMeshVertexOnFace(e10[i], gf, pi);
    fprintf(f2,"SP(%g,%g,%g) {%d};\n", pi.x(), pi.y(), 0.0, 10);
  }

  for (unsigned int i = 0; i < e23.size(); i++){
    SPoint2 pi; reparamMeshVertexOnFace(e23[i], gf, pi);
    fprintf(f2,"SP(%g,%g,%g) {%d};\n", pi.x(), pi.y(), 0.0, 23);
  }

  for (unsigned int i = 0; i < e32.size(); i++){
    SPoint2 pi; reparamMeshVertexOnFace(e32[i], gf, pi);
    fprintf(f2,"SP(%g,%g,%g) {%d};\n", pi.x(), pi.y(), 0.0, 32);
  }

  for (unsigned int i = 0; i < e02.size(); i++){
    SPoint2 pi; reparamMeshVertexOnFace(e02[i], gf, pi);
    fprintf(f2,"SP(%g,%g,%g) {%d};\n", pi.x(), pi.y(), 0.0, 2);
  }

  for (unsigned int i = 0; i < e13.size(); i++){
    SPoint2 pi; reparamMeshVertexOnFace(e13[i], gf, pi);
    fprintf(f2,"SP(%g,%g,%g) {%d};\n", pi.x(), pi.y(), 0.0, 13);
  }
  fprintf(f2,"};\n");
  fclose(f2);

  char name3[234];
  sprintf(name3,"quadXYZ_%d.pos", gf->tag());
  FILE *f3 = fopen(name3,"w");
  fprintf(f3,"View \"%s\" {\n",name3);
  for (unsigned int i = 0; i < quads.size(); i++){
    quads[i]->writePOS(f3,true,false,false,false,false,false);
  }
  fprintf(f3,"};\n");
  fclose(f3);

  return;

}

static void createQuadsFromUV(GFace *gf, fullMatrix<SPoint2> &uv,  std::vector<MQuadrangle*> &newq,  std::vector<MVertex*> &newv){

  newq.clear();
  newv.clear();

  int MM = uv.size1();
  int NN = uv.size2();
  std::vector<std::vector<MVertex*> > tab(MM);
  for(int i = 0; i < MM; i++) tab[i].resize(NN);

  for (int i = 0; i < MM; i++){
    for (int j = 0; j < NN; j++){
      GPoint gp = gf->point(uv(i,j));
      if (!gp.succeeded()) printf("** AH new vertex not created p=%g %g \n", uv(i,j).x(), uv(i,j).y());
      MVertex *vnew = new MFaceVertex(gp.x(),gp.y(),gp.z(),gf,gp.u(),gp.v());
      tab[i][j] = vnew;
      if (i != 0 && j != 0 && i != uv.size1()-1 && j != uv.size2()-1)
	newv.push_back(vnew);
    }
  }

  // create quads
  for (int i=0;i<MM-1;i++){
    for (int j=0;j<NN-1;j++){
      MQuadrangle *qnew = new MQuadrangle (tab[i][j],tab[i][j+1],tab[i+1][j+1],tab[i+1][j]);
      newq.push_back(qnew);
    }
  }

  return;

}
static std::vector<MVertex*> saturateEdgeRegular (GFace *gf, SPoint2 p1, SPoint2 p2,
						  double H,  double L,
						  int M, std::vector<SPoint2> &pe){

  std::vector<MVertex*> pts;
  for (int i=1;i<M;i++){
    //double s = ((double)i/((double)(M)));

    double y = ((double)(i))*H/M;
    double Yy = cosh(M_PI*y/L) - tanh(M_PI*H/L)*sinh(M_PI*y/L);
    double YyH = cosh(M_PI*H/L) - tanh(M_PI*H/L)*sinh(M_PI*H/L);
    double s = (1 - Yy)/(1.-YyH);

    SPoint2 p = p1 + (p2-p1)*s;
    pe.push_back(p);

    GPoint gp = gf->point(p);
    if (!gp.succeeded()) printf("** AH new vertex not created p=%g %g \n", p.x(), p.y());
    MVertex *v = new MFaceVertex(gp.x(),gp.y(),gp.z(),gf,gp.u(),gp.v());

    if (!v){ pts.clear(); pts.resize(0); return pts;}
    pts.push_back(v);
  }
  return pts;
}

//elliptic surface grid generator
//see eq. 9.26 page 9-24 in handbook of grid generation
static void ellipticSmoother(fullMatrix<SPoint2> uv, GFace* gf,
			     std::vector<MQuadrangle*> &newq,
			     std::vector<MVertex*> &newv){

  int nbSmooth = 100;
  int N = uv.size1();
  int M = uv.size2();

  for (int k = 0; k< nbSmooth; k++){
    double norm = 0.0;
    for (int i=1; i<N-1; i++){
      for (int j = 1; j<M-1; j++){
	Pair<SVector3, SVector3> der = gf->firstDer(uv(i,j));
	SVector3 du = der.first();
	SVector3 dv = der.second();
	double g11 = dot(du,du);
	double g12 = dot(du,dv);
	double g22 = dot(dv,dv);
	double over =  1./(4.*(g11+g22));
	double uk = over*(2.*g22*(uv(i+1,j).x()+uv(i-1,j).x())+
			  2.*g11*(uv(i,j+1).x()+uv(i,j-1).x())-
			  1.*g12*(uv(i+1,j+1).x()-uv(i-1,j+1).x()));
	double vk = over*(2.*g22*(uv(i+1,j).y()+uv(i-1,j).y())+
			  2.*g11*(uv(i,j+1).y()+uv(i,j-1).y())-
			  1.*g12*(uv(i+1,j+1).y()-uv(i-1,j+1).y()));
	norm += sqrt((uv(i,j).x()-uk)*(uv(i,j).x()-uk)+(uv(i,j).y()-vk)*(uv(i,j).y()-vk));
	uv(i,j) = SPoint2(uk,vk);
      }
    }
    //printf("Elliptic smoother iter (%d) = %g \n", k, norm);
    printQuads(gf, uv, newq, k+1);
  }

  createQuadsFromUV(gf, uv, newq, newv);
  printQuads(gf, uv, newq, 100);

}

//create initial grid points MxN using transfinite interpolation
/*
  c4          N              c3
  +--------------------------+
  |       |      |    |      |
  |       |      |    |      |
  +--------------------------+  M
  |       |      |    |      |
  |       |      |    |      |
  +--------------------------+
 c1                          c2
             N
*/
static void createRegularGrid (GFace *gf,
			       MVertex *v1, SPoint2 &c1,
			       std::vector<MVertex*> &e12, std::vector<SPoint2> &pe12, int sign12,
			       MVertex *v2, SPoint2 &c2,
			       std::vector<MVertex*> &e23, std::vector<SPoint2> &pe23, int sign23,
			       MVertex *v3, SPoint2 &c3,
			       std::vector<MVertex*> &e34,std::vector<SPoint2> &pe34, int sign34,
			       MVertex *v4, SPoint2 &c4,
			       std::vector<MVertex*> &e41, std::vector<SPoint2> &pe41,int sign41,
			       std::vector<MQuadrangle*> &q,
			       std::vector<MVertex*> &newv,
			       fullMatrix<SPoint2> &uv)
{
  int N = e12.size();
  int M = e23.size();

  uv.resize(M+2,N+2);
  printf("uv =%d %d \n", uv.size1(), uv.size2());

  char name3[234];
  sprintf(name3,"quadParam_%d.pos", gf->tag());
  FILE *f3 = fopen(name3,"w");
  fprintf(f3,"View \"%s\" {\n",name3);

  std::vector<std::vector<MVertex*> > tab(M+2);
  for(int i = 0; i <= M+1; i++) tab[i].resize(N+2);

  tab[0][0] = v1;
  tab[0][N+1] = v2;
  tab[M+1][N+1] = v3;
  tab[M+1][0] = v4;
  uv(0,0) = c1;
  uv(0,N+1) = c2;
  uv(M+1,N+1) = c3;
  uv(M+1,0) = c4;
  for (int i=0;i<N;i++){
    tab[0][i+1]   = sign12 > 0 ? e12 [i] : e12 [N-i-1];
    tab[M+1][i+1] = sign34 < 0 ? e34 [i] : e34 [N-i-1];
    uv(0,i+1)   = sign12 > 0 ? pe12 [i] : pe12 [N-i-1];
    uv(M+1,i+1) = sign34 < 0 ? pe34 [i] : pe34 [N-i-1];
  }
  for (int i=0;i<M;i++){
    tab[i+1][N+1] = sign23 > 0 ? e23 [i] : e23 [M-i-1];
    tab[i+1][0]   = sign41 < 0 ? e41 [i] : e41 [M-i-1];
    uv(i+1,N+1) = sign23 > 0 ? pe23 [i] : pe23 [M-i-1];
    uv(i+1,0)   = sign41 < 0 ? pe41 [i] : pe41 [M-i-1];
  }

  for (int i=0;i<N;i++){
    for (int j=0;j<M;j++){
      double u = (double) (i+1)/ ((double)(N+1));
      double v = (double) (j+1)/ ((double)(M+1));
      MVertex *v12 = (sign12 >0) ? e12[i] : e12 [N-1-i];
      MVertex *v23 = (sign23 >0) ? e23[j] : e23 [M-1-j];
      MVertex *v34 = (sign34 <0) ? e34[i] : e34 [N-1-i];
      MVertex *v41 = (sign41 <0) ? e41[j] : e41 [M-1-j];
      SPoint2 p12; reparamMeshVertexOnFace(v12, gf, p12);
      SPoint2 p23; reparamMeshVertexOnFace(v23, gf, p23);
      SPoint2 p34; reparamMeshVertexOnFace(v34, gf, p34);
      SPoint2 p41; reparamMeshVertexOnFace(v41, gf, p41);
      double Up = TRAN_QUAD(p12.x(), p23.x(), p34.x(), p41.x(),
			   c1.x(),c2.x(),c3.x(),c4.x(),u,v);
      double Vp = TRAN_QUAD(p12.y(), p23.y(), p34.y(), p41.y(),
			   c1.y(),c2.y(),c3.y(),c4.y(),u,v);
      fprintf(f3,"SP(%g,%g,%g) {%d};\n", Up, Vp, 0.0, 1);

      if ((p12.x() && p12.y() == -1.0) || (p23.x() && p23.y() == -1.0) ||
          (p34.x() && p34.y() == -1.0) || (p41.x() && p41.y() == -1.0)) {
        Msg::Error("Wrong param -1");
        return;
      }

      SPoint2 pij(Up,Vp);

      GPoint gp = gf->point(pij);
      if (!gp.succeeded()) printf("** AH new vertex not created p=%g %g \n", Up, Vp);
      MVertex *vnew = new MFaceVertex(gp.x(),gp.y(),gp.z(),gf,gp.u(),gp.v());
      newv.push_back(vnew);

      uv(j+1,i+1) = pij;
      tab[j+1][i+1] = vnew;
    }
  }

  // create quads
  for (int i=0;i<=N;i++){
    for (int j=0;j<=M;j++){
      MQuadrangle *qnew = new MQuadrangle (tab[j][i],tab[j][i+1],tab[j+1][i+1],tab[j+1][i]);
      q.push_back(qnew);
    }
  }

  fprintf(f3,"};\n");
  fclose(f3);

}

static void updateFaceQuads(GFace *gf, std::vector<MQuadrangle*> &quads, std::vector<MVertex*> &newv){

  printf("face has before %d quads %d tris  %d vert \n ", gf->quadrangles.size(), gf->triangles.size(), gf->mesh_vertices.size());
  for (unsigned int i = 0; i < quads.size(); i++){
    gf->quadrangles.push_back(quads[i]);
    for (int j=0;j<4;j++){
      MVertex *v = quads[i]->getVertex(j);
    }
  }
  for(int i = 0; i < newv.size(); i++){
    gf->mesh_vertices.push_back(newv[i]);
  }
  printf("face has after %d quads %d tris   %d vert  \n ", gf->quadrangles.size(), gf->triangles.size(), gf->mesh_vertices.size());

}

bool createRegularTwoCircleGrid (Centerline *center, GFace *gf)
{
#if defined(HAVE_ANN)
  std::list<GEdge*> bedges = gf->edges();
  std::list<GEdge*>::iterator itb = bedges.begin();
  std::list<GEdge*>::iterator ite = bedges.end(); ite--;
  GEdge *ge1 =  (*itb)->getCompound();
  GEdge *ge2 =  (*ite)->getCompound();
  int N = ge1->mesh_vertices.size() + 1;
  int N2 = ge2->mesh_vertices.size() + 1;
  if (N != N2 || N%2 != 0){
    Msg::Error("You should have an equal pair number of points in centerline field N =%d N2=%d\n", N, N2);
    return false;
  }

  //vert1 is the outer circle
  //vert2 is the inner circle
  //         - - - -
  //       -         -
  //     -      -      -
  //   v0-  v2-   -v3   -v1
  //     -      -      -
  //      -          -
  //         - - - -

  std::vector<MVertex*> vert1, vert2, vert_temp;
  vert1.push_back(ge1->getBeginVertex()->mesh_vertices[0]);
  vert2.push_back(ge2->getBeginVertex()->mesh_vertices[0]);
  for(int i = 0; i < N-1; i++) {
    vert1.push_back(ge1->mesh_vertices[i]);
    vert2.push_back(ge2->mesh_vertices[i]);
  }
  SPoint2 pv1; reparamMeshVertexOnFace(vert1[0], gf, pv1);
  SPoint2 pv2; reparamMeshVertexOnFace(vert2[0], gf, pv2);
  SPoint2 pv1b; reparamMeshVertexOnFace(vert1[1], gf, pv1b);
  SPoint2 pv2b; reparamMeshVertexOnFace(vert2[1], gf, pv2b);
  SPoint2 pv1c; reparamMeshVertexOnFace(vert1[2], gf, pv1c);
  SPoint2 pv2c; reparamMeshVertexOnFace(vert2[2], gf, pv2c);
  SVector3 vec1(pv1b.x()-pv1.x(),pv1b.y()-pv1.y() , 0.0);
  SVector3 vec1b(pv1c.x()-pv1b.x(),pv1c.y()-pv1b.y() ,0.0);
  SVector3 vec2(pv2b.x()-pv2.x(),pv2b.y()-pv2.y() ,0.0);
  SVector3 vec2b(pv2c.x()-pv2b.x(),pv2c.y()-pv2b.y() , 0.0);
  int sign2 =  (dot(crossprod(vec1,vec1b),crossprod(vec2,vec2b)) < 0)  ?   -1 : +1 ;
  double n1 = sqrt(pv1.x()*pv1.x()+pv1.y()*pv1.y());
  double n2 = sqrt(pv2.x()*pv2.x()+pv2.y()*pv2.y());
  if (n2 > n1) {
    vert_temp = vert1;
    vert1.clear();
    vert1 = vert2;
    vert2.clear();
    vert2 = vert_temp;
  }

  ANNpointArray nodes = annAllocPts(N, 3);
  ANNidxArray index  = new ANNidx[1];
  ANNdistArray dist = new ANNdist[1];
  for (int ind = 0; ind < N; ind++){
    SPoint2 pp2; reparamMeshVertexOnFace(vert2[ind], gf, pp2);
    nodes[ind][0] =  pp2.x();
    nodes[ind][1] =  pp2.y();
    nodes[ind][2] =  0.0;
  }
  ANNkd_tree *kdtree = new ANNkd_tree(nodes, N, 3);
  SPoint2 pp1; reparamMeshVertexOnFace(vert1[0], gf, pp1);
  double xyz[3] = {pp1.x(), pp1.y(),0.0};
  kdtree->annkSearch(xyz, 1, index, dist);
  int close_ind = index[0];
  double length = sqrt((vert1[0]->x()-vert2[close_ind]->x())*(vert1[0]->x()-vert2[close_ind]->x())+
		       (vert1[0]->y()-vert2[close_ind]->y())*(vert1[0]->y()-vert2[close_ind]->y())+
		       (vert1[0]->z()-vert2[close_ind]->z())*(vert1[0]->z()-vert2[close_ind]->z()));

  double rad = center->operator()(vert1[0]->x(), vert1[0]->y(), vert1[0]->z());
  double arc = 2*M_PI*rad;
  double lc =  arc/N;
  int M = length/lc;
  printf("length =%g  arc=%g  \n", length,  arc);

  delete kdtree;
  delete[]index;
  delete[]dist;
  annDeallocPts(nodes);

  MVertex *v0 = vert1[0]; SPoint2 p0; reparamMeshVertexOnFace(v0, gf, p0);
  MVertex *v1 = vert1[N/2]; SPoint2 p1; reparamMeshVertexOnFace(v1, gf, p1);
  MVertex *v2 = vert2[close_ind]; SPoint2 p2; reparamMeshVertexOnFace(v2, gf, p2);
  MVertex *v3 = vert2[(close_ind+N/2)%N]; SPoint2 p3; reparamMeshVertexOnFace(v3, gf, p3);

  printf("grid N = %d M = %d\n", N , M);
  //printf("v2 =%d v3=%d \n", close_ind, (close_ind+N/2)%N);
  std::vector<MVertex*> e01,e10,e23,e32;//edges without first and last vertex
  for (int i=1;i<N/2;i++) e01.push_back(vert1[i]);
  for (int i=N/2+1;i<N;i++) e10.push_back(vert1[i]);
  for (int i=1;i<N/2;i++) e23.push_back(vert2[(close_ind+i)%N]);
  for (int i=N/2+1;i<N;i++) e32.push_back(vert2[(close_ind+i)%N]);

  std::vector<SPoint2> pe01, pe10, pe23, pe32;
  for (unsigned int i = 0; i < e01.size(); i++){
     SPoint2 p01; reparamMeshVertexOnFace(e01[i], gf, p01);
     SPoint2 p10; reparamMeshVertexOnFace(e10[i], gf, p10);
     SPoint2 p23; reparamMeshVertexOnFace(e23[i], gf, p23);
     SPoint2 p32; reparamMeshVertexOnFace(e32[i], gf, p32);
     pe01.push_back(p01);
     pe10.push_back(p10);
     pe23.push_back(p23);
     pe32.push_back(p32);
  }

  std::vector<SPoint2> pe02, pe13;
  std::vector<MVertex*> e02 = saturateEdgeRegular (gf,p0,p2,length, arc, M+1, pe02);
  std::vector<MVertex*> e13 = saturateEdgeRegular (gf,p1,p3,length, arc, M+1, pe13);

  printf("N/2 edges size =%d %d \n", pe01.size(), pe10.size());
  printf("M edges size =%d %d \n", pe02.size(), pe13.size());

  std::vector<MVertex*> e_inner1 = e23;
  std::vector<MVertex*> e_inner2 = e32;
  std::vector<SPoint2> pe_inner1 = pe23;
  std::vector<SPoint2> pe_inner2 = pe32;
  if (sign2 <0){
    e_inner1 = e32;
    e_inner2 = e23;
    pe_inner1 = pe32;
    pe_inner2 = pe23;
  }

  std::vector<MQuadrangle*> quads, quadS1, quadS2;
  std::vector<MVertex*> newv, newv1, newv2;
  fullMatrix<SPoint2> uv1, uv2;
  createRegularGrid (gf,
  		     v0, p0,
  		     e01, pe01, +1,
  		     v1,p1,
  		     e13, pe13, +1,
  		     v3,p3,
		     e_inner1, pe_inner1, -sign2,
  		     v2,p2,
  		     e02, pe02, -1,
  		     quads, newv, uv1);

  //updateFaceQuads(gf, quads, newv);
  //printQuads(gf, uv1, quads, 0);
  ellipticSmoother(uv1, gf, quadS1, newv1);

  createRegularGrid (gf,
  		     v0,p0,
  		     e02, pe02,  +1,
  		     v2,p2,
  		     e_inner2, pe_inner2, -sign2,
  		     v3,p3,
  		     e13, pe13, -1,
  		     v1,p1,
  		     e10, pe10, +1,
  		     quads, newv, uv2);

  ellipticSmoother(uv2, gf, quadS2, newv2);

  quads.clear();
  for (int i= 0; i< quadS1.size(); i++) quads.push_back(quadS1[i]);
  for (int i= 0; i< quadS2.size(); i++) quads.push_back(quadS2[i]);
  printParamGrid(gf, vert1, vert2, e01,e10,e23,e32,e02,e13, quads);

#endif;
  return true;

}

