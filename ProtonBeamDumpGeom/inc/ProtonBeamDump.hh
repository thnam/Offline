// Geometry of the proton beam dump.
//
// Andrei Gaponenko, 2011

#ifndef PROTONBEAMDUMP_HH
#define PROTONBEAMDUMP_HH

#include <vector>

#include "CLHEP/Vector/ThreeVector.h"
#include "CLHEP/Vector/Rotation.h"

#include "art/Persistency/Common/Wrapper.h"

#include "Mu2eInterfaces/inc/Detector.hh"

namespace mu2e {

  class ProtonBeamDumpMaker;

  class ProtonBeamDump : virtual public Detector {
  public:

    //----------------------------------------------------------------
    // Primary inputs: sizes

    const std::vector<double>& coreHalfSize() const { return _coreHalfSize; }

    const std::vector<double>& neutronCaveHalfSize() const { return _neutronCaveHalfSize; }

    const std::vector<double>& mouthHalfSize() const { return _mouthHalfSize; }

    double minCoreShieldingThickness() const { return _minCoreShieldingThickness; }

    //----
    // Primary inputs: placement
    const CLHEP::Hep3Vector& coreCenterInMu2e() const { return _coreCenterInMu2e; }
    // absolute w.r.t to the Mu2e
    double coreRotY() const { return _coreRotY; }
    const CLHEP::HepRotation& coreRotationInMu2e() const { return _coreRotationInMu2e; }

    //---
    // Derived stuff

    // Front core shielding: contains the dump core and collimator1.
    double coreCenterDistanceToShieldingFace() const { return _coreCenterDistanceToShieldingFace; }

    const std::vector<double>& frontShieldingHalfSize() const { return _frontShieldingHalfSize; }
    const CLHEP::Hep3Vector& frontShieldingCenterInMu2e() const { return _frontShieldingCenterInMu2e; }

    const CLHEP::Hep3Vector& mouthCenterInMu2e() const { return _mouthCenterInMu2e; }
    const CLHEP::Hep3Vector& neutronCaveCenterInMu2e() const { return _neutronCaveCenterInMu2e; }

    // May need to add extra shielding to "frontShielding" to satisfy the min. shielding requirement
    const std::vector<double>& backShieldingHalfSize() const { return _backShieldingHalfSize; }
    const CLHEP::Hep3Vector& backShieldingCenterInMu2e() const { return _backShieldingCenterInMu2e; }

    double shieldingFaceXmin() const { return _shieldingFaceXmin; }
    double shieldingFaceXmax() const { return _shieldingFaceXmax; }

    double shieldingFaceZatXmin() const { return _shieldingFaceZatXmin; }
    double shieldingFaceZatXmax() const { return _shieldingFaceZatXmax; }

    //----------------------------------------------------------------
    // Transform to the "beam dump" coordinate system, which is centered
    // at the core center, and is rotated around the Y axis w.r.t the mu2e system

    CLHEP::Hep3Vector mu2eToBeamDump_position(const CLHEP::Hep3Vector& mu2epos) const;
    CLHEP::Hep3Vector mu2eToBeamDump_momentum(const CLHEP::Hep3Vector& mu2emom) const;

    CLHEP::Hep3Vector beamDumpToMu2e_position(const CLHEP::Hep3Vector& dumppos) const;
    CLHEP::Hep3Vector beamDumpToMu2e_momentum(const CLHEP::Hep3Vector& dumpmom) const;

    //----------------------------------------------------------------
  private:
    friend class ProtonBeamDumpMaker;
    // Private ctr: the class should be only obtained via ProtonBeamDumpFNAL::ProtonBeamDumpMaker.
    ProtonBeamDump();
    // Or read back from persistent storage
    template<class T> friend class art::Wrapper;

    CLHEP::Hep3Vector _coreCenterInMu2e;
    double _coreRotY;
    CLHEP::HepRotation _coreRotationInMu2e;

    std::vector<double> _coreHalfSize;
    std::vector<double> _neutronCaveHalfSize;
    std::vector<double> _mouthHalfSize;
    double _minCoreShieldingThickness;

    // computed stuff
    double _coreCenterDistanceToShieldingFace;
    std::vector<double> _frontShieldingHalfSize;
    CLHEP::Hep3Vector _frontShieldingCenterInMu2e;

    std::vector<double> _backShieldingHalfSize;
    CLHEP::Hep3Vector _backShieldingCenterInMu2e;

    CLHEP::Hep3Vector _mouthCenterInMu2e;
    CLHEP::Hep3Vector _neutronCaveCenterInMu2e;

    double _shieldingFaceXmin;
    double _shieldingFaceXmax;
    double _shieldingFaceZatXmin;
    double _shieldingFaceZatXmax;
  };
}

#endif/*PROTONBEAMDUMP_HH*/