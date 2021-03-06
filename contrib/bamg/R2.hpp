#ifndef R2_HPP
#define  R2_HPP
#include <cmath>
#include <cstdlib>
#include <iostream>
// Definition de la class R2 
//  sans compilation separe toute les fonctions 
// sous defini dans ce R2.hpp avec des inline 
//  
// definition R (les nombres reals)
// remarque la fonction abort est defini dans 
// #include <cstdlib> 
typedef double R;

// The class R2
class R2 {
public:  
  typedef double R;
  static const int d=2;

  R x,y;  // declaration de membre 
  // les 3 constructeurs ---
  R2 () :x(0.),y(0.) {} // rappel : x(0), y(0)  sont initialiser via le constructeur de double 
  R2 (R a,R b):x(a),y(b)  {}
  R2 (const R2 & a,const R2 & b):x(b.x-a.x),y(b.y-a.y)  {}
  // le constucteur par defaut est inutile
  R2 (const R2 & a) :x(a.x),y(a.y) {} 

  // rappel les operator definis dans une class on un parametre
  // cache qui est la classe elle meme (*this)

  // les operateurs affectation
  //  operateur affection (*this) = P est inutil par defaut il fait le travail correctement
  R2 &  operator=(const R2 & P)  {x = P.x;y = P.y;return *this;}
  // les autre operoteur affectations
  R2 &  operator+=(const R2 & P)  {x += P.x;y += P.y;return *this;}
  R2 &  operator-=(const R2 & P) {x -= P.x;y -= P.y;return *this;}
  // operateur binaire + - * , ^ /
  R2   operator+(const R2 & P)const   {return R2(x+P.x,y+P.y);}
  R2   operator-(const R2 & P)const   {return R2(x-P.x,y-P.y);}
  R    operator,(const R2 & P)const  {return  x*P.x+y*P.y;} // produit scalaire
  R    operator^(const R2 & P)const {return  x*P.y-y*P.x;} // produit mixte
  R2   operator*(R c)const {return R2(x*c,y*c);}
  R2   operator/(R c)const {return R2(x/c,y/c);}
  // operateur unaire 
  R2   operator-()const  {return R2(-x,-y);} 
  R2   operator+()const  {return *this;}
  // un methode
  R2   perp() const {return R2(-y,x);} // la perpendiculaire
  // les operators  tableau
  // version qui peut modifie la class  via l'adresse de x ou y 
  R  &  operator[](int i){ return (&x)[i];}
  const R  &  operator[](int i) const { return (&x)[i];}


  R norme() const { return std::sqrt(x*x+y*y);}
  R norme2() const { return (x*x+y*y);}

friend  R2 operator*(R c,const R2 & P) {return P*c;} 
friend  R2 perp(const R2 & P) { return R2(-P.y,P.x) ; }
//inline R2 Perp(const R2 & P) { return P.perp(); }  // autre ecriture  de la fonction perp
friend R  det(const R2 & A,const R2 & B,const R2 &C) { return R2(A,B)^R2(A,C);}

friend  std::ostream& operator <<(std::ostream& f, const R2 & P )
       { f << P.x << ' ' << P.y   ; return f; }
friend  std::istream& operator >>(std::istream& f,  R2 & P)
       { f >>  P.x >>  P.y  ; return f; }
};

#endif

