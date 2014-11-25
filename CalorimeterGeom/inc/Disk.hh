#ifndef CalorimeterGeom_Disk_hh
#define CalorimeterGeom_Disk_hh
// $Id: Disk.hh,v 1.7 2014/08/01 20:57:44 echenard Exp $
// $Author: echenard $
// $Date: 2014/08/01 20:57:44 $
//
// Hold information about a disk in the calorimter.
//
// Original author B Echenard 
//

// C++ includes
#include <vector>
#include <memory>
#include "CLHEP/Vector/TwoVector.h"
#include "CLHEP/Vector/ThreeVector.h"

// Mu2e includes
#include "CalorimeterGeom/inc/CaloSection.hh"
#include "CalorimeterGeom/inc/CrystalMapper.hh"
#include "CalorimeterGeom/inc/Crystal.hh"



namespace mu2e {


    class Disk : public CaloSection {

       public:

	   Disk(int id, double rin, double rout, double cellSize, int crystalNedges, CLHEP::Hep3Vector crystalOffset); 
	   
	   double                          innerRadius()                                const {return _radiusIn;}
           double                          outerRadius()                                const {return _radiusOut;}
	   double                          estimateEmptySpace()                         const;
           
	   int                             idxFromPosition(double x, double y)          const;           
	   std::vector<int>                findLocalNeighbors(int crystalId, int level) const;            


       private:

           void                            fillCrystals();
           bool                            isInsideDisk(double x, double y) const;
	   double                          calcDistToSide(CLHEP::Hep2Vector& a, CLHEP::Hep2Vector& b) const;

	   double                          _radiusIn;
	   double                          _radiusOut;
	   double                          _cellSize;	 
       	   std::unique_ptr<CrystalMapper>  _crystalMap;

           std::vector<int>                _mapToCrystal;
	   std::vector<int>                _crystalToMap;

    };
}

#endif /* CalorimeterGeom_Disk_hh */