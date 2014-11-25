//
// Free function to create  Production Solenoid and Production Target.
//
// $Id: constructPS.cc,v 1.20 2014/09/19 19:15:04 knoepfel Exp $
// $Author: knoepfel $
// $Date: 2014/09/19 19:15:04 $
//
// Original author KLG based on Mu2eWorld constructPS
//
// Notes:
// Construct the PS. Parent volume is the air inside of the hall.

// C++ includes
#include <iostream>

// Mu2e includes.
#include "BeamlineGeom/inc/Beamline.hh"
#include "ProductionSolenoidGeom/inc/ProductionSolenoid.hh"
#include "GeomPrimitives/inc/Tube.hh"
#include "GeomPrimitives/inc/Polycone.hh"
#include "G4Helper/inc/VolumeInfo.hh"
#include "GeometryService/inc/G4GeometryOptions.hh"
#include "GeometryService/inc/GeomHandle.hh"
#include "ProductionTargetGeom/inc/ProductionTarget.hh"
#include "Mu2eG4/inc/findMaterialOrThrow.hh"
#include "Mu2eG4/inc/constructPS.hh"
#include "Mu2eG4/inc/constructPSShield.hh"
#include "Mu2eG4/inc/constructTargetPS.hh"
#include "Mu2eG4/inc/nestTubs.hh"
#include "Mu2eG4/inc/finishNesting.hh"
#include "Mu2eG4/inc/SensitiveDetectorName.hh"

#include "ProductionSolenoidGeom/inc/PSVacuum.hh"

// FIXME: only needed when the constructPSShield() call below is conditional
#include "ProductionSolenoidGeom/inc/PSShield.hh"

// G4 includes
#include "G4ThreeVector.hh"
#include "G4Material.hh"
#include "G4Color.hh"
#include "G4Polycone.hh"
#include "G4SDManager.hh"

#include "CLHEP/Units/SystemOfUnits.h"

using namespace std;

namespace mu2e {

