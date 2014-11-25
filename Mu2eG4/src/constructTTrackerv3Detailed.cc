//
// Free function to construct the TTracker version that:
//  - Uses version v3 of the G4 model
//  - Uses the support model SupportModel::detailedv0.
//  - See comments at the top of constructTTrackerv3.cc for additional details
//    of the meaning of version numbers.
//
// $Id: constructTTrackerv3Detailed.cc,v 1.7 2014/01/06 20:46:39 kutschke Exp $
// $Author: kutschke $
// $Date: 2014/01/06 20:46:39 $
//
// Contact person Rob Kutschke,
//   - Based on constructTTrackerv3 by KLG
//
// Notes
//     This version makes logical mother volumes per device and per
//     sector and places sectors in device and straws in sector
//     It has only one sector/device logical volume placed several times
//     This versoin has a negligeable construction time and a much smaller memory footprint
//

// C++ includes
#include <iomanip>
#include <iostream>
#include <string>

// Framework includes
#include "cetlib/exception.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

// Mu2e includes
#include "G4Helper/inc/G4Helper.hh"
#include "GeometryService/inc/GeomHandle.hh"
#include "Mu2eG4/inc/SensitiveDetectorName.hh"
#include "Mu2eG4/inc/StrawSD.hh"
#include "Mu2eG4/inc/constructTTracker.hh"
#include "Mu2eG4/inc/findMaterialOrThrow.hh"
#include "Mu2eG4/inc/finishNesting.hh"
#include "Mu2eG4/inc/nestBox.hh"
#include "Mu2eG4/inc/nestTubs.hh"
#include "TTrackerGeom/inc/TTracker.hh"
#include "Mu2eG4/inc/checkForOverlaps.hh"

// G4 includes
#include "G4Box.hh"
#include "G4Colour.hh"
#include "G4Material.hh"
#include "G4Polycone.hh"
#include "G4PVPlacement.hh"
#include "G4SDManager.hh"
#include "G4ThreeVector.hh"
#include "G4Tubs.hh"

#include "cetlib/pow.h"

using namespace std;

namespace mu2e{

namespace {

  // A helper structure to for nesting a many tubs.
  struct VolHelper{

    VolHelper( PlacedTubs const& apart, G4Colour const& acolour, std::string const& amotherName, std::deque<VolHelper> const& vols, bool avisible=false ):
      part(&apart), colour(acolour), motherName(amotherName), info(), mother(0), visible(avisible) {
      for ( std::deque<VolHelper>::const_iterator i=vols.begin(), e=vols.end();
            i != e; ++i ){
        if ( i->part->name() == motherName ){
          mother = &(i->info);
        }
      }
      if ( mother == 0 ){
        throw cet::exception("GEOM") << "For volume: "
                                     << part->name()
                                     << "  Could not find mother: "
                                     << motherName
                                     << "\n";
      }
    }

    VolHelper( PlacedTubs const& apart, G4Colour const& acolour, std::string const& amotherName, mu2e::VolumeInfo const& amother, bool avisible=false ):
      part(&apart), colour(acolour), motherName(amotherName), info(), mother(&amother), visible(avisible) {
    }

    PlacedTubs const* part;
    G4Colour          colour;
    std::string       motherName;
    mu2e::VolumeInfo        info;
    mu2e::VolumeInfo const* mother;
    bool                    visible;
  };

  void addSupports(){
  }

