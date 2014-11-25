// Andrei Gaponenko, 2012

#include "Mu2eG4/inc/constructPSEnclosure.hh"

#include "CLHEP/Vector/ThreeVector.h"

#include "ProductionSolenoidGeom/inc/PSEnclosure.hh"
#include "ProductionSolenoidGeom/inc/PSVacuum.hh"
#include "GeometryService/inc/GeomHandle.hh"
#include "GeomPrimitives/inc/Tube.hh"
#include "GeomPrimitives/inc/TubsParams.hh"
#include "Mu2eG4/inc/findMaterialOrThrow.hh"
#include "Mu2eG4/inc/nestTubs.hh"
#include "G4Helper/inc/VolumeInfo.hh"
#include "ConfigTools/inc/SimpleConfig.hh"

namespace mu2e {

  //================================================================

  void constructPSEnclosure(const VolumeInfo& parent, const SimpleConfig& config) {

    GeomHandle<PSEnclosure> pse;
    GeomHandle<PSVacuum> psv;

    const bool forceAuxEdgeVisible = config.getBool("g4.forceAuxEdgeVisible",false);
    const bool doSurfaceCheck      = config.getBool("g4.doSurfaceCheck",false);
    const bool placePV             = true;

    //----------------------------------------------------------------
    nestTubs("PSEnclosureShell",
             pse->shell().getTubsParams(),
             findMaterialOrThrow(pse->shell().materialName()),
             0,
             pse->shell().originInMu2e() - parent.centerInMu2e(),
             parent,
             0,
             config.getBool("PSEnclosure.visible"),
             G4Colour::Blue(),
             config.getBool("PSEnclosure.solid"),
             forceAuxEdgeVisible,
             placePV,
             doSurfaceCheck
             );

    const VolumeInfo endPlate = nestTubs("PSEnclosureEndPlate",
                                         pse->endPlate().getTubsParams(),
                                         findMaterialOrThrow(pse->endPlate().materialName()),
                                         0,
                                         pse->endPlate().originInMu2e() - parent.centerInMu2e(),
                                         parent,
                                         0,
                                         config.getBool("PSEnclosure.visible"),
                                         G4Colour::Blue(),
                                         config.getBool("PSEnclosure.solid"),
                                         forceAuxEdgeVisible,
                                         placePV,
                                         doSurfaceCheck
                                         );

    //----------------------------------------------------------------
    // Install the windows

    for(unsigned i=0; i<pse->nWindows(); ++i) {

      // Hole in the endPlate for the window
      const CLHEP::Hep3Vector vacCenter =
        pse->windows()[i].originInMu2e() +
        CLHEP::Hep3Vector(0,0, pse->endPlate().halfLength() + pse->windows()[i].halfLength())
        ;

      const TubsParams vacTubs(pse->windows()[i].innerRadius(),
                               pse->windows()[i].outerRadius(),
                               pse->endPlate().halfLength()
                               );

      std::ostringstream vname;
      vname<<"PSEnclosureWindowVac"<<i;
      nestTubs(vname.str(),
               vacTubs,
               findMaterialOrThrow(psv->vacuum().materialName()),
               0,
               vacCenter - endPlate.centerInMu2e(),
               endPlate,
               0,
               config.getBool("PSEnclosure.vacuum.visible"),
               G4Colour::Black(),
               config.getBool("PSEnclosure.vacuum.solid"),
               forceAuxEdgeVisible,
               placePV,
               doSurfaceCheck
               );

      // The window itself
      std::ostringstream wname;
      wname<<"PSEnclosureWindow"<<i;
      nestTubs(wname.str(),
               pse->windows()[i].getTubsParams(),
               findMaterialOrThrow(pse->windows()[i].materialName()),
               0,
               pse->windows()[i].originInMu2e() - parent.centerInMu2e(),
               parent,
               0,
               config.getBool("PSEnclosure.visible"),
               G4Colour::Grey(),
               config.getBool("PSEnclosure.solid"),
               forceAuxEdgeVisible,
               placePV,
               doSurfaceCheck
               );

    }

    //----------------------------------------------------------------
  }

}