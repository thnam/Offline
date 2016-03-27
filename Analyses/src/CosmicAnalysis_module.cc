#include <iostream>
#include <memory>
#include <string>

#include <TDirectory.h>
#include <TMath.h>
#include <TFile.h>
#include <TTree.h>
#include <TROOT.h>

#include "CLHEP/Units/GlobalSystemOfUnits.h"
#include "CosmicRayShieldGeom/inc/CosmicRayShield.hh"
#include "DataProducts/inc/CRSScintillatorBarIndex.hh"
#include "GeometryService/inc/DetectorSystem.hh"
#include "GeometryService/inc/GeomHandle.hh"
#include "GeometryService/inc/GeometryService.hh"
#include "HepPID/ParticleName.hh"
#include "MCDataProducts/inc/GenParticle.hh"
#include "MCDataProducts/inc/GenParticleCollection.hh"
#include "MCDataProducts/inc/PhysicalVolumeInfoMultiCollection.hh"
#include "MCDataProducts/inc/MCTrajectoryCollection.hh"
#include "MCDataProducts/inc/StepPointMCCollection.hh"
#include "MCDataProducts/inc/StrawDigiMCCollection.hh"
#include "Mu2eUtilities/inc/PhysicalVolumeMultiHelper.hh"
#include "RecoDataProducts/inc/StrawHitCollection.hh"
#include "RecoDataProducts/inc/CrvCoincidenceCheckResult.hh"
#include "RecoDataProducts/inc/CrvRecoPulsesCollection.hh"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "fhiclcpp/ParameterSet.h"

using namespace CLHEP;
#include "RecoDataProducts/inc/KalRepCollection.hh"
#include "BTrkData/inc/TrkStrawHit.hh"
#include "BTrk/KalmanTrack/KalRep.hh"

typedef struct
{
  double cosmic_pos[3];
  double cosmic_E, cosmic_p;
  double cosmic_ph, cosmic_th, cosmic_costh;
  char   cosmic_particle[100];

  double reco_t0;

  char   simreco_particle[100];
  char   simreco_production_process[100], simreco_production_volume[100];
  double simreco_startp, simreco_endp, simreco_startp_z;
  double simreco_pos[3];

  double zplane1[3], zplane2[3], zplane3[3];
  double xplane1[3], xplane2[3], xplane3[3];
  double yplane1[3];

  double firstCoincidenceHitTime;
  bool   CRVhit_allSectors;
  bool   CRVhit[8];
  double CRVhitTime[8];
  double CRVhitZ[8];

  int    run_number, subrun_number, event_number;

  char filename[200];
  
  void clear()
  {
    for(int i=0; i<3; i++)
    {
      zplane1[i]=NAN;
      zplane2[i]=NAN;
      zplane3[i]=NAN;
      xplane1[i]=NAN;
      xplane2[i]=NAN;
      xplane3[i]=NAN;
      yplane1[i]=NAN;
    }

    firstCoincidenceHitTime=NAN;
    
    CRVhit_allSectors=false;
    for(int i=0; i<8; i++)
    {
      CRVhit[i]=false;
      CRVhitTime[i]=NAN;
      CRVhitZ[i]=NAN;
    }
  };
} EventInfo;

namespace mu2e
{
  class CosmicAnalysis : public art::EDAnalyzer
  {
    public:
    explicit CosmicAnalysis(fhicl::ParameterSet const &pset);
    virtual ~CosmicAnalysis() { }
    virtual void beginJob();
    void analyze(const art::Event& e);
    void findCrossings(const art::Event& event, const cet::map_vector_key& particleKey);

    private:
    std::string _fitterModuleLabel;
    std::string _fitterModuleInstance;
    std::string _g4ModuleLabel;
    std::string _generatorModuleLabel;
    std::string _hitmakerModuleLabel;
    std::string _hitmakerModuleInstance;
    std::string _crvCoincidenceModuleLabel;
    std::string _crvRecoPulsesModuleLabel;
    std::string _volumeModuleLabel;
    std::string _filenameSearchPattern;
    EventInfo   _eventinfo;
    TTree      *_tree;

    CLHEP::Hep3Vector _detSysOrigin;
  };