  void constructPS(VolumeInfo const & parent, SimpleConfig const & _config) {
    
    ProductionSolenoid const & psgh = *(GeomHandle<ProductionSolenoid>());

    Tube const & psVacVesselInnerParams     = *psgh.getVacVesselInnerParamsPtr();
    Tube const & psVacVesselOuterParams     = *psgh.getVacVesselOuterParamsPtr();

    // Extract some information from the config file.

    int verbosityLevel                  = _config.getInt("PS.verbosityLevel");
                                                
    G4Material* psVacVesselMaterial = findMaterialOrThrow(psVacVesselInnerParams.materialName());

    verbosityLevel >0 && 
      cout << __func__ << " verbosityLevel                   : " << verbosityLevel  << endl;

    G4GeometryOptions* geomOptions = art::ServiceHandle<GeometryService>()->geomOptions();
    geomOptions->loadEntry( _config, "PS", "PS" );

    bool psVacuumSensitive = _config.getBool("PS.Vacuum.Sensitive", false);

    G4ThreeVector _hallOriginInMu2e = parent.centerInMu2e();

    // we need to replace this in future getting/moving the info from/to geom service

    // VacVessel

    // VacVessel is a "G4Tubs"; or a combination of those

    // Build the barrel of the cryostat, the length is the "outer length"

    VolumeInfo psVacVesselInnerInfo = 
      nestTubs("PSVacVesselInner",
               psVacVesselInnerParams.getTubsParams(),
               psVacVesselMaterial,
               0,
               psVacVesselInnerParams.originInMu2e()-_hallOriginInMu2e,
               parent,
               0,
               G4Colour::Green(),
	       "PS"
               );

    VolumeInfo psVacVesselOuterInfo = 
      nestTubs("PSVacVesselOuter",
               psVacVesselOuterParams.getTubsParams(),
               psVacVesselMaterial,
               0,
               psVacVesselOuterParams.originInMu2e()-_hallOriginInMu2e,
               parent,
               0,
               G4Colour::Green(),
	       "PS"
               );

    // two endplates

    Tube const & psVacVesselEndPlateDParams = *psgh.getVacVesselEndPlateDParamsPtr();
    Tube const & psVacVesselEndPlateUParams = *psgh.getVacVesselEndPlateUParamsPtr();

    VolumeInfo psVacVesselEndPlateDInfo = 
      nestTubs("PSVacVesselEndPlateD",
               psVacVesselEndPlateDParams.getTubsParams(),
               psVacVesselMaterial,
               0,
               psVacVesselEndPlateDParams.originInMu2e()-_hallOriginInMu2e,
               parent,
               0,
               G4Colour::Yellow(),
	       "PS"
               );

    VolumeInfo psVacVesselEndPlateUInfo 
      = nestTubs("PSVacVesselEndPlateU",
                 psVacVesselEndPlateUParams.getTubsParams(),
                 psVacVesselMaterial,
                 0,
                 psVacVesselEndPlateUParams.originInMu2e()-_hallOriginInMu2e,
                 parent,
                 0,
                 G4Colour::Red(),
		 "PS"
                 );

    Polycone const & psCoilShellParams = *psgh.getCoilShellParamsPtr();

    //Coil "Outer Shell"

    //we will place the shell inside the parent, which is the hall
    string const psCoilShellName = "PSCoilShell";
  
    VolumeInfo psCoilShellInfo(psCoilShellName,
                               psCoilShellParams.originInMu2e()-parent.centerInMu2e(),
                               parent.centerInWorld);

    psCoilShellInfo.solid  =  new G4Polycone( psCoilShellName,
                                              psCoilShellParams.phi0(),
                                              psCoilShellParams.phiTotal(),
                                              psCoilShellParams.numZPlanes(),
                                              &psCoilShellParams.zPlanes()[0],
                                              &psCoilShellParams.rInner()[0],
                                              &psCoilShellParams.rOuter()[0]);
    
    G4Material* psCoilShellMaterial = findMaterialOrThrow(psCoilShellParams.materialName());

    finishNesting(psCoilShellInfo,
                  psCoilShellMaterial,
                  0,
                  psCoilShellParams.originInMu2e()-parent.centerInMu2e(),
                  parent.logical,
                  0,
                  G4Colour::White(),
		  "PS"
                  );

    // the superconducting Coils

    //the coils are "G4Tubs" placed inside the Coil Shell

    Tube const & psCoil1Params        = *psgh.getCoil1ParamsPtr();

    // all coil materials are the same...
    G4Material* psCoilMaterial        = findMaterialOrThrow(psCoil1Params.materialName());

    VolumeInfo psCoil1Info = nestTubs("PSCoil1",
                                      psCoil1Params.getTubsParams(),
                                      psCoilMaterial,
                                      0,
                                      psCoil1Params.originInMu2e()- psCoilShellParams.originInMu2e(),
                                      psCoilShellInfo,
                                      0,
                                      G4Colour::Red(),
				      "PS"
                                      );


    Tube const & psCoil2Params        = *psgh.getCoil2ParamsPtr();

    VolumeInfo psCoil2Info = nestTubs("PSCoil2",
                                      psCoil2Params.getTubsParams(),
                                      psCoilMaterial,
                                      0,
                                      psCoil2Params.originInMu2e()- psCoilShellParams.originInMu2e(),
                                      psCoilShellInfo,
                                      0,
                                      G4Colour::Red(),
				      "PS"
                                      );


    Tube const & psCoil3Params        = *psgh.getCoil3ParamsPtr();

    VolumeInfo psCoil3Info = nestTubs("PSCoil3",
                                      psCoil3Params.getTubsParams(),
                                      psCoilMaterial,
                                      0,
                                      psCoil3Params.originInMu2e()- psCoilShellParams.originInMu2e(),
                                      psCoilShellInfo,
                                      0,
                                      G4Colour::Red(),
				      "PS"
                                      );


    if ( verbosityLevel > 0) {

      cout << __func__ << " parent.centerInMu2e()                     : " 
           << parent.centerInMu2e() << endl;

      cout << __func__ << " psVacVesselInnerParams.originInMu2e()     : " 
           << psVacVesselInnerParams.originInMu2e() << endl;
      cout << __func__ << " psVacVesselOuterParams.originInMu2e()     : " 
           << psVacVesselOuterParams.originInMu2e() << endl;

      cout << __func__ << " psVacVesselEndPlateDParams.originInMu2e() : " 
           << psVacVesselEndPlateDParams.originInMu2e() << endl;
      cout << __func__ << " psVacVesselEndPlateUParams.originInMu2e() : " 
           << psVacVesselEndPlateUParams.originInMu2e() << endl;

      cout << __func__ << " psCoilShellParams.originInMu2e()          : "
           << psCoilShellParams.originInMu2e() << endl;

      cout << __func__ << " psCoilShellInfo.centerInMu2e()            : "
           << psCoilShellInfo.centerInMu2e() <<  endl;

      cout << __func__ << " psCoil1Params.originInMu2e()              : " 
           << psCoil1Params.originInMu2e() << endl;
      cout << __func__ << " psCoil1 local Offset                      : " 
           << psCoil1Params.originInMu2e()- psCoilShellInfo.centerInMu2e() << endl;

      cout << __func__ << " psCoil2Params.originInMu2e()              : " 
           << psCoil2Params.originInMu2e() << endl;
      cout << __func__ << " psCoil2 local Offset                      : " 
           << psCoil2Params.originInMu2e()- psCoilShellInfo.centerInMu2e() << endl;

      cout << __func__ << " psCoil3Params.originInMu2e()              : " 
           << psCoil3Params.originInMu2e() << endl;
      cout << __func__ << " psCoil3 local Offset                      : " 
           << psCoil3Params.originInMu2e()- psCoilShellInfo.centerInMu2e() << endl;

    }

    // Build the main PS vacuum body.

    // we may need to adjust it to make sure it conforms to the newer
    // drawings where the collimators enter the PS area


    Tube const & psVacuumParams  = GeomHandle<PSVacuum>()->vacuum();

    VolumeInfo psVacuumInfo   = nestTubs( "PSVacuum",
                                          psVacuumParams.getTubsParams(),
                                          findMaterialOrThrow(psVacuumParams.materialName()),
                                          0,
                                          psVacuumParams.originInMu2e()-_hallOriginInMu2e,
                                          parent,
                                          0,
                                          G4Colour::Blue(),
					  "PS"
                                          );

    if(psVacuumSensitive) {
      G4VSensitiveDetector* psVacuumSD = G4SDManager::GetSDMpointer()->
        FindSensitiveDetector(SensitiveDetectorName::PSVacuum());
      if(psVacuumSD) psVacuumInfo.logical->SetSensitiveDetector(psVacuumSD);
    }

//    // Build the production target.
//    GeomHandle<ProductionTarget> tgt;
//    TubsParams prodTargetParams( 0., tgt->rOut(), tgt->halfLength());
//
//    G4Material* prodTargetMaterial = findMaterialOrThrow(_config.getString("targetPS_materialName"));
//
//    geomOptions->loadEntry( config, "ProductionTarget", "targetPS" );
//
//    VolumeInfo prodTargetInfo   = nestTubs( "ProductionTarget",
//                                            prodTargetParams,
//                                            prodTargetMaterial,
//                                            &tgt->productionTargetRotation(),
//                                            tgt->position() - psVacuumInfo.centerInMu2e(),
//                                            psVacuumInfo,
//                                            0,
//                                            G4Colour::Magenta()
//                                            );
    constructTargetPS(psVacuumInfo, _config);

    // FIXME: make unconditional
    if(art::ServiceHandle<GeometryService>()->hasElement<PSShield>()) {
      constructPSShield(psVacuumInfo, _config);
    }

  } // end Mu2eWorld::constructPS
}