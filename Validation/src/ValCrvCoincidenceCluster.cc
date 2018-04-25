
#include "Validation/inc/ValCrvCoincidenceCluster.hh"


int mu2e::ValCrvCoincidenceCluster::declare(art::TFileDirectory tfs) {
  _hVer = tfs.make<TH1D>( "Ver", "Version Number", 101, -0.5, 100.0);
  _hN = tfs.make<TH1D>( "NClus", "N Clusters", 101, -0.5, 100.5);
  _hSec = tfs.make<TH1D>( "SecType", "Sector type",21, -10.5, 10.5);
  _hPE= tfs.make<TH1D>( "PE", "PE",100, 0.0, 2000.0);
  _ht = tfs.make<TH1D>( "t", "start time", 100, 0.0, 2000.0);
  _pos.declare(tfs,"pos","Coincidence pos ");

  return 0;
}

int mu2e::ValCrvCoincidenceCluster::fill(const mu2e::CrvCoincidenceClusterCollection & coll,
				art::Event const& event) {

  // increment this by 1 any time the defnitions of the histograms or the 
  // histogram contents change, and will not match previous versions
  _hVer->Fill(0.0);

  _hN->Fill(coll.size());
  for(auto cc : coll) {
    _hSec->Fill(cc.GetCrvSectorType());
    _hPE->Fill(cc.GetPEs());
    _ht->Fill(cc.GetStartTime());
    _pos.fill(cc.GetAvgCounterPos());
  }
  return 0;
}