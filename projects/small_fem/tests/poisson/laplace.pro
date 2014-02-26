Group{
  Cathode = Region[5];
  Anode   = Region[6];
  OmegaE  = Region[7];
}

Function{
  F[] = 1;
  D[] = TensorDiag[1, 2, 0.];
}

Constraint{
  { Name ElectricScalarPotential ;
    Case {
      { Region Anode    ; Value 0/*-1.*/ ; }
      { Region Cathode  ; Value 0/*+2.*/ ; }
    }
  }
}

Jacobian {
  { Name JVol ;
    Case {
      { Region All ; Jacobian Vol ; }
    }
  }
}

Integration {
  { Name I1 ;
    Case { {Type Gauss ;
            Case { { GeoElement Triangle    ; NumberOfPoints  4 ; }
                   { GeoElement Quadrangle  ; NumberOfPoints  4 ; }
                   { GeoElement Tetrahedron ; NumberOfPoints  4 ; }
                   { GeoElement Hexahedron  ; NumberOfPoints  6 ; }
                   { GeoElement Prism       ; NumberOfPoints  9 ; } }
           }
         }
  }
}

FunctionSpace {
  { Name Hgrad_v ; Type Form0 ;
    BasisFunction {
      { Name rn ; NameOfCoef vn ; Function BF_Node ;
        Support OmegaE ; Entity NodesOf[ All] ; }
    }
    Constraint {
      { NameOfCoef vn ; EntityType NodesOf ; NameOfConstraint ElectricScalarPotential ; }
    }
  }
}

Formulation {
  { Name ElectricScalar ; Type FemEquation ;
    Quantity {
      { Name v ; Type Local  ; NameOfSpace Hgrad_v ; }
    }
    Equation {
      Galerkin { [-D[] * Dof{Grad v} , {Grad v}] ;
                 In OmegaE ; Jacobian JVol ; Integration I1 ; }

      Galerkin { [F[] , {v}] ;
                 In OmegaE ; Jacobian JVol ; Integration I1 ; }
    }
  }
}

Resolution {
  { Name ElectricScalar ;
    System {
      { Name B ; NameOfFormulation ElectricScalar ; }
    }
    Operation { Generate[B] ; Solve[B] ; SaveSolution[B] ; }
  }
}

PostProcessing {
  { Name ElectricScalar ; NameOfFormulation ElectricScalar ;
    Quantity {
      { Name laplaceRef ; Value { Local { [{v}] ; In OmegaE ; Jacobian JVol ; } } }
    }
  }
}


PostOperation {
  { Name ElectricScalar ; NameOfPostProcessing ElectricScalar ;
    Operation {
      Print [ laplaceRef, OnElementsOf OmegaE, File "laplace.pos" ] ;
    }
  }
}
