// C++ includes
#include <iostream>
#include <string>
#include <cmath>
#include <deque>
#include <map>
#include <list>

// Framework includes.
#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "DataFormats/Common/interface/Handle.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Services/interface/TFileService.h"
#include "FWCore/Framework/interface/TFileDirectory.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "DataFormats/Provenance/interface/Provenance.h"

// Root includes.
#include "TFile.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TNtuple.h"

// Mu2e includes.
#include "GeometryService/inc/GeometryService.hh"
#include "GeometryService/inc/GeomHandle.hh"
#include "GeometryService/inc/getTrackerOrThrow.hh"
#include "TrackerGeom/inc/Tracker.hh"
#include "TrackerGeom/inc/Straw.hh"
#include "ToyDP/inc/StrawHitCollection.hh"
#include "ToyDP/inc/StrawHitMCTruthCollection.hh"
#include "ToyDP/inc/DPIndexVectorCollection.hh"
#include "ToyDP/inc/StepPointMCCollection.hh"
#include "ToyDP/inc/ToyGenParticleCollection.hh"
#include "ToyDP/inc/SimParticleCollection.hh"
#include "ToyDP/inc/PhysicalVolumeInfoCollection.hh"
#include "ToyDP/inc/CaloHitCollection.hh"
#include "ToyDP/inc/CaloHitMCTruthCollection.hh"
#include "ToyDP/inc/CaloCrystalHitCollection.hh"
#include "ToyDP/inc/CaloCrystalOnlyHitCollection.hh"
#include "CalorimeterGeom/inc/Calorimeter.hh"
#include "Mu2eUtilities/inc/MCCaloUtilities.hh"

using namespace std;

namespace mu2e {


  class BkgRates : public edm::EDAnalyzer {
  public:
    explicit BkgRates(edm::ParameterSet const& pset):
      _diagLevel(pset.getUntrackedParameter<int>("diagLevel",0)),
      _trackerStepPoints(pset.getUntrackedParameter<string>("trackerStepPoints","tracker")),
      _makerModuleLabel(pset.getParameter<std::string>("makerModuleLabel")),
      _minimumEnergy(pset.getUntrackedParameter<double>("minimumEnergy",0.0001)), // MeV
      _nAnalyzed(0),
      _hHitMult(0),
      _hStrawEvt(0),
      _hRateUZ(0),
      _hRateU(0),
      _hRateUT(0),
      _hRateMaxU(0),
      _hCaloHitMult(0),
      _tNtup(0),
      _cNtup(0),
      _nGenParticles(0),
      _nDevices(36),
      _nSectors(6), 
      _nLayers(2),
      _nStrawsPerLay(50), 
      _nVanes(4),
      _nZCryPerVane(50),
      _nRCryPerVane(10)
    {
    }
    virtual ~BkgRates() {
      //cout << "NgenParticles " << _nGenParticles << endl;

      for (int i=0; i<_nDevices; ++i) {
        _hStrawRates[i]->Scale(1/(double)_nGenParticles);
      }

      for (int i=0; i<_nVanes; ++i) {
        _hCrystalRates[i]->Scale(1/(double)_nGenParticles);
      }

      _hRateUZ->Scale(1/(double)_nGenParticles);
      _hRateU->Scale(1/(double)_nGenParticles);
      _hRateUT->Scale(1/(double)_nGenParticles);

    }
    
    virtual void beginJob(edm::EventSetup const&);

    void analyze(edm::Event const& e, edm::EventSetup const&);

  private:

    void doTracker(edm::Event const& evt);

    void doCalorimeter(edm::Event const& evt);

    // Diagnostic level
    int _diagLevel;

    // Name of the tracker StepPoint collection
    std::string _trackerStepPoints;

    // Label of the module that made the hits.
    std::string _makerModuleLabel;

    double _minimumEnergy; //minimum energy deposition of hits

    //number of analyzed events
    int _nAnalyzed;


    TH1F* _hHitMult;
    TH1F* _hStrawEvt;
    TH2F* _hRateUZ;
    TH1F* _hRateU;
    TH2F* _hRateUT;
    TH1F* _hRateMaxU;
    TH1F* _hCaloHitMult;    

