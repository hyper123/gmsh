#ifndef _DECASTELJAU_H_
#define _DECASTELJAU_H_
#include <vector>
class SPoint3;
void decasteljau(double tol, const SPoint3 &p0, const SPoint3 &p1, const SPoint3 &p2, std::vector<SPoint3> &pts, std::vector<double> &ts);
void decasteljau(double tol, const SPoint3 &p0, const SPoint3 &p1, const SPoint3 &p2, const SPoint3 &p3, std::vector<SPoint3> &pts, std::vector<double> &ts);
void decasteljau(double tol, const std::vector<SPoint3> &controlPoints, std::vector<SPoint3> &pts, std::vector<double> &ts);
double sqDistPointSegment(const SPoint3 &p, const SPoint3 &s0, const SPoint3 &s1);
#endif
