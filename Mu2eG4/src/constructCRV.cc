//
// Free function to create CRV aka Scintillator Shield in CosmicRayShield
//
// $Id: constructCRV.cc,v 1.20 2014/02/10 14:23:03 ehrlich Exp $
// $Author: ehrlich $
// $Date: 2014/02/10 14:23:03 $
//
// Original author KLG
//
// Notes:


// clhep includes
#include "CLHEP/Vector/ThreeVector.h"

// Mu2e includes.

#include "Mu2eG4/inc/constructCRV.hh"
#include "GeometryService/inc/GeomHandle.hh"
#include "GeometryService/inc/GeometryService.hh"
#include "G4Helper/inc/G4Helper.hh"
#include "CosmicRayShieldGeom/inc/CosmicRayShield.hh"
#include "CosmicRayShieldGeom/inc/CRSScintillatorShield.hh"
#include "Mu2eG4/inc/findMaterialOrThrow.hh"
#include "Mu2eG4/inc/SensitiveDetectorName.hh"
#include "Mu2eG4/inc/checkForOverlaps.hh"
#include "Mu2eG4/inc/nestBox.hh"

// G4 includes

#include "G4Material.hh"
#include "G4Color.hh"
#include "G4Box.hh"

#include "G4VSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"

#include "G4VisAttributes.hh"

#include "G4RotationMatrix.hh"

#include "G4SDManager.hh"

using namespace std;