  CosmicAnalysis::CosmicAnalysis(fhicl::ParameterSet const &pset)
    :
    art::EDAnalyzer(pset), 
    _fitterModuleLabel(pset.get<std::string>("fitterModuleLabel")),
    _fitterModuleInstance(pset.get<std::string>("fitterModuleInstance")),
    _g4ModuleLabel(pset.get<std::string>("g4ModuleLabel","detectorFilter")),
    _generatorModuleLabel(pset.get<std::string>("generatorModuleLabel")),
    _hitmakerModuleLabel(pset.get<std::string>("hitmakerModuleLabel")),
    _hitmakerModuleInstance(pset.get<std::string>("hitmakerModuleInstance")),
    _crvCoincidenceModuleLabel(pset.get<std::string>("crvCoincidenceModuleLabel")),
    _crvRecoPulsesModuleLabel(pset.get<std::string>("crvRecoPulsesModuleLabel")),
    _volumeModuleLabel(pset.get<std::string>("volumeModuleLabel")),
    _filenameSearchPattern(pset.get<std::string>("filenameSearchPattern"))
  {
  }

  void CosmicAnalysis::beginJob()
  {
    art::ServiceHandle<art::TFileService> tfs;
    art::TFileDirectory tfdir = tfs->mkdir("cosmicInfo");
    _tree = tfdir.make<TTree>("cosmicInfoTree","cosmicInfoTree");

    EventInfo &e=_eventinfo;
    _tree->Branch("cosmic_pos",e.cosmic_pos,"cosmic_pos[3]/D");
    _tree->Branch("cosmic_E",&e.cosmic_E,"cosmic_E/D");
    _tree->Branch("cosmic_p",&e.cosmic_p,"cosmic_p/D");
    _tree->Branch("cosmic_ph",&e.cosmic_ph,"cosmic_ph/D");
    _tree->Branch("cosmic_th",&e.cosmic_th,"cosmic_th/D");
    _tree->Branch("cosmic_costh",&e.cosmic_costh,"cosmic_costh/D");
    _tree->Branch("cosmic_particle",e.cosmic_particle,"cosmic_particle[100]/C");
    _tree->Branch("reco_t0",&e.reco_t0,"reco_t0/D");
    _tree->Branch("simreco_particle",e.simreco_particle,"simreco_particle[100]/C");
    _tree->Branch("simreco_production_process",e.simreco_production_process,"simreco_production_process[100]/C");
    _tree->Branch("simreco_production_volume",e.simreco_production_volume,"simreco_production_volume[100]/C");
    _tree->Branch("simreco_startp",&e.simreco_startp,"simreco_startp/D");
    _tree->Branch("simreco_endp",&e.simreco_endp,"simreco_endp/D");
    _tree->Branch("simreco_startp_z",&e.simreco_startp_z,"simreco_startp_z/D");
    _tree->Branch("simreco_pos",e.simreco_pos,"simreco_pos[3]/D");
    _tree->Branch("zplane1",e.zplane1,"zplane1[3]/D");
    _tree->Branch("zplane2",e.zplane2,"zplane2[3]/D");
    _tree->Branch("zplane3",e.zplane3,"zplane3[3]/D");
    _tree->Branch("xplane1",e.xplane1,"xplane1[3]/D");
    _tree->Branch("xplane2",e.xplane2,"xplane2[3]/D");
    _tree->Branch("xplane3",e.xplane3,"xplane3[3]/D");
    _tree->Branch("yplane1",e.yplane1,"yplane1[3]/D");
    _tree->Branch("firstCoincidenceHitTime",&e.firstCoincidenceHitTime,"firstCoincidenceHitTime/D");
    _tree->Branch("CRVhit_allSectors",&e.CRVhit_allSectors,"CRVhit_allSectors/O");
    _tree->Branch("CRVhit",e.CRVhit,"CRVhit[8]/O");
    _tree->Branch("CRVhitTime",e.CRVhitTime,"CRVhitTime[8]/D");
    _tree->Branch("CRVhitZ",e.CRVhitTime,"CRVhitZ[8]/D");
    _tree->Branch("run_number",&e.run_number,"run_number/I");
    _tree->Branch("subrun_number",&e.subrun_number,"subrun_number/I");
    _tree->Branch("event_number",&e.event_number,"event_number/I");
    _tree->Branch("filename",e.filename,"filename[200]/C");
  }