    TNtuple* _tNtup;
    TNtuple* _cNtup;

    int _nGenParticles;

    const int _nDevices, _nSectors, _nLayers, _nStrawsPerLay;
    const int _nVanes, _nZCryPerVane, _nRCryPerVane;

    vector<TH2F*> _hStrawRates; 
    vector<TH2F*> _hCrystalRates;

    MCCaloUtilities* CaloManager;

  };

  typedef  list< pair <SimParticleCollection::key_type , double> >  PairList;



  bool SortByEnergyT(const pair<SimParticleCollection::key_type,double> & a,
                    const pair<SimParticleCollection::key_type,double> & b ) {
    return a.second < b.second;
  }
  

  void PairListAdd(PairList& list, SimParticleCollection::key_type prt, double e) {
  
    for (PairList::iterator it = list.begin(); 
         it != list.end(); ++it) {
      if (it->first == prt) {
        it->second += e;
        return;
      }
    }
    list.push_back(pair<SimParticleCollection::key_type,double>(prt,e));
    return;
  }


  void BkgRates::beginJob(edm::EventSetup const& ) {

    CaloManager = new MCCaloUtilities();

    edm::Service<edm::TFileService> tfs;

    _hHitMult     = tfs->make<TH1F>( "hHitMult",    "Multiplicity of g4 hit per Straw ", 100,  0.,  100. );
    _hCaloHitMult = tfs->make<TH1F>( "hCaloHitMult",    "Multiplicity of g4 hit per Crystal ", 100,  0.,  100. );
    _hStrawEvt    = tfs->make<TH1F>( "hStrawEvt",   "Multiplicity of straws per event ", 200,  0.,  200. );
    _hRateUZ      = tfs->make<TH2F>( "hRateUZ",     "Straw hit in u and z coordinates",  80,  380.,  690., 40, -1550., 1550.);
    _hRateU       = tfs->make<TH1F>( "hRateU",      "Straw hit in u coordinate",         100,  380.,  690. ); 
    _hRateUT      = tfs->make<TH2F>( "hRateUT",     "Straw hit in u coordinate and time",80,  380.,  690., 40, 700., 1900.);
    //    _hRateMaxU    = tfs->make<TH1F>( "hRateMaxU",      ask Asset

    _tNtup        = tfs->make<TNtuple>( "StrawHits", "Straw Ntuple",
                                        "evt:time:dt:eDep:lay:dev:sec:strawId:MChitX:MChitY:v:vMC:z:nTrk:t1trkId:t1pdgId:t1genId:t1en:t1genE:t1genP:t2trkId:t2pdgId:t2genId:t2en:t2genE:t2genP:t3trkId:t3pdgId:t3genId:t3en:t3genE:t3genP:driftTime:driftDist" );
    _cNtup        = tfs->make<TNtuple>( "CaloHits", "Calo Ntuple",
                                        "evt:crTime:crE:crId:crVane:crX:crY:crZ:ESwr:EOutVane:NtrkOutside:OutsideE1:OutsidePdg1:OutsideE2:OutsidePdg2:OutsideE3:OutsidePdg3:NtrkGen:GenE1:GenPdg1:GenE2:GenPdg2:GenE3:GenPdg3" );

    for (int i=0; i<_nDevices; ++i) {
      stringstream name, descr;
      name << "hRateDev" << i;
      descr << "Straw rates in device " << i;
      
      _hStrawRates.push_back(tfs->make<TH2F>(name.str().c_str(), 
                                             descr.str().c_str(),
                                             _nStrawsPerLay, 0, _nStrawsPerLay,
                                             _nLayers*_nSectors, 0, _nLayers*_nSectors));
      _hStrawRates[i]->Sumw2();
    }

    for (int i=0; i<_nVanes; ++i) {
      stringstream name, descr;
      name << "hRateVane" << i;
      descr << "Crystal rates in vane " << i;  
      _hCrystalRates.push_back(tfs->make<TH2F>(name.str().c_str(), 
                                               descr.str().c_str(),
                                               _nZCryPerVane, 0, _nZCryPerVane,
                                               _nRCryPerVane, 0, _nRCryPerVane));
      _hCrystalRates[i]->Sumw2();
    }
    
  }    
  
