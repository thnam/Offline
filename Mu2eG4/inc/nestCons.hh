#ifndef Mu2eG4_nestCons_hh
#define Mu2eG4_nestCons_hh
//
// Free function to create and place a new G4Cons inside a logical volume.
//
// $Id: nestCons.hh,v 1.6 2014/09/19 19:14:54 knoepfel Exp $
// $Author: knoepfel $
// $Date: 2014/09/19 19:14:54 $
//
// Original author Rob Kutschke
//

#include <string>
#include <vector>

#include "G4Helper/inc/VolumeInfo.hh"

class G4Material;
class G4LogicalVolume;
class G4VPhysicalVolume;
class G4CSGSolid;

// G4 includes
#include "G4ThreeVector.hh"
#include "G4RotationMatrix.hh"
#include "G4Colour.hh"


namespace mu2e {

  VolumeInfo nestCons ( std::string const& name,
                        double const params[7],
                        G4Material* material,
                        G4RotationMatrix const* rot,
                        G4ThreeVector const & offset,
                        G4LogicalVolume* parent,
                        int copyNo,
                        bool const isVisible,
                        G4Colour const color,
                        bool const forceSolid,
                        bool const forceAuxEdgeVisible,
                        bool const placePV,
                        bool const doSurfaceCheck
                        );



  // Alternate argument list, using a vector for the parameters.
  inline VolumeInfo nestCons ( std::string const& name,
                               std::vector<double>&  params,
                               G4Material* material,
                               G4RotationMatrix const* rot,
                               G4ThreeVector const & offset,
                               G4LogicalVolume* parent,
                               int copyNo,
                               bool const isVisible,
                               G4Colour const color,
                               bool const forceSolid,
                               bool const forceAuxEdgeVisible,
                               bool const placePV,
                               bool const doSurfaceCheck
                               ){
    return nestCons( name,
                     &params[0],
                     material,
                     rot,
                     offset,
                     parent,
                     copyNo,
                     isVisible,
                     color,
                     forceSolid,
                     forceAuxEdgeVisible,
                     placePV,
                     doSurfaceCheck
                     );
  }

  // Alternate argument list (and different behavior)
  // using VolumeInfo object for the parameters.
  VolumeInfo nestCons ( std::string const& name,
                        double const params[7],
                        G4Material* material,
                        G4RotationMatrix const* rot,
                        G4ThreeVector const & offset,
                        VolumeInfo const & parent,
                        int copyNo,
                        bool const isVisible,
                        G4Colour const color,
                        bool const forceSolid,
                        bool const forceAuxEdgeVisible,
                        bool const placePV,
                        bool const doSurfaceCheck
                        );

 
  // Alternate argument list (and different behavior)
  // using VolumeInfo object for the parameters.
  VolumeInfo nestCons ( std::string const& name,
                        double const params[7],
                        G4Material* material,
                        G4RotationMatrix const* rot,
                        G4ThreeVector const & offset,
                        VolumeInfo const & parent,
                        int copyNo,
                        G4Colour const color,
			std::string const& lookupToken
                        ); 

  inline VolumeInfo nestCons ( std::string const& name,
			       std::vector<double>& params,
			       G4Material* material,
			       G4RotationMatrix const* rot,
			       G4ThreeVector const & offset,
			       VolumeInfo const & parent,
			       int copyNo,
			       G4Colour const color,
			       std::string const& lookupToken
			       ) {
    return nestCons ( name,
		      &params[0],
		      material,
		      rot,
		      offset,
		      parent,
		      copyNo,
		      color,
		      lookupToken
		      ); 
  }

}

#endif /* Mu2eG4_nestCons_hh */