namespace mu2e 
{
  void constructCRV( VolumeInfo const & parent, SimpleConfig const &  _config)
  {
    GeomHandle<CosmicRayShield> CosmicRayShieldGeomHandle;

    G4Helper    & _helper = *(art::ServiceHandle<G4Helper>());
    AntiLeakRegistry & reg = _helper.antiLeakRegistry();

    bool scintillatorShieldVisible = _config.getBool("crs.vetoVisible",true);
    bool scintillatorShieldSolid   = _config.getBool("crs.vetoSolid",false);

    int verbosityLevel = _config.getInt("crs.verbosityLevel",0);

    bool const forceAuxEdgeVisible = _config.getBool("g4.forceAuxEdgeVisible",false);
    bool const doSurfaceCheck      = _config.getBool("g4.doSurfaceCheck",false);

    std::string hallAirMaterialName = _config.getString("hall.insideMaterialName");

    CLHEP::Hep3Vector parentCenterInMu2e = parent.centerInMu2e();

    std::vector<CRSScintillatorShield> const &shields = CosmicRayShieldGeomHandle->getCRSScintillatorShields();
    std::vector<CRSScintillatorShield>::const_iterator ishield;
    for(ishield=shields.begin(); ishield!=shields.end(); ++ishield) 
    {
      CRSScintillatorShield const & shield = *ishield;
      std::string const & scintillatorBarName = shield.getName();

      if(verbosityLevel > 0) cout << __func__ << " constructing            : " << scintillatorBarName << endl;

      // all materials and dimensions are the same within a particular shield
      const std::shared_ptr<CRSScintillatorBarDetail> barDetail = shield.getCRSScintillatorBarDetail();

      G4Material* scintillatorBarMaterial = findMaterialOrThrow(barDetail->getMaterialName());
      std::vector<double> const &  scintillatorBarHalfLengths = barDetail->getHalfLengths();

      G4VSolid* scintillatorBarSolid = new G4Box(scintillatorBarName,
                                                 scintillatorBarHalfLengths[0],
                                                 scintillatorBarHalfLengths[1],
                                                 scintillatorBarHalfLengths[2]);

      G4LogicalVolume* scintillatorBarLogical = new G4LogicalVolume(scintillatorBarSolid,
                                                                    scintillatorBarMaterial,
                                                                    scintillatorBarName);

      // visibility attributes
      if (!scintillatorShieldVisible) 
      {
        scintillatorBarLogical->SetVisAttributes(G4VisAttributes::Invisible);
      }
      else 
      {
        G4Colour  orange  (.75, .55, .0);
        G4VisAttributes* visAtt = reg.add(G4VisAttributes(true, orange));
        visAtt->SetForceSolid(scintillatorShieldSolid);
        visAtt->SetForceAuxEdgeVisible(forceAuxEdgeVisible);
        scintillatorBarLogical->SetVisAttributes(visAtt);
      }

      // Make each scintillatorBar a sensitive detector.
      G4VSensitiveDetector *sd = 
                     G4SDManager::GetSDMpointer()->FindSensitiveDetector(SensitiveDetectorName::CRSScintillatorBar());
      if(sd) scintillatorBarLogical->SetSensitiveDetector(sd);

      if (verbosityLevel > 1) 
      {
        G4SDManager::GetSDMpointer()->SetVerboseLevel(verbosityLevel-1);
      }

      const std::vector<CRSScintillatorModule> &modules = shield.getCRSScintillatorModules();
      std::vector<CRSScintillatorModule>::const_iterator imodule;
      for(imodule=modules.begin(); imodule!=modules.end(); ++imodule) 
      {
        const std::vector<CRSScintillatorLayer> &layers = imodule->getLayers();
        std::vector<CRSScintillatorLayer>::const_iterator ilayer;
        for(ilayer=layers.begin(); ilayer!=layers.end(); ++ilayer) 
        {

          //construct a G4Box around all bars of a layer to speed up the surface checks
          const std::vector<double> &layerHalflengths=ilayer->getHalfLengths();
          const CLHEP::Hep3Vector &layerCenterInMu2e=ilayer->getPosition();
          CLHEP::Hep3Vector layerAirOffset = layerCenterInMu2e - parentCenterInMu2e;

          G4Material* hallAirMaterial = findMaterialOrThrow(hallAirMaterialName);
          VolumeInfo layerInfo = nestBox(ilayer->name("CRSScintillatorLayer_"),
                                         layerHalflengths,
                                         hallAirMaterial,
                                         NULL/*rotation*/,
                                         layerAirOffset,
                                         parent.logical,
                                         0/*copyNumber*/,
                                         false/*visible*/,
                                         G4Colour::Yellow(),
                                         false/*solid*/,
                                         forceAuxEdgeVisible,
                                         true/*placePV*/,
                                         doSurfaceCheck);

          const std::vector<const CRSScintillatorBar*> &bars = ilayer->getBars();
          std::vector<const CRSScintillatorBar*>::const_iterator ibar;
          for(ibar=bars.begin(); ibar!=bars.end(); ++ibar) 
          {
            const CRSScintillatorBar &bar = **ibar; 

            //bar.getPosition() returns the bar position in Mu2e coordinates
            CLHEP::Hep3Vector barLayerOffset = bar.getPosition() - layerCenterInMu2e; 

            if ( verbosityLevel > 3 ) 
            {
              cout << __func__ << " barPosition          : " <<  bar.getPosition() << endl;
              cout << __func__ << " barShieldOffset      : " <<  barLayerOffset << endl;
              cout << __func__ << " shieldAirOffset      : " <<  layerAirOffset << endl;
            }

            G4VPhysicalVolume* pv = new G4PVPlacement( NULL,
                                                       barLayerOffset,
                                                       scintillatorBarLogical,
                                                       bar.name("CRSScintillatorBar_"),
                                                       layerInfo.logical,
                                                       false,
                                                       bar.index().asInt(),
                                                       false);

            if ( doSurfaceCheck) 
            {
              checkForOverlaps( pv, _config, verbosityLevel>0);
            }
          } //ibar
        } //ilayer

        //absorber sheet
        const std::vector<CRSAbsorberLayer> &absorberLayers = imodule->getAbsorberLayers();
        std::vector<CRSAbsorberLayer>::const_iterator iabsorberlayer;
        for(iabsorberlayer=absorberLayers.begin(); iabsorberlayer!=absorberLayers.end(); ++iabsorberlayer) 
        {
          const std::vector<double> &absorberLayerHalflengths=iabsorberlayer->getHalfLengths();
          const CLHEP::Hep3Vector &absorberLayerCenterInMu2e=iabsorberlayer->getPosition();
          CLHEP::Hep3Vector absorberLayerAirOffset = absorberLayerCenterInMu2e - parentCenterInMu2e;

          const std::string &absorberMaterialName = shield.getAbsorberMaterialName();
          G4Material* absorberMaterial = findMaterialOrThrow(absorberMaterialName);

          std::string absorberName = iabsorberlayer->name("CRSAbsorber_");

          G4VSolid* absorberSolid = new G4Box(absorberName,
                                              absorberLayerHalflengths[0],
                                              absorberLayerHalflengths[1],
                                              absorberLayerHalflengths[2]);

          G4LogicalVolume* absorberLogical = new G4LogicalVolume(absorberSolid,
                                                                 absorberMaterial,
                                                                 absorberName);

          if(!scintillatorShieldVisible) 
          {
            absorberLogical->SetVisAttributes(G4VisAttributes::Invisible);
          }
          else 
          {
            G4Colour  darkorange  (.45, .25, .0);
            G4VisAttributes* visAtt = reg.add(G4VisAttributes(true, darkorange));
            visAtt->SetForceSolid(absorberSolid);
            visAtt->SetForceAuxEdgeVisible(forceAuxEdgeVisible);
            absorberLogical->SetVisAttributes(visAtt);
          }

          G4VPhysicalVolume* pv = new G4PVPlacement(NULL,
                                                    absorberLayerAirOffset,
                                                    absorberLogical,
                                                    absorberName,
                                                    parent.logical,
                                                    false,
                                                    0,
                                                    false);
          if(doSurfaceCheck) 
          {
            checkForOverlaps( pv, _config, verbosityLevel>0);
          }
        } //iabsorber
      } //imodule
    } //ishield
  } //construct CRV

} //namespace mu2e
