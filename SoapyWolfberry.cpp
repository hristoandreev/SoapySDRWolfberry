#include <SoapySDR/Device.hpp>
#include <SoapySDR/Registry.hpp>

#include "SoapyWolfberry.hpp"

/***********************************************************************
 * Find available devices
 **********************************************************************/

SoapySDR::KwargsList findWolfberry(const SoapySDR::Kwargs &args) {
	(void) args;
	
	SoapySDR::Kwargs options;
	
	static std::vector<SoapySDR::Kwargs> results;
	
	options["driver"] = "wolfberry";
	options["label"] = "Wolfberry Transceiver";
	results.push_back(options);

	return results;
}

/***********************************************************************
 * Make device instance
 **********************************************************************/
SoapySDR::Device *makeWolfberry(const SoapySDR::Kwargs &args) {
    return new SoapyWolfberry(args);
}

/***********************************************************************
 * Registration
 **********************************************************************/
static SoapySDR::Registry registerWolfberry("wolfberry", &findWolfberry, &makeWolfberry, SOAPY_SDR_ABI_VERSION);