  void CosmicAnalysis::findCrossings(const art::Event& event, const cet::map_vector_key& particleKey)
  {
    double xCrossing1= -2525.4; //CRV-R
    double xCrossing2=  2525.4; //CRV-L
    double xCrossing3=  3800.0; //TS entrance
    double yCrossing1=  2589.7; //CRV-T / CRV-TS
    double zCrossing1=-12496.8; //CRV-U
    double zCrossing2= -7512.6; //open left side at the TS entrance
    double zCrossing3=  8571.0; //CRV-D

    std::string productInstanceName="";
    art::Handle<mu2e::MCTrajectoryCollection> _mcTrajectories;
    if(event.getByLabel(_g4ModuleLabel,productInstanceName,_mcTrajectories))
    {
      std::map<art::Ptr<mu2e::SimParticle>,mu2e::MCTrajectory>::const_iterator traj_iter;
      for(traj_iter=_mcTrajectories->begin(); traj_iter!=_mcTrajectories->end(); traj_iter++)
      {
        if(traj_iter->first->id()==particleKey) 
        {
          const std::vector<CLHEP::HepLorentzVector> &points = traj_iter->second.points();
          for(unsigned int i=1; i<points.size(); i++)
          {
            CLHEP::Hep3Vector point1=points[i-1]-_detSysOrigin;
            CLHEP::Hep3Vector point2=points[i]-_detSysOrigin;
            CLHEP::Hep3Vector diffVect=point2-point1;
            if((point1.z()>=zCrossing1 && point2.z()<=zCrossing1)
            || (point1.z()<=zCrossing1 && point2.z()>=zCrossing1))
            {
              double ratio=0;
              if(diffVect.z()!=0) ratio=(zCrossing1 - point1.z())/diffVect.z();
              CLHEP::Hep3Vector crossPoint=ratio*diffVect+point1;
              _eventinfo.zplane1[0]=crossPoint.x();
              _eventinfo.zplane1[1]=crossPoint.y();
              _eventinfo.zplane1[2]=crossPoint.z();
            }
            if((point1.z()>=zCrossing2 && point2.z()<=zCrossing2)
            || (point1.z()<=zCrossing2 && point2.z()>=zCrossing2))
            {
              double ratio=0;
              if(diffVect.z()!=0) ratio=(zCrossing2 - point1.z())/diffVect.z();
              CLHEP::Hep3Vector crossPoint=ratio*diffVect+point1;
              _eventinfo.zplane2[0]=crossPoint.x();
              _eventinfo.zplane2[1]=crossPoint.y();
              _eventinfo.zplane2[2]=crossPoint.z();
            }
            if((point1.z()>=zCrossing3 && point2.z()<=zCrossing3)
            || (point1.z()<=zCrossing3 && point2.z()>=zCrossing3))
            {
              double ratio=0;
              if(diffVect.z()!=0) ratio=(zCrossing3 - point1.z())/diffVect.z();
              CLHEP::Hep3Vector crossPoint=ratio*diffVect+point1;
              _eventinfo.zplane3[0]=crossPoint.x();
              _eventinfo.zplane3[1]=crossPoint.y();
              _eventinfo.zplane3[2]=crossPoint.z();
            }
            if((point1.x()>=xCrossing1 && point2.x()<=xCrossing1)
            || (point1.x()<=xCrossing1 && point2.x()>=xCrossing1))
            {
              double ratio=0;
              if(diffVect.x()!=0) ratio=(xCrossing1 - point1.x())/diffVect.x();
              CLHEP::Hep3Vector crossPoint=ratio*diffVect+point1;
              _eventinfo.xplane1[0]=crossPoint.x();
              _eventinfo.xplane1[1]=crossPoint.y();
              _eventinfo.xplane1[2]=crossPoint.z();
            }
            if((point1.x()>=xCrossing2 && point2.x()<=xCrossing2)
            || (point1.x()<=xCrossing2 && point2.x()>=xCrossing2))
            {
              double ratio=0;
              if(diffVect.x()!=0) ratio=(xCrossing2 - point1.x())/diffVect.x();
              CLHEP::Hep3Vector crossPoint=ratio*diffVect+point1;
              _eventinfo.xplane2[0]=crossPoint.x();
              _eventinfo.xplane2[1]=crossPoint.y();
              _eventinfo.xplane2[2]=crossPoint.z();
            }
            if((point1.x()>=xCrossing3 && point2.x()<=xCrossing3)
            || (point1.x()<=xCrossing3 && point2.x()>=xCrossing3))
            {
              double ratio=0;
              if(diffVect.x()!=0) ratio=(xCrossing3 - point1.x())/diffVect.x();
              CLHEP::Hep3Vector crossPoint=ratio*diffVect+point1;
              _eventinfo.xplane3[0]=crossPoint.x();
              _eventinfo.xplane3[1]=crossPoint.y();
              _eventinfo.xplane3[2]=crossPoint.z();
            }
            if((point1.y()>=yCrossing1 && point2.y()<=yCrossing1)
            || (point1.y()<=yCrossing1 && point2.y()>=yCrossing1))
            {
              double ratio=0;
              if(diffVect.y()!=0) ratio=(yCrossing1 - point1.y())/diffVect.y();
              CLHEP::Hep3Vector crossPoint=ratio*diffVect+point1;
              _eventinfo.yplane1[0]=crossPoint.x();
              _eventinfo.yplane1[1]=crossPoint.y();
              _eventinfo.yplane1[2]=crossPoint.z();
            }
          }
        }
      }
    }
  }

