#ifndef Mu2eUtilities_Random2Dpair_hh
#define Mu2eUtilities_Random2Dpair_hh

//
// Return CLHEP::Hep3Vector objects that are unit vectors uniformly
// distributed over the unit sphere.
//
// $Id: Random2Dpair.hh,v 1.1 2014/02/05 21:32:26 knoepfel Exp $
// $Author: knoepfel $
// $Date: 2014/02/05 21:32:26 $
//
// Original author Rob Kutschke
//
// Allows for range limitations on cos(theta) and phi.
//
//

#include <cmath>
#include <iostream>

#include "CLHEP/Random/RandFlat.h"
#include "CLHEP/Random/RandomEngine.h"
#include "CLHEP/Units/PhysicalConstants.h"
#include "CLHEP/Vector/ThreeVector.h"

namespace mu2e {

  template <class FunctionClass>
  class Random2Dpair {

  public:

    explicit Random2Dpair( CLHEP::HepRandomEngine& engine,
                           double xmin=0.,
                           double xmax=1.,
                           double ymin=0.,
                           double ymax=1. ) :
      _xmin(xmin),
      _xmax(xmax),
      _ymin(ymin),
      _ymax(ymax),
      _randFlat( engine ) {}

    // I have inspected RandFlat source and assert that its destructor does not throw
    ~Random2Dpair() noexcept {}

    // Includes optional arguments that (*_function) accepts
    template<typename... Args>
    std::pair<double,double> fire( Args... args ) {

      // Accept/reject method
      double prob(0.), threshold(0.), x(0.), y(0.);

      do {
        x = _xmin + ( _xmax - _xmin )*_randFlat.fire();
        y = _ymin + ( _ymax - _ymin )*_randFlat.fire();
        
        static const double pdfMax = _pdf.get2DMax( std::forward<Args>(args)...);
        threshold = _pdf.get2DWeight( x, y, std::forward<Args>(args)... );
        prob      = pdfMax*_randFlat.fire();

      } while ( prob > threshold );
      
      return std::make_pair( x, y );

    }

    CLHEP::HepRandomEngine& engine(){
      return _randFlat.engine();
    }

  private:

    double _xmin;
    double _xmax;
    double _ymin;
    double _ymax;

    FunctionClass _pdf;

    // An underlying uniform random number distribution.
    CLHEP::RandFlat _randFlat;

  };

}

#endif /* Mu2eUtilities_Random2Dpair_hh */