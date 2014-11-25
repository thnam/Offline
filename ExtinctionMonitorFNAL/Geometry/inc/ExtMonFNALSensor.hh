//  A silicon sensor.   May have several readout chips attached.
//
// Andrei Gaponenko, 2012

#ifndef EXTMONFNALSENSOR_HH
#define EXTMONFNALSENSOR_HH

#include <vector>

#include "art/Persistency/Common/Wrapper.h"

#include "ExtinctionMonitorFNAL/Geometry/inc/ExtMonFNALPixelChip.hh"
#include "DataProducts/inc/ExtMonFNALPixelId.hh"
#include "CLHEP/Vector/TwoVector.h"

namespace mu2e {

  namespace ExtMonFNAL { class ExtMon; }
  namespace ExtMonFNAL { class ExtMonMaker; }

  class ExtMonFNALSensor {
  public:

    std::vector<double> halfSize() const { return halfSize_; }

    // Returns default-constructed ExtMonFNALPixelId for out of range (x,y) inputs.
    ExtMonFNALPixelId findPixel(ExtMonFNALSensorId sid, double xSensor, double ySensor) const;

    // No protection for invalid pixel id
    CLHEP::Hep2Vector sensorCoordinates(const ExtMonFNALPixelId& pix) const;

    const ExtMonFNALPixelChip& chip() const { return chip_; }
    unsigned int nxChips() const;
    unsigned int nyChips() const;

    ExtMonFNALSensor(const ExtMonFNALPixelChip& chip, const std::vector<double>& hs)
      : chip_(chip), halfSize_(hs)
    {}

    //----------------------------------------------------------------
  private:
    ExtMonFNALPixelChip chip_;
    std::vector<double> halfSize_;

    // Required by genreflex persistency
    ExtMonFNALSensor() {}
    template<class T> friend class art::Wrapper;

    friend class ExtMonFNAL::ExtMon;
    friend class ExtMonFNAL::ExtMonMaker;
  };

  //================================================================

} // namespace mu2e

#endif/*EXTMONFNALSENSOR_HH*/