  inline bool operator < (const art::Ptr<mu2e::SimParticle> l, const art::Ptr<mu2e::SimParticle> r)
  {
    return l->id()<r->id();
  }

  void CosmicAnalysis::analyze(const art::Event& event)
  {
    _eventinfo.clear();

    for(int i=0; i<gROOT->GetListOfFiles()->GetEntries(); i++)
    {
      std::string filename=((TFile*)gROOT->GetListOfFiles()->At(i))->GetName();
      if(filename.find(_filenameSearchPattern)!=std::string::npos) strncpy(_eventinfo.filename, filename.c_str(),199);
    }

    _eventinfo.run_number=event.id().run();
    _eventinfo.subrun_number=event.id().subRun();
    _eventinfo.event_number=event.id().event();

    _detSysOrigin = mu2e::GeomHandle<mu2e::DetectorSystem>()->getOrigin();

//generated cosmic ray muons
    art::Handle<GenParticleCollection> genParticles;
    if(event.getByLabel(_generatorModuleLabel, genParticles))
    {
      if(genParticles.product()!=NULL)
      {
        if(genParticles->size()>0)   //there shouldn't be more than 1 particle
        {
          const mu2e::GenParticle &particle=genParticles->at(0);
          _eventinfo.cosmic_pos[0]=particle.position().x()-_detSysOrigin.x();
          _eventinfo.cosmic_pos[1]=particle.position().y()-_detSysOrigin.y();
          _eventinfo.cosmic_pos[2]=particle.position().z()-_detSysOrigin.z();
          _eventinfo.cosmic_E=particle.momentum().e();

          double p=particle.momentum().vect().mag();
          double px=particle.momentum().px();
          double py=particle.momentum().py();
          double pz=particle.momentum().pz();
          double zenith=acos(py/p)*180.0/TMath::Pi();
          double azimuth=atan2(px,pz)*180.0/TMath::Pi();
          if(azimuth<0) azimuth+=360.0;

          _eventinfo.cosmic_p=p;
          _eventinfo.cosmic_ph=azimuth;
          _eventinfo.cosmic_th=zenith;
          _eventinfo.cosmic_costh=py/p;

          PDGCode::type pdgId = particle.pdgId();
          strncpy(_eventinfo.cosmic_particle, HepPID::particleName(pdgId).c_str(),99);
        }
      }
    }

//particles crossing the CRV modules
    std::string productInstanceName="";
    art::Handle<SimParticleCollection> simParticleCollection;
    if(event.getByLabel(_g4ModuleLabel,productInstanceName,simParticleCollection))
    {
      if(simParticleCollection.product()!=NULL)
      {
        cet::map_vector<mu2e::SimParticle>::const_iterator iter;
        for(iter=simParticleCollection->begin(); iter!=simParticleCollection->end(); iter++)
        {
          const mu2e::SimParticle& particle = iter->second;
          int pdgId=particle.pdgId();
          if(abs(pdgId)==11 || abs(pdgId)==13)
          {
            const cet::map_vector_key& particleKey = iter->first;
            findCrossings(event,particleKey); //FIXME: if a second particle crosses the same plane, it will be ignored
          }
        }
      }
    }

//get the physical volume information for later
    art::Handle<mu2e::PhysicalVolumeInfoMultiCollection> physicalVolumesMulti;
    bool hasPhysicalVolumes=event.getSubRun().getByLabel(_volumeModuleLabel, physicalVolumesMulti);

//find the straw hits used by the reconstructed track
    std::map<art::Ptr<mu2e::SimParticle>, int> simParticles;   // < operator defined above

    art::Handle<KalRepCollection> kalReps;
    art::Handle<mu2e::StrawDigiMCCollection> strawDigiMCs;
    if(event.getByLabel(_fitterModuleLabel,_fitterModuleInstance,kalReps) && event.getByLabel(_hitmakerModuleLabel,_hitmakerModuleInstance,strawDigiMCs))
    {

      if(kalReps.product()!=NULL && strawDigiMCs.product()!=NULL)
      {
        if(kalReps->size()>0)
        {
          const KalRep &particle = kalReps->at(0);   //assume that there is only one reconstructed track
          _eventinfo.reco_t0=particle.t0().t0();
          TrkHitVector const& hots = particle.hitVector();
          for(auto iter=hots.begin(); iter!=hots.end(); iter++)
          {
            const TrkHit *hitOnTrack = *iter;
            const mu2e::TrkStrawHit* trkStrawHit = dynamic_cast<const mu2e::TrkStrawHit*>(hitOnTrack);
            if(trkStrawHit)
            {
              int strawHitVectorIndex = trkStrawHit->index(); //index of this straw hit in the strawHitCollection, which is equal to the index in the StrawDiguMCCollection 
                                                              //FIXME Is this true????
              //get the sim particle which caused this hit
              const mu2e::StrawDigiMC &strawDigiMC = (*strawDigiMCs)[strawHitVectorIndex];
              if(strawDigiMC.hasTDC(StrawDigi::TDCChannel::zero)) simParticles[strawDigiMC.stepPointMC(StrawDigi::TDCChannel::zero)->simParticle()]++;
              if(strawDigiMC.hasTDC(StrawDigi::TDCChannel::one)) simParticles[strawDigiMC.stepPointMC(StrawDigi::TDCChannel::one)->simParticle()]++;
            }
          }

          //find simparticle which matches the highest number of straw hits associated with the reconstructed track
          int largestNumberOfHits=0;
          art::Ptr<mu2e::SimParticle> matchingSimparticle;
          std::map<art::Ptr<mu2e::SimParticle>, int>::const_iterator simParticleIter;
          for(simParticleIter=simParticles.begin(); simParticleIter!=simParticles.end(); simParticleIter++)
          {
            if(simParticleIter->second>largestNumberOfHits)
            {
              largestNumberOfHits=simParticleIter->second;
              matchingSimparticle=simParticleIter->first;
            }
          }

          //access the simparticle found above
          if(largestNumberOfHits>0)
          {
            _eventinfo.simreco_endp=matchingSimparticle->endMomentum().vect().mag();

            //this sim particle may have been create in the previous simulation stage
            //therefore, a particle which has the creation code "primary", may not be a primary
            //but a continuation of a particle from a previous stage.
            //this particle from the previous stage is this particle's parent
            if(matchingSimparticle->creationCode()==ProcessCode::mu2ePrimary &&
               matchingSimparticle->hasParent()) matchingSimparticle=matchingSimparticle->parent();

            ProcessCode productionProcess = matchingSimparticle->creationCode();
            strncpy(_eventinfo.simreco_production_process, productionProcess.name().c_str(),99);

            int pdgId=matchingSimparticle->pdgId();
            strncpy(_eventinfo.simreco_particle, HepPID::particleName(pdgId).c_str(),99);

            _eventinfo.simreco_startp=matchingSimparticle->startMomentum().vect().mag();
            _eventinfo.simreco_startp_z=matchingSimparticle->startMomentum().vect().z();
            _eventinfo.simreco_pos[0]=matchingSimparticle->startPosition().x()-_detSysOrigin.x();
            _eventinfo.simreco_pos[1]=matchingSimparticle->startPosition().y()-_detSysOrigin.y();
            _eventinfo.simreco_pos[2]=matchingSimparticle->startPosition().z()-_detSysOrigin.z();

            std::string productionVolumeName="unknown volume";
            if(hasPhysicalVolumes && physicalVolumesMulti.product()!=NULL)
            {
              mu2e::PhysicalVolumeMultiHelper volumeMultiHelper(*physicalVolumesMulti);
              productionVolumeName=volumeMultiHelper.startVolume(*matchingSimparticle).name();
            }
            strncpy(_eventinfo.simreco_production_volume, productionVolumeName.c_str(),99);
          }
        } //kalReps.size()>0
      } //kalReps.product()!=0
    } //event.getByLabel

//check CRV veto
    art::Handle<CrvCoincidenceCheckResult> crvCoincidenceCheckResult;
    std::string crvCoincidenceInstanceName="";
    
    if(event.getByLabel(_crvCoincidenceModuleLabel,crvCoincidenceInstanceName,crvCoincidenceCheckResult))
    {
      std::vector<CrvCoincidenceCheckResult::DeadTimeWindow> deadTimeWindows;
      deadTimeWindows = crvCoincidenceCheckResult->GetDeadTimeWindows(0.0,125.0);
      for(unsigned int i=0; i < deadTimeWindows.size(); i++)
      {
        double t = deadTimeWindows[i]._startTime;
        if(isnan(_eventinfo.firstCoincidenceHitTime) || t<_eventinfo.firstCoincidenceHitTime) _eventinfo.firstCoincidenceHitTime=t;
      }
    }

//check for CRV step points
    GeomHandle<CosmicRayShield> CRS;

    std::vector<art::Handle<StepPointMCCollection> > CRVStepsVector;
    art::Selector selector(art::ProductInstanceNameSelector("CRV") && 
                           art::ProcessNameSelector("*"));

    event.getMany(selector, CRVStepsVector);
    for(size_t i=0; i<CRVStepsVector.size(); i++)
    {
      const art::Handle<StepPointMCCollection> &CRVSteps = CRVStepsVector[i];

      for(StepPointMCCollection::const_iterator iter=CRVSteps->begin(); iter!=CRVSteps->end(); iter++)
      {
        StepPointMC const& step(*iter);
        int PDGcode = step.simParticle()->pdgId();
        if(abs(PDGcode)!=11 && abs(PDGcode)!=13) continue;

        const CRSScintillatorBar &CRSbar = CRS->getBar(step.barIndex());

        int sectorNumber = CRSbar.id().getShieldNumber();
        int sectorType = CRS->getCRSScintillatorShield(sectorNumber).getSectorType();
        sectorType--;
        if(sectorType>=8 || sectorType<0) continue;
        // 0: R
        // 1: L
        // 2: T
        // 3: D
        // 4: U
        // 5,6,7: C1,C2,C3

        double t = step.time();
        double z = step.position().z()-_detSysOrigin.z();

        _eventinfo.CRVhit_allSectors =true;
        _eventinfo.CRVhit[sectorType]=true;
        if(_eventinfo.CRVhitTime[sectorType]>t || isnan(_eventinfo.CRVhitTime[sectorType])) _eventinfo.CRVhitTime[sectorType]=t;
        if(_eventinfo.CRVhitZ[sectorType]>t || isnan(_eventinfo.CRVhitZ[sectorType])) _eventinfo.CRVhitZ[sectorType]=z;
      }
    }

    _tree->Fill();
  }
}

using mu2e::CosmicAnalysis;
DEFINE_ART_MODULE(CosmicAnalysis);
