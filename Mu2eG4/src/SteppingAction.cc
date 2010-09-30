//
// Called at every G4 step.
//
// $Id: SteppingAction.cc,v 1.12 2010/09/30 21:25:00 kutschke Exp $
// $Author: kutschke $ 
// $Date: 2010/09/30 21:25:00 $
//
// Original author Rob Kutschke
//

// C++ includes
#include <cstdio>
#include <cmath>

// Framework includes
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/Utilities/interface/Exception.h"

// Mu2e includes
#include "Mu2eG4/inc/SteppingAction.hh"
#include "Mu2eUtilities/inc/SimpleConfig.hh"
#include "Mu2eUtilities/inc/PDGCode.hh"
#include "Mu2eG4/inc/getPhysicalVolumeOrThrow.hh"

// G4 includes
#include "G4Event.hh"
#include "G4EventManager.hh"
#include "G4PhysicalVolumeStore.hh"
#include "G4RunManager.hh"
#include "G4SteppingManager.hh"
#include "G4String.hh"

using namespace std;

namespace mu2e {

  SteppingAction::SteppingAction( const SimpleConfig& config ):

    // Parameters from the run time configuration.
    _doKillLowEKine(false),
    _doKillInHallAir(false),
    _killerVerbose(false),
    _eKineMin(0.1),
    _debugEventList(),
    _debugTrackList(),

    // Other parameters.
    _hallAirPhysVol(0),
    _lastPosition(),
    _lastMomentum(),
    _zref(0.){ 

    _doKillLowEKine  = config.getBool("g4SteppingAction.killLowEKine",  _doKillLowEKine);
    _doKillInHallAir = config.getBool("g4SteppingAction.killInHallAir", _doKillInHallAir);
    _killerVerbose   = config.getBool("g4SteppingAction.killerVerbose", _killerVerbose);

    _eKineMin        = config.getDouble("g4SteppingAction.eKineMin", _eKineMin );

    // Get list of events for which to make debug printout.
    string key("g4.steppingActionEventList");
    if ( config.hasName(key) ){
      vector<int> list;
      config.getVectorInt(key,list);
      _debugEventList.add(list);
    }

    // Get list of tracks (within the above events) for which to make debug printout.
    // If empty list, then make printout for all tracks.
    string key2("g4.steppingActionTrackList");
    if ( config.hasName(key2) ){
      vector<int> list;
      config.getVectorInt(key2,list);
      _debugTrackList.add(list);
    }

    // Get list of particles to keep or to drop in stepping action
    if ( config.hasName("g4.steppingActionDropPDG") ){
      config.getVectorInt("g4.steppingActionDropPDG",_pdgToDrop);
    }
    if( _pdgToDrop.size()>0 ) {
      cout << "Drop these particles in the SteppingAction: ";
      for( size_t i=0; i<_pdgToDrop.size(); ++i ) cout << _pdgToDrop[i] << ",";
      cout << endl;
    }

    // Get maximum allowed number of steps per event
    _maxSteps = config.getInt("g4.steppingActionMaxSteps", 0);
    if( _maxSteps>0 ) {
      cout << "Limit maximum number of steps in SteppingAction to "
	   << _maxSteps << endl;
    }

  }

  // A helper function to manage the printout.
  void printit( G4String const& s, 
                G4int id,
                G4ThreeVector const& pos,
                G4ThreeVector const& mom,
                double localTime,
                double globalTime ){

    // It is easier to line up printout in columns with printf than with cout.
    printf ( "%-8s %4d %15.4f %15.4f %15.4f %15.4f %15.4f %15.4f %15.4f %13.4f %13.4f\n",
             s.data(), id,
             pos.x(), pos.y(), pos.z(), 
             mom.x(), mom.y(), mom.z(),
             mom.mag(),
             localTime, globalTime);
  }

  void SteppingAction::beginRun(){
    _hallAirPhysVol  = getPhysicalVolumeOrThrow("HallAir");
  }