  void addSectors( int idev,
                   VolumeInfo const& devInfo,
                   VolumeInfo& sec0Info,
                   double sectorCenterPhi,
                   AntiLeakRegistry& reg,
                   SimpleConfig const& config ){

    TTracker const& ttracker = *(GeomHandle<TTracker>());
    Device const& dev        = ttracker.getDevice(idev);

    int secDraw         = config.getInt ("ttracker.secDraw",-1);
    bool doSurfaceCheck = config.getBool("g4.doSurfaceCheck",false);
    int verbosityLevel  = config.getInt ("ttracker.verbosityLevel",0);


    // Needed to get the center position in local z.
    PlacedTubs const& chanUp(ttracker.getSupportStructure().innerChannelUpstream());

    // Place the sector envelope into the device envelope, one placement per sector.
    for ( int isec=0; isec<dev.nSectors(); ++isec){

      // For debugging, only place the one requested sector.
      if ( secDraw > -1  && isec != secDraw ) continue;
      //if ( secDraw > -1  && isec%2 == 0 ) continue;

      // Choose a representative straw from this this (device,sector).
      Straw const& straw = ttracker.getStraw( StrawId(idev,isec,0,0) );

      // Azimuth of the midpoint of the wire.
      CLHEP::Hep3Vector const& mid = straw.getMidPoint();
      double phimid = mid.phi();
      if ( phimid < 0 ) phimid += 2.*M_PI;

      // Is this sector on the front or back face of the plane?
      double sign   = ((mid.z() - dev.origin().z())>0. ? 1.0 : -1.0 );
      CLHEP::Hep3Vector sectorPosition(0.,0., sign*std::abs(chanUp.position().z()) );

      // The rotation that will make the straw mid point have the correct azimuth.
      // Sectors on the downstream side are flipped front/back by rotation about Y.
      // so the sense of the z rotation changes sign.
      double phi0 = sectorCenterPhi-phimid;
      if ( sign > 0 ){
        phi0 = sectorCenterPhi + M_PI +phimid;
      }

      CLHEP::HepRotationZ rotZ(phi0);
      CLHEP::HepRotationY rotY(M_PI);
      G4RotationMatrix* rotation  = (sign < 0 )?
        reg.add(G4RotationMatrix(rotZ)) :
        reg.add(G4RotationMatrix(rotZ*rotY));

      bool many(false);
      sec0Info.physical =  new G4PVPlacement(rotation,
                                             sectorPosition,
                                             sec0Info.logical,
                                             sec0Info.name,
                                             devInfo.logical,
                                             many,
                                             isec,
                                             false);
      if ( doSurfaceCheck) {
        checkForOverlaps(sec0Info.physical, config, verbosityLevel>0);
      }

    }

  } // end addSectors

} // end anonymous namespace


