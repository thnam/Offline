// fit waveform using root TF1
#include "TrkChargeReco/inc/PeakFitRoot.hh"
#include "TF1.h"
#include "TGraphErrors.h"
#include "TFitResult.h"
#include <iostream>

namespace mu2e {
  namespace TrkChargeReco {

  PeakFitRoot::PeakFitRoot(StrawElectronics const& strawele, FitConfig const& config, FitType const& fittype,
    std::string fitoptions) : PeakFit(strawele,fittype),
  _peakfit(strawele,config) , _config(config), _fitoptions(fitoptions){}

  void PeakFitRoot::process(TrkTypes::ADCWaveform const& adcData, PeakFitParams & fit) const {
    // find initial values for the fit
    PeakFit::process(adcData,fit);
    if(_config._debug>0)std::cout << "PeakFitRoot Initialization charge = " << fit._charge << std::endl;
    // set the initial values based on this 'fit'
    Double_t parray[PeakFitParams::nParams];
    fit.fillArray(parray);
    _peakfit.fitModelTF1()->SetParameters(parray);
    // temporarily set width negative to force an unconvolved fit FIXME!!
    _peakfit.fitModelTF1()->SetParameter(PeakFitParams::width,-1.0);
    // cobnvert waveform to a TGraph
    TGraphErrors fitData;
    adcWaveform2TGraphErrors(adcData,fitData);
    // debug
    if(_config._debug>1){
      std::cout << "data = ";
      for (size_t i = 0; i < adcData.size(); ++i)
	std::cout << fitData.GetY()[i] << ",  ";
      std::cout << std::endl;
      std::cout << "func = ";  
      for (size_t i = 0; i < adcData.size(); ++i)
	std::cout << _peakfit.fitModelTF1()->Eval(fitData.GetX()[i]) << ",  ";
      std::cout << std::endl;
    }
    // invoke the fit
    TFitResultPtr fitresult = fitData.Fit(_peakfit.fitModelTF1(),_fitoptions.c_str()); 
    // if the fit is a failure, try again, up to the maximum # of iterations
    unsigned ifit=1;
    if(fitresult->Status() ==4 && ifit < _config._maxnit){
      ++ifit;
      fitData.Fit(_peakfit.fitModelTF1(),_fitoptions.c_str());
    }
    // copy fit result back to the the PeakFitParams
    fit = PeakFitParams(_peakfit.fitModelTF1()->GetParameters(),
      fitresult->Chi2(),
      fitresult->Ndf(),
      fitresult->Status());
    // set fix/free
    for(int ipar=0;ipar < _peakfit.fitModelTF1()->GetNpar();++ipar){
      Double_t parmin, parmax;
      _peakfit.fitModelTF1()->GetParLimits(ipar,parmin,parmax);
      if(parmin == parmax)
	fit.fixParam((TrkChargeReco::PeakFitParams::paramIndex)ipar);
      else
	fit.freeParam((TrkChargeReco::PeakFitParams::paramIndex)ipar);
    }
  }

  void PeakFitRoot::adcWaveform2TGraphErrors(TrkTypes::ADCWaveform const& adcData, TGraphErrors &fitData) const{
      Double_t adcDataTemp[adcData.size()];
      Double_t measurementTimes[adcData.size()];
      Double_t measurementTimesErrors[adcData.size()];
      Double_t adcDataErrors[adcData.size()];

      for (size_t i = 0; i < adcData.size(); ++i)
      {
	adcDataTemp[i] = (Double_t) adcData[i];
	measurementTimes[i] = (Double_t) i * _strawele.adcPeriod(); // should deal with global time offset FIXME!!
	measurementTimesErrors[i] = 0.0; //_strawele.adcPeriod();
	adcDataErrors[i] = 1.0*_strawele.analogNoise(TrkTypes::adc)/_strawele.adcLSB(); // should be able to scale the error FIXME!!
      }
      fitData = TGraphErrors(adcData.size(),measurementTimes,adcDataTemp,measurementTimesErrors,adcDataErrors);
    }

  } // TrkChargeReco namespace

}// mu2e namespace