  void SteppingAction::UserSteppingAction(const G4Step* step){  

    _nSteps++;

    G4Track* track = step->GetTrack();


    // Do we reached maximum allowed number of steps per event?
    if( _maxSteps>0 && _nSteps>_maxSteps ) {
      cout << "SteppingAction: kill particle pdg=" 
	   << track->GetDefinition()->GetPDGEncoding()
	   << " due to large number of steps." << endl;
      track->SetTrackStatus(fStopAndKill);

    // If particle is in the drop list - drop it
    } else if( _pdgToDrop.size()>0 ) {
      
      for( size_t i=0; i<_pdgToDrop.size(); ++i ) {
	if( track->GetDefinition()->GetPDGEncoding() == _pdgToDrop[i] ) {
	  track->SetTrackStatus(fStopAndKill);
	  break;
	}
      }
    }	  

    if ( _doKillInHallAir &&  killInHallAir(track) ){
      track->SetTrackStatus(fStopAndKill);
    } else if ( _doKillLowEKine && killLowEKine(track) ){
      track->SetTrackStatus(fStopAndKill);
    }
    
    // Do we want to do make debug printout for this event?
    if ( !_debugEventList.inList() ) return;

    // Get information about this track.
    G4int id = track->GetTrackID();

    // If no tracks are listed, then printout for all tracks.
    // If some tracks are listed, then printout only for those tracks.
    if ( _debugTrackList.size() > 0 ){
      if ( !_debugTrackList.inList(id) ) return;
    }

    //G4Event const* event = G4RunManager::GetRunManager()->GetCurrentEvent();

    // Pre and post stepping points.
    G4StepPoint const* prept  = step->GetPreStepPoint();
    G4StepPoint const* postpt = step->GetPostStepPoint();

    // Position and momentum at the the pre point.
    G4ThreeVector const& pos = prept->GetPosition();
    G4ThreeVector const& mom = prept->GetMomentum();
  
    // On the last step on a track the post step point does not have an
    // associated physical volume. So we need to protect against that.
    G4String preVolume, postVolume;
    G4int preCopy(-1), postCopy(-1);

    // Get the names if they are defined.
    if ( prept->GetPhysicalVolume() ){
      preVolume = prept->GetPhysicalVolume()->GetName();
      preCopy   = prept->GetPhysicalVolume()->GetCopyNo();
    }

    if ( postpt->GetPhysicalVolume() ){
      postVolume = postpt->GetPhysicalVolume()->GetName();
      postCopy   = postpt->GetPhysicalVolume()->GetCopyNo();
    }

    // On the forward trace, save the particle status at the start of the 
    // last reporting volume.
    bool save = (std::abs(pos.z()+_zref)<0.0001) && (mom.z() > 0.);

    // On the backward trace, report the position when at the first 
    // reporting volume.
    //bool report = (std::abs(pos.z()-_zref)<0.0001) && (mom.z() < 0.);

    // Save the status.
    if ( save ){
      _lastPosition = prept->GetPosition();
      _lastMomentum = prept->GetMomentum();
    }

    // Status report.
    printf ( "Step number: %d\n", _nSteps ); 

    printit ( "Pre: ", id, 
              prept->GetPosition(),
              prept->GetMomentum(),
              track->GetLocalTime(),
              track->GetGlobalTime()
              );


    printit ( "Step:", id, 
              track->GetPosition(),
              track->GetMomentum(),
              track->GetLocalTime(),
              track->GetGlobalTime()
              );


    printit ( "Post: ", id, 
              postpt->GetPosition(),
              postpt->GetMomentum(),
              track->GetLocalTime(),
              track->GetGlobalTime()
              );
    fflush(stdout);

    cout << "Pre  Volume and copy: " << preVolume  << " " << preCopy  << endl;
    cout << "Post Volume and copy: " << postVolume << " " << postCopy << endl;

    printf ( "\n");
    fflush(stdout);

  } // end UserSteppingAction


  // Kill tracks that drop below the kinetic energy cut.
  // It might be smarter to program G4 to do this itself?
  bool SteppingAction::killLowEKine ( const G4Track* trk ){
    if ( trk->GetKineticEnergy() < _eKineMin ){
      if ( _killerVerbose ){
        cout << "Killed track: low energy." << endl;
      }
      return true;
    }
    return false;
  }

  // Kill tracks that enter the hall air.
  bool SteppingAction::killInHallAir( const G4Track* trk ){

    // If we are not in the hall air, keep the track.
    if ( trk->GetVolume() != _hallAirPhysVol ) { 
      return false; 
    }

    if ( _killerVerbose ){
      cout << "Killed track: in Hall Air." << endl;
    }
    return true;
  }

  void SteppingAction::BeginOfEvent() {
    //_nSteps = 0;
  }
  
  void SteppingAction::EndOfEvent() {
  }

  void SteppingAction::BeginOfTrack() {
    _nSteps = 0;
  }
  
  void SteppingAction::EndOfTrack() {
  }

} // end namespace mu2e

