#ifndef BeamlineGeom_TorusSection_hh
#define BeamlineGeom_TorusSection_hh

//
// Class to represent the transport solenoid
//
#include <array>
#include <memory>

#include "GeomPrimitives/inc/Torus.hh"
#include "BeamlineGeom/inc/TSSection.hh"

#include "CLHEP/Vector/Rotation.h"
#include "CLHEP/Vector/ThreeVector.h"

namespace mu2e {

  class TorusSection : public TSSection {

  friend class BeamlineMaker;

  public:

    TorusSection() : 
      _rTorus(0.),_rIn(0.),_rOut(0.),
      _phiBegin(0.),_deltaPhi(0.) 
    {
      fillData();
    }


    TorusSection(double rTorus, double rIn, double rOut, double phi0, double dPhi, 
                 CLHEP::Hep3Vector origin, CLHEP::HepRotation rotation = CLHEP::HepRotation()) :
      _rTorus(rTorus),_rIn(rIn),_rOut(rOut),
      _phiBegin(phi0),_deltaPhi(dPhi)
    {
      fillData();
      _origin=origin; // _origin is derived data member; cannot be in initialization list
      _rotation=rotation;
    }
    
    TorusSection(const Torus& torus) :
      _rTorus(torus.torusRadius()),
      _rIn(torus.innerRadius()),
      _rOut(torus.outerRadius()),
      _phiBegin(torus.phi0()),
      _deltaPhi(torus.deltaPhi())
    {
      fillData();
      _origin=torus.originInMu2e(); // _origin is derived data member; cannot be in initialization list
      _rotation=torus.rotation();
    }

    ~TorusSection(){}

    void set(double rTorus, double rIn, double rOut, double phi0, double dPhi, 
             CLHEP::Hep3Vector origin, CLHEP::HepRotation rotation = CLHEP::HepRotation()) {
      _rTorus  =rTorus;
      _rIn     =rIn;
      _rOut    =rOut;
      _phiBegin=phi0;
      _deltaPhi=dPhi;
      fillData();
      _origin=origin;
      _rotation=rotation;
    }

    double torusRadius() const {return _rTorus;}
    double rIn()         const {return _rIn;   }
    double rOut()        const {return _rOut;  }
    double phiStart()    const {return _phiBegin; }
    double deltaPhi()    const {return _deltaPhi; }

    std::string getMaterial() const { return _materialName; }
    void setMaterial( std::string material ) { _materialName = material; }

    const std::array<double,5>& getParameters() const { return _data; }

  private:

    // All dimensions in mm.
    double _rTorus;
    double _rIn;
    double _rOut;

    double _phiBegin;
    double _deltaPhi;

    std::string _materialName;

    std::array<double,5> _data;

    void fillData() {
      _data[0] = _rIn;
      _data[1] = _rOut;
      _data[2] = _rTorus;
      _data[3] = _phiBegin;
      _data[4] = _deltaPhi;
    }

};

}
#endif /* BeamlineGeom_TorusSection_hh */