  VolumeInfo constructTTrackerv3Detailed( VolumeInfo const& ds3Vac,
                                          SimpleConfig const& config ){

    cout << "Mark Detailed. " << endl;

    G4Helper& _helper      = *(art::ServiceHandle<G4Helper>());
    AntiLeakRegistry & reg = _helper.antiLeakRegistry();

    int verbosityLevel = config.getInt("ttracker.verbosityLevel",0);

    // Control of graphics for debugging the geometry.
    // Only instantiate sectors to be drawn.
    int devDraw = config.getInt("ttracker.devDraw",-1);
    //int secDraw = config.getInt("ttracker.secDraw",-1);
    bool const doSurfaceCheck = config.getBool("g4.doSurfaceCheck",false);
    bool const forceAuxEdgeVisible = config.getBool("g4.forceAuxEdgeVisible",false);

    G4ThreeVector const zeroVector(0.0,0.0,0.0);
    bool place(true);
    bool doNotPlace(false);
    G4RotationMatrix* noRotation(0);

    // Master geometry for the TTracker.
    TTracker const & ttracker = *(GeomHandle<TTracker>());

    // Parameters of the new style mother volume ( replaces the envelope volume ).
    PlacedTubs const& mother = ttracker.mother();

    // Make the envelope volume that holds the tracker devices - not the end rings and staves.
    //TubsParams envelopeParams = ttracker.getInnerTrackerEnvelopeParams();

    static int const newPrecision = 8;
    static int const newWidth = 14;

    if (verbosityLevel > 0) {
      int oldPrecision = cout.precision(newPrecision);
      int oldWidth = cout.width(newWidth);
      std::ios::fmtflags oldFlags = cout.flags();
      cout.setf(std::ios::fixed,std::ios::floatfield);
      cout << "Debugging tracker env envelopeParams ir,or,zhl,phi0,phimax:            " <<
	"   " <<
	mother.innerRadius() << ", " <<
	mother.outerRadius() << ", " <<
	mother.zHalfLength() << ", " <<
	mother.phi0()        << ", " <<
	mother.phiMax()      << ", " <<
	endl;
      cout.setf(oldFlags);
      cout.precision(oldPrecision);
      cout.width(oldWidth);
    }

    //G4ThreeVector trackerOffset( 0., 0., ttracker.z0() );
    // Offset of the center of the tracker within its mother volume.

    // Offset of the center of the tracker within its mother volume.
    CLHEP::Hep3Vector motherOffset(0., 0., ttracker.z0()-mother.position().z() );
    cout << "Centers: " << mother.position() << " " << ds3Vac.centerInWorld << " " << ttracker.z0() << endl;
    cout << "         " << motherOffset << endl;

    // All mother/envelope volumes are made of this material.
    G4Material* envelopeMaterial = findMaterialOrThrow(ttracker.envelopeMaterial());

    VolumeInfo motherInfo = nestTubs( "TrackerMother",
                                      mother.tubsParams(),
                                      envelopeMaterial,
                                      noRotation,
                                      mother.position() - ds3Vac.centerInWorld,
                                      ds3Vac,
                                      0,
                                      config.getBool("ttracker.envelopeVisible",false),
                                      G4Colour::Blue(),
                                      config.getBool("ttracker.envelopeSolid",true),
                                      forceAuxEdgeVisible,
                                      place,
                                      doSurfaceCheck
                                      );

    if ( verbosityLevel > 0) {
      double zhl         = static_cast<G4Tubs*>(motherInfo.solid)->GetZHalfLength();
      double motherOffsetInMu2eZ = motherInfo.centerInMu2e()[CLHEP::Hep3Vector::Z];
      int oldPrecision = cout.precision(3);
      std::ios::fmtflags oldFlags = cout.flags();
      cout.setf(std::ios::fixed,std::ios::floatfield);
      cout << __func__ << " motherOffsetZ           in Mu2e    : " <<
        motherOffsetInMu2eZ << endl;
      cout << __func__ << " mother         Z extent in Mu2e    : " <<
        motherOffsetInMu2eZ - zhl << ", " << motherOffsetInMu2eZ + zhl << endl;
      cout.setf(oldFlags);
      cout.precision(oldPrecision);
    }

    // Now place the endRings and Staves.
    // FixME: add the cut-outs for services within the staves.
    if ( ttracker.getSupportModel() == SupportModel::detailedv0 ) {

      SupportStructure const& sup = ttracker.getSupportStructure();

      for ( auto const& ring : sup.stiffRings() ){

        cout << "Ring Position: "
             << ring.position() << " "
             << motherInfo.centerInWorld << " "
             << ring.position()-motherInfo.centerInWorld
             << endl;

        VolumeInfo info = nestTubs( ring.name(),
                                    ring.tubsParams(),
                                    findMaterialOrThrow(ring.materialName()),
                                    noRotation,
                                    ring.position()-motherInfo.centerInWorld,
                                    motherInfo,
                                    0,
                                    config.getBool("ttracker.envelopeVisible",false),
                                    G4Colour::Red(),
                                    config.getBool("ttracker.envelopeSolid",true),
                                    forceAuxEdgeVisible,
                                    place,
                                    doSurfaceCheck
                                    );

      }

      for ( auto const& stave : sup.staveBody() ){

        cout << "Stave Position: "
             << stave.position() << " "
             << motherInfo.centerInWorld << " "
             << stave.position()-motherInfo.centerInWorld
             << endl;

        nestTubs( stave.name(),
                  stave.tubsParams(),
                  findMaterialOrThrow(stave.materialName()),
                  &stave.rotation(),
                  stave.position()-motherInfo.centerInWorld,
                  motherInfo,
                  0,
                  config.getBool("ttracker.envelopeVisible",false),
                  G4Colour::Yellow(),
                  config.getBool("ttracker.envelopeSolid",true),
                  forceAuxEdgeVisible,
                  place,
                  doSurfaceCheck
                  );


      }

    }

    TubsParams deviceEnvelopeParams = ttracker.getDeviceEnvelopeParams();

    bool deviceEnvelopeVisible = config.getBool("ttracker.deviceEnvelopeVisible",false);
    bool deviceEnvelopeSolid   = config.getBool("ttracker.deviceEnvelopeSolid",true);
    bool supportVisible        = config.getBool("ttracker.supportVisible",false);
    bool supportSolid          = config.getBool("ttracker.supportSolid",true);
    bool sectorEnvelopeVisible = config.getBool("ttracker.sectorEnvelopeVisible",false);
    bool sectorEnvelopeSolid   = config.getBool("ttracker.sectorEnvelopeSolid",true);
    bool strawVisible          = config.getBool("ttracker.strawVisible",false);
    bool strawSolid            = config.getBool("ttracker.strawSolid",true);
    //bool strawLayeringVisible  = config.getBool("ttracker.strawLayeringVisible",false);
    // bool strawLayeringSolid    = config.getBool("ttracker.strawLayeringSolid",false);
    //bool drawAxes              = config.getBool("ttracker.drawAxes",false);

    //bool ttrackerActiveWr_Wl_SD = config.getBool("ttracker.ActiveWr_Wl_SD",false);

    // The devices differ only in the rotations of their sectors (panels).
    // Construct one device but do not place it until its insides have been populated.

    string trackerEnvelopeName("TTrackerDeviceEnvelope");
    VolumeInfo devInfo = nestTubs( trackerEnvelopeName,
                                   deviceEnvelopeParams,
                                   envelopeMaterial,
                                   noRotation,
                                   zeroVector,
                                   0,
                                   0,
                                   deviceEnvelopeVisible,
                                   G4Colour::Magenta(),
                                   deviceEnvelopeSolid,
                                   forceAuxEdgeVisible,
                                   doNotPlace,
                                   doSurfaceCheck
                                   );
    if ( verbosityLevel > 0 ){
      cout << "Device Envelope parameters: " << deviceEnvelopeParams << endl;
    }

    // Temporary while debugging the new mother volume.
    if ( config.getBool("rkk.disableTT",false ) ){
      cout << "Will not create TTracker because of a user request " << endl;
      return motherInfo;
    }


    // Many parts of the support structure are G4Tubs objects.  Place all of them,
    SupportStructure const& sup = ttracker.getSupportStructure();
    G4Colour  lightBlue (0.0, 0.0, 0.75);
    std::deque<VolHelper> vols;
    vols.push_back( VolHelper(sup.centerPlate(),         lightBlue,           trackerEnvelopeName,        devInfo, supportVisible ));
    vols.push_back( VolHelper(sup.outerRingUpstream(),   G4Colour::Green(),   trackerEnvelopeName,        devInfo, supportVisible ));
    vols.push_back( VolHelper(sup.outerRingDownstream(), G4Colour::Green(),   trackerEnvelopeName,        devInfo, supportVisible ));
    vols.push_back( VolHelper(sup.coverUpstream(),       G4Colour::Cyan(),    trackerEnvelopeName,        devInfo, supportVisible ));
    vols.push_back( VolHelper(sup.coverDownstream(),     G4Colour::Cyan(),    trackerEnvelopeName,        devInfo, supportVisible ));
    vols.push_back( VolHelper(sup.gasUpstream(),         G4Colour::Magenta(), trackerEnvelopeName,        devInfo, supportVisible ));
    vols.push_back( VolHelper(sup.gasDownstream(),       G4Colour::Magenta(), trackerEnvelopeName,        devInfo, supportVisible ));
    vols.push_back( VolHelper(sup.g10Upstream(),         G4Colour::Blue(),    sup.gasUpstream().name(),   vols, supportVisible ));
    vols.push_back( VolHelper(sup.g10Downstream(),       G4Colour::Blue(),    sup.gasDownstream().name(), vols, supportVisible ));
    vols.push_back( VolHelper(sup.cuUpstream(),          G4Colour::Yellow(),  sup.gasUpstream().name(),   vols, supportVisible ));
    vols.push_back( VolHelper(sup.cuDownstream(),        G4Colour::Yellow(),  sup.gasDownstream().name(), vols, supportVisible ));

    for ( std::deque<VolHelper>::iterator i=vols.begin(), e=vols.end();
          i != e; ++i ){
      PlacedTubs const& part = *i->part;

      bool placePhysicalVolume = true;
      i->info = nestTubs( part.name(),
                          part.tubsParams(),
                          findMaterialOrThrow(part.materialName()),
                          noRotation,
                          part.position(),
                          i->mother->logical,
                          0,
                          i->visible,
                          i->colour,
                          supportSolid,
                          forceAuxEdgeVisible,
                          placePhysicalVolume,
                          doSurfaceCheck
                          );

    }

    // Pick one of the tubs that represents mocked-up electronics and make it a senstive detector.
    G4VSensitiveDetector *sd = G4SDManager::GetSDMpointer()->
      FindSensitiveDetector(SensitiveDetectorName::TTrackerDeviceSupport());

    for ( std::deque<VolHelper>::iterator i=vols.begin(), e=vols.end();
          i != e; ++i ){
      PlacedTubs const& part = *i->part;
      if ( part.name().find("TTrackerSupportElecCu") != string::npos ){
        if(sd) i->info.logical->SetSensitiveDetector(sd);
      }
    }

    // The last part of the support structure is the inner ring; Construct it as a polycone
    // that includes the two channels to receive the straws.
    // FixME: use the other c'tor of Polycone that takes points in the rz plane.
    //        This will let us get rid of eps,below.
    {

      PlacedTubs const& ring     = sup.innerRing();
      PlacedTubs const& chanUp   = sup.innerChannelUpstream();
      PlacedTubs const& chanDown = sup.innerChannelDownstream();

      // Parameters of the polycone.
      const int n(10);
      double z[n];
      double rIn[n];
      double rOut[n];

      // FixME: see above about alternate c'tor.
      // I am not sure if the zplanes of a polycone are permitted to have zero spacing.
      // So include a minimum z space at the sharp edges.
      double eps=0.1;

      z[0]    = -ring.tubsParams().zHalfLength();
      z[1]    = chanUp.position().z()   - chanUp.tubsParams().zHalfLength() - eps;
      z[2]    = chanUp.position().z()   - chanUp.tubsParams().zHalfLength();
      z[3]    = chanUp.position().z()   + chanUp.tubsParams().zHalfLength();
      z[4]    = chanUp.position().z()   + chanUp.tubsParams().zHalfLength() + eps;
      z[5]    = chanDown.position().z() - chanDown.tubsParams().zHalfLength() - eps;
      z[6]    = chanDown.position().z() - chanDown.tubsParams().zHalfLength();
      z[7]    = chanDown.position().z() + chanDown.tubsParams().zHalfLength();
      z[8]    = chanDown.position().z() + chanDown.tubsParams().zHalfLength() + eps;
      z[9]    = ring.tubsParams().zHalfLength();

      rIn[0]  = ring.tubsParams().innerRadius();
      rIn[1]  = ring.tubsParams().innerRadius();
      rIn[2]  = chanUp.tubsParams().outerRadius();
      rIn[3]  = chanUp.tubsParams().outerRadius();
      rIn[4]  = ring.tubsParams().innerRadius();
      rIn[5]  = ring.tubsParams().innerRadius();
      rIn[6]  = chanUp.tubsParams().outerRadius();
      rIn[7]  = chanUp.tubsParams().outerRadius();
      rIn[8]  = ring.tubsParams().innerRadius();
      rIn[9]  = ring.tubsParams().innerRadius();

      for ( int i=0; i<n; ++i){
        rOut[i] = ring.tubsParams().outerRadius();
      }

      VolumeInfo innerRingInfo( ring.name(), ring.position(), ring.position() + devInfo.centerInWorld );
      innerRingInfo.solid = new G4Polycone( ring.name(),
                                            0.,
                                            2.*M_PI,
                                            n,
                                            z,
                                            rIn,
                                            rOut);

      bool visible = true;
      bool placePV = true;
      finishNesting( innerRingInfo,
                     findMaterialOrThrow(ring.materialName()),
                     0,
                     ring.position(),
                     devInfo.logical,
                     0,
                     visible,
                     G4Colour::Red(),
                     supportSolid,
                     forceAuxEdgeVisible,
                     placePV,
                     doSurfaceCheck
                     );


    }

    // Construct one sector envelope.
    // In this model, sector envelopes are arcs of a disk.

    // Sectors are identical other than placement - so get required properties from device 0, sector 0.
    Sector const& sec00(ttracker.getSector(SectorId(0,0)));

    // Azimuth of the centerline of a the sector enveleope: sectorCenterPhi
    // Upon construction and before placement, the sector envelope occupies [0,phiMax].
    double r1              = deviceEnvelopeParams.innerRadius();
    double r2              = sup.innerChannelUpstream().tubsParams().outerRadius();
    double halfChord       = sqrt ( r2*r2 - r1*r1 );
    double sectorCenterPhi = atan2(halfChord,r1);
    double phiMax          = 2.*sectorCenterPhi;

    // Get information about the channel position and depth.
    PlacedTubs const& chanUp(sup.innerChannelUpstream());

    // Internally all sector envelopes are the same.
    // Create one logical sector envelope but, for now, do not place it.
    // Fill it with straws and then place it multiple times.
    TubsParams s0(r1, r2, chanUp.tubsParams().zHalfLength(), 0., phiMax);
    VolumeInfo sec0Info = nestTubs( "SectorEnvelope0",
                                    s0,
                                    findMaterialOrThrow(chanUp.materialName()),
                                    0,
                                    zeroVector,
                                    devInfo.logical,
                                    0,                     // copyNo
                                    sectorEnvelopeVisible,
                                    G4Colour::Magenta(),
                                    sectorEnvelopeSolid,
                                    true,                  // edge visible
                                    doNotPlace,
                                    doSurfaceCheck
                                    );


    // The rotation matrix that will place the straw inside the sector envelope.
    // For straws on the upstream side of the support, the sign of the X rotation was chosen to put the
    // z axis of the straw volume to be positive when going clockwise; this is the same convention used
    // within the TTracker class.  For straws on the downstream side of the support, the z axis of the straw
    // volume is positive when going counter-clockwise.  This won't be important since we never work
    // in the straw-local coordiates within G4.
    CLHEP::HepRotationZ secRz(-sectorCenterPhi);
    CLHEP::HepRotationX secRx(M_PI/2.);
    G4RotationMatrix* sec00Rotation = reg.add(G4RotationMatrix(secRx*secRz));

    // The z of the center of the placed sector envelope, in Mu2e coordinates.
    // This carries a sign, depending on upstream/downstream.
    double zSector(0.);
    for ( int i=0; i<sec00.nLayers(); ++i){
      zSector += sec00.getStraw(StrawId(0,0,i,0)).getMidPoint().z();
    }
    zSector /= sec00.nLayers();

    // A unit vector in the direction from the origin to the wire center within the sector envelope.
    CLHEP::Hep3Vector unit( cos(sectorCenterPhi), sin(sectorCenterPhi), 0.);

    // Place the straws into the sector envelope.
    for ( std::vector<Layer>::const_iterator i=sec00.getLayers().begin(); i != sec00.getLayers().end(); ++i ){

      Layer const& lay(*i);

      if ( lay.id().getLayer() != 0 ) continue;

      for ( std::vector<Straw const*>::const_iterator j=lay.getStraws().begin();
            j != lay.getStraws().end(); ++j ){

        Straw const&       straw(**j);
        StrawDetail const& detail(straw.getDetail());

        //if ( straw.id().getStraw()%8 != 0 ) continue;

        // Mid point of the straw in Mu2e coordinates.
        CLHEP::Hep3Vector const& pos(straw.getMidPoint());

        // Mid point of the straw, within the sector envelope.
        double r = (CLHEP::Hep3Vector( pos.x(), pos.y(), 0.)).mag();
        CLHEP::Hep3Vector mid = r*unit;
        mid.setZ(pos.z() - zSector);

        int copyNo=straw.index().asInt();
        bool edgeVisible(true);
        bool placeIt(true);

        // The enclosing volume for the straw is made of gas.  The walls and the wire will be placed inside.
        VolumeInfo strawVol =  nestTubs( straw.name("TTrackerStrawGas_"),
                                         detail.getOuterTubsParams(),
                                         findMaterialOrThrow(detail.gasMaterialName()),
                                         sec00Rotation,
                                         mid,
                                         sec0Info.logical,
                                         copyNo,
                                         strawVisible,
                                         G4Colour::Red(),
                                         strawSolid,
                                         edgeVisible,
                                         placeIt,
                                         doSurfaceCheck
                                         );
        /*
        // Make the gas volume of this straw a sensitive detector.
        G4VSensitiveDetector *sd = G4SDManager::GetSDMpointer()->
          FindSensitiveDetector(SensitiveDetectorName::TrackerGas());
        if(sd) strawVol.logical->SetSensitiveDetector(sd);


        // Wall has 4 layers; the three metal layers sit inside the plastic layer.
        // The plastic layer sits inside the gas.
        VolumeInfo wallVol =  nestTubs( straw.name("TTrackerStrawWall_"),
                                        detail.wallMother(),
                                        findMaterialOrThrow(detail.wallMaterialName()),
                                        0,
                                        zeroVector,
                                        strawVol.logical,
                                        copyNo,
                                        strawLayeringVisible,
                                        G4Colour::Green(),
                                        strawLayeringSolid,
                                        edgeVisible,
                                        placeIt,
                                        doSurfaceCheck
                                        );

        VolumeInfo outerMetalVol =  nestTubs( straw.name("TTrackerStrawWallOuterMetal_"),
                                              detail.wallOuterMetal(),
                                              findMaterialOrThrow(detail.wallOuterMetalMaterialName()),
                                              0,
                                              zeroVector,
                                              wallVol.logical,
                                              copyNo,
                                              strawLayeringVisible,
                                              G4Colour::Blue(),
                                              strawLayeringSolid,
                                              edgeVisible,
                                              placeIt,
                                              doSurfaceCheck
                                              );

        VolumeInfo innerMetal1Vol =  nestTubs( straw.name("TTrackerStrawWallInnerMetal1_"),
                                               detail.wallInnerMetal1(),
                                               findMaterialOrThrow(detail.wallInnerMetal1MaterialName()),
                                               0,
                                               zeroVector,
                                               wallVol.logical,
                                               copyNo,
                                               strawLayeringVisible,
                                               G4Colour::Blue(),
                                               strawLayeringSolid,
                                               edgeVisible,
                                               placeIt,
                                               doSurfaceCheck
                                               );

        VolumeInfo innerMetal2Vol =  nestTubs( straw.name("TTrackerStrawWallInnerMetal2_"),
                                               detail.wallInnerMetal2(),
                                               findMaterialOrThrow(detail.wallInnerMetal2MaterialName()),
                                               0,
                                               zeroVector,
                                               wallVol.logical,
                                               copyNo,
                                               strawLayeringVisible,
                                               G4Colour::Blue(),
                                               strawLayeringSolid,
                                               edgeVisible,
                                               placeIt,
                                               doSurfaceCheck
                                               );

        VolumeInfo wireVol =  nestTubs( straw.name("TTrackerWireCore_"),
                                        detail.wireMother(),
                                        findMaterialOrThrow(detail.wireCoreMaterialName()),
                                        0,
                                        zeroVector,
                                        strawVol.logical,
                                        copyNo,
                                        strawLayeringVisible,
                                        G4Colour::Green(),
                                        strawLayeringSolid,
                                        edgeVisible,
                                        placeIt,
                                        doSurfaceCheck
                                        );

        VolumeInfo platingVol =  nestTubs( straw.name("TTrackerWirePlate_"),
                                           detail.wirePlate(),
                                           findMaterialOrThrow(detail.wirePlateMaterialName()),
                                           0,
                                           zeroVector,
                                           wireVol.logical,
                                           copyNo,
                                           strawLayeringVisible,
                                           G4Colour::Red(),
                                           strawLayeringSolid,
                                           edgeVisible,
                                           placeIt,
                                           doSurfaceCheck
                                           );
        if (ttrackerActiveWr_Wl_SD) {
	        G4VSensitiveDetector *sd = G4SDManager::GetSDMpointer()->
                                FindSensitiveDetector(SensitiveDetectorName::TrackerSWires());
                if (sd) {
                        wireVol.logical->SetSensitiveDetector(sd);
                        platingVol.logical->SetSensitiveDetector(sd);
                }

                sd = nullptr;
                sd = G4SDManager::GetSDMpointer()->
                                FindSensitiveDetector(SensitiveDetectorName::TrackerWalls());
                if (sd) {
                        wallVol.logical->SetSensitiveDetector(sd);
                        outerMetalVol.logical->SetSensitiveDetector(sd);
                        innerMetal1Vol.logical->SetSensitiveDetector(sd);
                        innerMetal2Vol.logical->SetSensitiveDetector(sd);
                }
        }
        */

        if ( verbosityLevel > 1 ){
          cout << "Detail for: " << straw.id() << " " << detail.Id()            << endl;
          cout << "           outerTubsParams: " << detail.getOuterTubsParams() << detail.gasMaterialName()             << endl;
          cout << "           wallMother:      " << detail.wallMother()         << detail.wallMotherMaterialName()      << endl;
          cout << "           wallOuterMetal:  " << detail.wallOuterMetal()     << detail.wallOuterMetalMaterialName()  << endl;
          cout << "           wallCore         " << detail.wallCore()           << detail.wallCoreMaterialName()        << endl;
          cout << "           wallInnerMetal1: " << detail.wallInnerMetal1()    << detail.wallInnerMetal1MaterialName() << endl;
          cout << "           wallInnerMetal2: " << detail.wallInnerMetal2()    << detail.wallInnerMetal2MaterialName() << endl;
          cout << "           wireMother:      " << detail.wireMother()         << detail.wireMotherMaterialName()      << endl;
          cout << "           wirePlate:       " << detail.wirePlate()          << detail.wirePlateMaterialName()       << endl;
          cout << "           wireCore:        " << detail.wireCore()           << detail.wireCoreMaterialName()        << endl;
        }

      } // end loop over straws within a layer
    } // end loop over layers


    // Place the device envelopes into the tracker mother.
    for ( int idev=0; idev<ttracker.nDevices(); ++idev ){

      // changes here affect StrawSD

      //if ( devDraw > -1 && idev != devDraw ) continue;
      if ( devDraw > -1 && idev > devDraw ) continue;

      verbosityLevel > 1 &&
        cout << "Debugging dev: " << idev << " " << devInfo.name << " devDraw: " << devDraw << endl;

      const Device& device = ttracker.getDevice(idev);

      // devPosition - is in the coordinate system of the Tracker mother volume.
      // device.origin() is in the detector coordinate system.
      CLHEP::Hep3Vector devPosition = device.origin()+motherOffset;

      if ( verbosityLevel > 1 ){
        cout << "Debugging -device.rotation(): " << -device.rotation() << " " << endl;
        cout << "Debugging device.origin():    " << device.origin() << " " << endl;
        cout << "Debugging position in mother: " << devPosition << endl;
      }

      // could we descend the final hierarchy and set the "true" copy numbers?

      // we may need to keep those pointers somewhre... (this is only the last one...)
      bool pMany(false);
      bool pSurfChk(false);
      devInfo.physical =  new G4PVPlacement(noRotation,
                                            devPosition,
                                            devInfo.logical,
                                            devInfo.name,
                                            motherInfo.logical,
                                            pMany,
                                            idev,
                                            pSurfChk);
      if ( doSurfaceCheck) {
         checkForOverlaps(devInfo.physical, config, verbosityLevel>0);
      }

      addSectors ( idev, devInfo, sec0Info, sectorCenterPhi, reg, config );

    } // end loop over devices


    // The section below this is for graphical debugging, not part of the TTracker construction.

    // Draw three boxes to mark the coordinate axes.
    // Dimensions and positions chosen to look right when drawing the first plane or the first station.
    cout << "Doing draw axes: " << config.getBool("ttracker.drawAxes",false) << endl;
    if ( config.getBool("ttracker.drawAxes",false) ) {

      double blockWidth(20.);
      double blockLength(100.);
      double extra(10.);
      double z0   = blockWidth + extra;
      double xOff = blockLength + blockWidth + extra;
      double yOff = blockLength + blockWidth + extra;
      double zOff = z0 + blockLength + blockWidth + extra;

      int copyNo(0);
      int isVisible(true);
      int isSolid(true);
      int edgeVisible(true);
      int placeIt(true);

      // A box marking the x-axis.
      double halfDimX[3];
      halfDimX[0] = blockLength;
      halfDimX[1] = blockWidth;
      halfDimX[2] = blockWidth;
      CLHEP::Hep3Vector xAxisPos(xOff,0.,z0);
      CLHEP::Hep3Vector trackerOffset(0.,0.,11800.);
      xAxisPos += trackerOffset;
      nestBox( "xAxis",
               halfDimX,
               envelopeMaterial,
               noRotation,
               xAxisPos,
               ds3Vac,
               copyNo,
               isVisible,
               G4Colour::Blue(),
               isSolid,
               edgeVisible,
               placeIt,
               doSurfaceCheck
               );

      // A box marking the y-axis.
      double halfDimY[3];
      halfDimY[0] = blockWidth;
      halfDimY[1] = blockLength;
      halfDimY[2] = blockWidth;
      CLHEP::Hep3Vector yAxisPos(0.,yOff,z0);
      yAxisPos += trackerOffset;
      nestBox( "yAxis",
               halfDimY,
               envelopeMaterial,
               noRotation,
               yAxisPos,
               ds3Vac,
               copyNo,
               isVisible,
               G4Colour::Red(),
               isSolid,
               edgeVisible,
               placeIt,
               doSurfaceCheck
               );

      // A box marking the z-axis.
      double halfDimZ[3];
      halfDimZ[0] = blockWidth;
      halfDimZ[1] = blockWidth;
      halfDimZ[2] = blockLength;
      CLHEP::Hep3Vector zAxisPos(0.,0.,zOff);
      zAxisPos += trackerOffset;
      nestBox( "zAxis",
               halfDimZ,
               envelopeMaterial,
               noRotation,
               zAxisPos,
               ds3Vac,
               copyNo,
               isVisible,
               G4Colour::Green(),
               isSolid,
               edgeVisible,
               placeIt,
               doSurfaceCheck
               );

      cout << "Pos: " << xAxisPos << " " << yAxisPos << " "<< zAxisPos << endl;
    }

    return motherInfo;

  } // end of constructTTrackerv3Detailed

} // end namespace mu2e