  void BkgRates::analyze(edm::Event const& evt, edm::EventSetup const&) {

    ++_nAnalyzed;

    //*****test code******
        static int ncalls(0);
    ++ncalls;
    if (ncalls == 1) {
      // cout << "This should be done only in the first event" << endl;
    }

    doTracker(evt);

    doCalorimeter(evt);

  } // end of analyze

  void BkgRates::doTracker(edm::Event const& evt) {
    
    // Geometry info for the LTracker.
    // Get a reference to one of the L or T trackers.
    // Throw exception if not successful.
    const Tracker& tracker = getTrackerOrThrow();
    
    edm::Handle<StrawHitCollection> pdataHandle;
    evt.getByLabel(_makerModuleLabel,pdataHandle);
    StrawHitCollection const* hits = pdataHandle.product();

    // Get the persistent data about the StrawHitsMCTruth.
    
    edm::Handle<StrawHitMCTruthCollection> truthHandle;
    evt.getByLabel(_makerModuleLabel,truthHandle);
    StrawHitMCTruthCollection const* hits_truth = truthHandle.product();

    // Get the persistent data about pointers to StepPointMCs
    edm::Handle<DPIndexVectorCollection> mcptrHandle;
    evt.getByLabel(_makerModuleLabel,"StrawHitMCPtr",mcptrHandle);
    DPIndexVectorCollection const* hits_mcptr = mcptrHandle.product();

    // Get the persistent data about the StepPointMCs. More correct implementation
    // should look for product ids in DPIndexVectorCollection, rather than 
    // use producer name directly ("g4run"). 

    edm::Handle<StepPointMCCollection> mchitsHandle;
    evt.getByLabel("g4run",_trackerStepPoints,mchitsHandle);
    StepPointMCCollection const* mchits = mchitsHandle.product();

    /*    
    //Get and printout all straw (just to check)
    const std::deque<Straw>& allstraws = tracker.getAllStraws();
    for (size_t i = 0; i< allstraws.size(); ++i) {
    Straw str = allstraws[i];
    cout << "Index " << str.Index() 
    << "\t" << str.Id() << endl;
    }
    */

    if (!(hits->size() == hits_truth->size() &&
          hits_mcptr->size() == hits->size() ) ) {
      throw cms::Exception("RANGE")
        << "Strawhits: " << hits->size() 
        << " MCTruthStrawHits: " << hits_truth->size() 
        << " MCPtr: " << hits_mcptr->size(); 
    }

    // Get handles to the generated and simulated particles.
    edm::Handle<ToyGenParticleCollection> genParticles;
    evt.getByType(genParticles);

    _nGenParticles += genParticles->size();

    edm::Handle<SimParticleCollection> simParticles;
    evt.getByType(simParticles);

    // Handle to information about G4 physical volumes.
    edm::Handle<PhysicalVolumeInfoCollection> volumes;
    evt.getRun().getByType(volumes);

    // Some files might not have the SimParticle and volume information.
    bool haveSimPart = ( simParticles.isValid() && volumes.isValid() );

    // Other files might have empty collections.
    if ( haveSimPart ){
      haveSimPart = !(simParticles->empty() || volumes->empty());
    }

    size_t nStrawPerEvent = hits->size();
    _hStrawEvt->Fill(nStrawPerEvent);

    for (size_t i=0; i<nStrawPerEvent; ++i) {
      
      // Access data
      StrawHit        const&      hit(hits->at(i));
      StrawHitMCTruth const&    truth(hits_truth->at(i));
      DPIndexVector   const&    mcptr(hits_mcptr->at(i));

      double hitEnergy = hit.energyDep();

      //Skip the straw if the energy of the hit is smaller than the minimum required
      if (hitEnergy < _minimumEnergy) continue;

      //Get hit straw
      StrawIndex si = hit.strawIndex();
      Straw str = tracker.getStraw(si);
      StrawId sid = str.Id();
      LayerId lid = sid.getLayerId();
      DeviceId did = sid.getDeviceId();
      SectorId secid = sid.getSectorId();
      
      //Get coordinates of the hit:

      //X, Y and Z coordinate of the straw middle point

      const CLHEP::Hep3Vector stMidPoint3 = str.getMidPoint();
      double xc = stMidPoint3.getX();
      double yc = stMidPoint3.getY();
      double z = stMidPoint3.getZ();

      //u coordinate (radial from the center)
      double u = sqrt((xc*xc)+(yc*yc));

      //time of the hit
      double hitTime = hit.time(); 


      //direction of the straw
      const CLHEP::Hep3Vector stDirection3 = str.getDirection();

      //here the rates
      _hStrawRates[did]->Fill(sid.getStraw(),2*(secid.getSector())+lid.getLayer());
      _hRateUZ->Fill(u,z);
      _hRateU->Fill(u);
      _hRateUT->Fill(u,hitTime);

      // Get MC truth data
      double driftTime = truth.driftTime();
      double driftDistance = truth.driftDistance();

      //Position along the wire using mctruth info
      double vMC = truth.distanceToMid();

      //Position along the wire using dt and propagation velocity (c)
      const double signalVelocity = 299.792458; // mm/ns
      double v = 10e4 * hit.dt()/(2*signalVelocity); 

      const CLHEP::Hep3Vector HitPoint = stMidPoint3 + (v/stDirection3.mag())*stDirection3;
      const CLHEP::Hep3Vector MCHitPoint = stMidPoint3 + (vMC/stDirection3.mag())*stDirection3;
      
      if (fabs(v) > str.getHalfLength()) {
        if (_diagLevel > 0) cout << "Position along the wire bigger than halflength" << endl;
      }

      size_t nHitsPerStraw = mcptr.size();
      _hHitMult->Fill(nHitsPerStraw);

      float tntpArray[34];
      int idx(0);
      tntpArray[idx++] = evt.id().event();
      tntpArray[idx++] = hitTime;
      tntpArray[idx++] = hit.dt();
      tntpArray[idx++] = hitEnergy;
      tntpArray[idx++] = lid.getLayer();
      tntpArray[idx++] = did;
      tntpArray[idx++] = secid.getSector();
      tntpArray[idx++] = sid.getStraw();
      tntpArray[idx++] = MCHitPoint.getX();
      tntpArray[idx++] = MCHitPoint.getY();
      tntpArray[idx++] = v;
      tntpArray[idx++] = vMC;
      tntpArray[idx++] = z;

      //Get related G4 hits to identify the track. 


      //Map of track id as key, and vector index as value
      map<SimParticleCollection::key_type , size_t > StrawTracksMap;

      //Vectors of pdgId and GenId of the tracks associated to the strawhit
      vector<int>     PdgIdTracks;
      vector<int>     GenIdTracks;
      vector<double>  GenPartEnergy;
      vector<double>  GenPartMomentum;

      //List of trackId and energy deposition
      PairList TracksEDep;

      //common index for vectors
      size_t trackIdx(0);      

      for (size_t j = 0; j < mcptr.size(); ++j) {
        
        StepPointMC const& mchit = (*mchits)[mcptr[j].index];
        
        // The simulated particle that made this hit.
        SimParticleCollection::key_type trackId(mchit.trackId());

        //Find in the map if the track is already stored
        map<SimParticleCollection::key_type , size_t >::iterator it;
        it = StrawTracksMap.find(trackId);

        //if the contributing track id does not exist in the map
        //add an element to the map itself, energy to the list and pdgId and genId to the vectors
        if (it==StrawTracksMap.end()) {

          //insert track id in the trackId vector
          StrawTracksMap.insert(pair<SimParticleCollection::key_type, size_t>(trackId,trackIdx));

          //insert trackId, and energy deposition in the list     
          TracksEDep.push_back(pair<SimParticleCollection::key_type, double>(trackId,mchit.eDep()));

          
          if ( haveSimPart ){
            SimParticle const& sim = simParticles->at(trackId);
            
            // PDG Particle Id of the sim particle that made this hit.
            PdgIdTracks.push_back(sim.pdgId()); 
            
            // If this is a generated particle, which generator did it come from?
            // This default constructs to "unknown".
            if ( sim.fromGenerator() ){
              ToyGenParticle const& gen = genParticles->at(sim.generatorIndex());
              GenIdTracks.push_back(gen.generatorId().Id());
              GenPartEnergy.push_back(gen.momentum().e());
              GenPartMomentum.push_back(gen.momentum().vect().mag());
            } else if ( !sim.fromGenerator() ){ //if sim and gen info are not available set them to zero
              GenIdTracks.push_back(0);
              GenPartEnergy.push_back(0);
              GenPartMomentum.push_back(0);
            }
          } else if ( !haveSimPart) {
            PdgIdTracks.push_back(0);
            GenIdTracks.push_back(0);
            GenPartEnergy.push_back(0);
            GenPartMomentum.push_back(0);
          }

          //increment index
          trackIdx++;
        } else if (it != StrawTracksMap.end()) {          
          for (PairList::iterator it2 = TracksEDep.begin(); it2 != TracksEDep.end(); ++it2) {
            if (it2->first == trackId) {
              it2->second += mchit.eDep();
            }
          }
        }
      }
    
      TracksEDep.sort(SortByEnergyT);      
      
      int nTrkPerStraw = TracksEDep.size();
      
      if (nTrkPerStraw > 3) {
        if (_diagLevel > 0) {
          cout << "More than 3 different tracks contribute to the straw:"
               << "\nonly the first three with higher e deposit will be stored" << endl;
        }
      }

      tntpArray[idx++] = nTrkPerStraw;
      int counter = 0;

      for (PairList::reverse_iterator it = TracksEDep.rbegin();
           it != TracksEDep.rend(); ++it) {
        if (counter == 3) break;        
        
        size_t vec_idx = StrawTracksMap[it->first];

        tntpArray[idx++] = it->first.asInt();
        tntpArray[idx++] = PdgIdTracks[vec_idx];
        tntpArray[idx++] = GenIdTracks[vec_idx];
        tntpArray[idx++] = it->second;
        tntpArray[idx++] = GenPartEnergy[vec_idx];
        tntpArray[idx++] = GenPartMomentum[vec_idx];
        counter++;
      }
    
      
      //Fill with 0 the rest of the ntupla leaves 
      //if there are less than 3 tracks contributing
      //to the straw hit
      
      for (int add_idx = 0; add_idx < 3 - counter; ++add_idx) {
        tntpArray[idx++] = 0;
        tntpArray[idx++] = 0;
        tntpArray[idx++] = 0;
        tntpArray[idx++] = 0;
        tntpArray[idx++] = 0;
        tntpArray[idx++] = 0;
      }
      
      tntpArray[idx++] = driftTime;
      tntpArray[idx++] = driftDistance;
      
      _tNtup->Fill(tntpArray);
      
    } //end of Strawhits loop
  
  } // end of doTracker

