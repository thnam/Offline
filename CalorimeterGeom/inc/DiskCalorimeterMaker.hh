#ifndef CalorimeterGeom_DiskCalorimeterMaker_hh
#define CalorimeterGeom_DiskCalorimeterMaker_hh

// original authors B. Echenard

//
// C++ includes
#include <iomanip>
#include <memory>
#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include "CLHEP/Vector/ThreeVector.h"

//
//Mu2e includes
#include "CalorimeterGeom/inc/DiskCalorimeter.hh"
#include "CalorimeterGeom/inc/Disk.hh"
#include "ConfigTools/inc/SimpleConfig.hh"


namespace mu2e{


    class SimpleConfig;
    class DiskCalorimeter;

    class DiskCalorimeterMaker{

    public:
 
       DiskCalorimeterMaker(SimpleConfig const& config, double solenoidOffset);
      ~DiskCalorimeterMaker();

      // Accessor and auto_ptr to calorimeter needed by GeometryService.
      std::auto_ptr<DiskCalorimeter> _calo;
      std::auto_ptr<DiskCalorimeter> getCalorimeterPtr() { return _calo; }

    private:

      void MakeDisks(void);
      void CheckIt(void);

      int verbosityLevel;

    };

} //namespace mu2e

#endif /* CalorimeterGeom_DiskCalorimeterMaker_hh */