  void BkgRates::doCalorimeter(edm::Event const& evt) {

    //    CaloManager->printOutCaloInfo();

    //Get handle to the calorimeter
    edm::Service<GeometryService> geom;
    if( ! geom->hasElement<Calorimeter>() ) return;
    GeomHandle<Calorimeter> cg;

    // Get handles to calorimeter collections
    edm::Handle<CaloHitCollection> caloHits;
    edm::Handle<CaloHitMCTruthCollection> caloMC; //unused
    edm::Handle<CaloCrystalHitCollection>  caloCrystalHits;

    // Get the persistent data about pointers to StepPointMCs
    edm::Handle<DPIndexVectorCollection> mcptrHandle;
    edm::Handle<StepPointMCCollection> steps;

    evt.getByLabel("CaloROHitsMaker","CaloHitMCCrystalPtr",mcptrHandle);
    evt.getByLabel("g4run","calorimeter",steps);
    evt.getByType(caloHits);
    evt.getByType(caloMC);
    evt.getByType(caloCrystalHits);

    DPIndexVectorCollection const* hits_mcptr = mcptrHandle.product();
    StepPointMCCollection const* mchits = steps.product();
    if (!( caloHits.isValid() && caloMC.isValid())) {
      return;
    }

    if (!caloCrystalHits.isValid()) {
      cout << "NO CaloCrystalHits" << endl;
      return;
    }


    // Get handles to the generated and simulated particles.
    edm::Handle<ToyGenParticleCollection> genParticles;
    evt.getByType(genParticles);

    edm::Handle<SimParticleCollection> simParticles;
    evt.getByType(simParticles);
    
    // Handle to information about G4 physical volumes.
    edm::Handle<PhysicalVolumeInfoCollection> volumes;
    evt.getRun().getByType(volumes);
    
    // Some files might not have the SimParticle and volume information.
    bool haveSimPart = ( simParticles.isValid() && volumes.isValid() );
    
    // Other files might have empty collections.
    if ( haveSimPart ){
      haveSimPart = !(simParticles->empty() || volumes->empty());
    }
    
    map<uint32_t, size_t> CHMap;

    for (size_t j=0; j<caloHits->size(); ++j) {
      
      CaloHit const & ahit = (*caloHits).at(j);
      CHMap.insert(pair<uint32_t, size_t>(ahit.id(), j));

    }

    for ( size_t i=0; i<caloCrystalHits->size(); ++i ) {
      
      double EfromShower = 0;
      double EfromOutside1 = 0;
      double EfromOutside2 = 0;
      double EfromOutside3 = 0;
      double EfromOtherVane = 0;
      double EfromGenerated1 = 0;
      double EfromGenerated2 = 0;
      double EfromGenerated3 = 0;
      int OutsideTrkPdgId1 = 0;
      int OutsideTrkPdgId2 = 0;
      int OutsideTrkPdgId3 = 0;
      int GeneratedTrkPdgId1 = 0;
      int GeneratedTrkPdgId2 = 0;
      int GeneratedTrkPdgId3 = 0;
      int nOutsideTrk = 0;
      int nGeneratedTrk = 0;
      
      CaloCrystalHit const & hit = (*caloCrystalHits).at(i);

      if (hit.energyDep() < _minimumEnergy) continue;

      DPIndexVector const & ROIds  = hit.roIds();

      bool readCryOnce(false);
      float cntpArray[24];
      int idx(0);

      //List of trackId and energy deposition
      PairList OutsideEDep;
      PairList GeneratedEDep;

      for (size_t it = 0; 
           it < ROIds.size() ; ++it ) {
        
        size_t CollectionPosition = CHMap[ROIds.at(it).index];

        CaloHit const & thehit = (*caloHits).at(CollectionPosition);
        
        if (!readCryOnce) {
          CLHEP::Hep3Vector cryCenter =  cg->getCrystalOriginByRO(thehit.id());
          int vane = cg->getVaneByRO(thehit.id());
          int Zcry = cg->getCrystalZByRO(thehit.id());
          int Rcry = cg->getCrystalRByRO(thehit.id());
          _hCrystalRates[vane]->Fill(Zcry,Rcry);
          cntpArray[idx++] = evt.id().event();
          cntpArray[idx++] = hit.time();
          cntpArray[idx++] = hit.energyDep();
          cntpArray[idx++] = cg->getCrystalByRO(thehit.id());
          cntpArray[idx++] = vane;
          cntpArray[idx++] = cryCenter.getX() + 3904.;  //value used to shift in tracker coordinate system
          cntpArray[idx++] = cryCenter.getY();
          cntpArray[idx++] = cryCenter.getZ() - 10200;  //value used to shift in tracker coordinate system
          readCryOnce = true;
        }

        DPIndexVector const & mcptr(hits_mcptr->at(CollectionPosition));        
        size_t nHitsPerCrystal = mcptr.size();
        _hCaloHitMult->Fill(nHitsPerCrystal);

        for (size_t j2=0; j2<nHitsPerCrystal; ++j2) {
          
          StepPointMC const& mchit = (*mchits)[mcptr[j2].index];
          // The simulated particle that made this hit.
          SimParticleCollection::key_type trackId(mchit.trackId());

          CaloManager->setTrackAndRO(evt, trackId, ROIds.at(it).index );  
          
          //cout << "Original Vane: " << CaloManager->localVane()
          //     << "\nStarting Vane: " << CaloManager->startingVane() << endl;
          if (CaloManager->localVane() == CaloManager->startingVane()) {
            EfromShower += mchit.eDep();
          }
          if (CaloManager->localVane() != CaloManager->startingVane() &&
              CaloManager->startingVane() != -1) {
            EfromOtherVane += mchit.eDep();
          }
          
          if (CaloManager->fromOutside()) {
            if (!CaloManager->generated()) {
              PairListAdd(OutsideEDep, trackId, mchit.eDep());
            }
            
            if (CaloManager->generated()) {
              PairListAdd(GeneratedEDep, trackId, mchit.eDep());
            }
          }
        }
      }
      
      OutsideEDep.sort(SortByEnergyT);
      GeneratedEDep.sort(SortByEnergyT);
      
      nOutsideTrk = OutsideEDep.size();
      nGeneratedTrk = GeneratedEDep.size();
      
      if (nOutsideTrk > 3) {
        if (_diagLevel > 0) {
          cout << "More than 3 different tracks from outside the calorimeter contribute to the crystal:"
               << "\nonly the first three with higher e deposit will be stored" << endl;
        }
      }
      
      if (nGeneratedTrk > 3) {
        if (_diagLevel > 0) {
          cout << "More than 3 different generated tracks contribute to the crystal:"
               << "\nonly the first three with higher e deposit will be stored" << endl;
        }
      }
      
      int counter = 0;
      
      for (PairList::reverse_iterator it = OutsideEDep.rbegin();
           it != OutsideEDep.rend(); ++it) {
        if (counter == 3) break;
        if (counter == 0) {
          EfromOutside1 = it->second;
          SimParticle const& sim = simParticles->at(it->first);
          OutsideTrkPdgId1 = sim.pdgId();
          counter++;
        }
        if (counter == 1) {
          EfromOutside2 = it->second;
          SimParticle const& sim = simParticles->at(it->first);
          OutsideTrkPdgId2 = sim.pdgId();
          counter++;
        }
        if (counter == 2) {
          EfromOutside3 = it->second;
          SimParticle const& sim = simParticles->at(it->first);
          OutsideTrkPdgId3 = sim.pdgId();
          counter++;
        }
      }
      
      counter = 0;
      
      for (PairList::reverse_iterator it = GeneratedEDep.rbegin();
           it != GeneratedEDep.rend(); ++it) {
        if (counter == 3) break;
        if (counter == 0) {
          EfromGenerated1 = it->second;
          SimParticle const& sim = simParticles->at(it->first);
          GeneratedTrkPdgId1 = sim.pdgId();
          counter++;
        }
        if (counter == 1) {
          EfromGenerated2 = it->second;
          SimParticle const& sim = simParticles->at(it->first);
          GeneratedTrkPdgId2 = sim.pdgId();
          counter++;
        }
        if (counter == 2) {
          EfromGenerated3 = it->second;
          SimParticle const& sim = simParticles->at(it->first);
          GeneratedTrkPdgId3 = sim.pdgId();
          counter++;
        }
      }
    
    
      cntpArray[idx++] = EfromShower;  
      cntpArray[idx++] = EfromOtherVane;
      cntpArray[idx++] = nOutsideTrk;
      cntpArray[idx++] = EfromOutside1;
      cntpArray[idx++] = OutsideTrkPdgId1;
      cntpArray[idx++] = EfromOutside2;
      cntpArray[idx++] = OutsideTrkPdgId2;
      cntpArray[idx++] = EfromOutside3;
      cntpArray[idx++] = OutsideTrkPdgId3;
      cntpArray[idx++] = nGeneratedTrk;
      cntpArray[idx++] = EfromGenerated1;
      cntpArray[idx++] = GeneratedTrkPdgId1;
      cntpArray[idx++] = EfromGenerated2;
      cntpArray[idx++] = GeneratedTrkPdgId2;
      cntpArray[idx++] = EfromGenerated3;
      cntpArray[idx++] = GeneratedTrkPdgId3;

        
      _cNtup->Fill(cntpArray);        
      
    }  
  } // end of doCalorimeter
  
}

using mu2e::BkgRates;
DEFINE_FWK_MODULE(BkgRates);
 
