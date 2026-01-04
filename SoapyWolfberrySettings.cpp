#include "SoapyWolfberry.hpp"
#include <chrono>
#include <thread>
#include <string>

#define RADIOBERRY_BUFFER_SIZE	4096

/***********************************************************************
 * Device interface
 **********************************************************************/
 
SoapyWolfberry::SoapyWolfberry( const SoapySDR::Kwargs &args ) {
	(void) args;

	SoapySDR_setLogLevel(SOAPY_SDR_INFO);
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyWolfberry::SoapyWolfberry  constructor called");
	mox = false;
	no_channels = 1;
	fd_rb = open("/dev/wolfberry", O_RDWR);
	try
	{
		i2c_ptr = std::make_unique<rpihw::driver::i2c>(rpihw::driver::i2c("/dev/i2c-1"));
		i2c_available = true;
	}
	catch (std::string s)
	{
		printf("I2c not found %s", s.c_str());
		i2c_available = false;
	}
}

SoapyWolfberry::~SoapyWolfberry()
{
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyWolfberry::SoapyWolfberry  destructor called");
	for (auto con : streams)
		delete (con);
	if (fd_rb != 0) close(fd_rb);
}

void SoapyWolfberry::controlWolfberry(uint32_t command, uint32_t command_data) {

	std::unique_lock<std::mutex> soapy_lock(send_command);
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyWolfberry::controlWolfberry called");
	
	uint32_t CWX =0;
	uint32_t running = 1;
	
	rb_control.rb_command = 0x04 | (((CWX << 1) & 0x02) | (running & 0x01));
	rb_control.command = command;
	rb_control.command_data = command_data;

	fprintf(stderr, "RB-Command = %02X Command = %02X  command_data = %08X Mox %d\n", rb_control.rb_command, command >> 1, command_data, command & 0x01);
	
	if (ioctl(fd_rb, WOLFBERRY_IOC_COMMAND, &rb_control) < 0) {
		SoapySDR_log(SOAPY_SDR_INFO, "Could not sent command to Wolfberry device.");
	} else SoapySDR_log(SOAPY_SDR_INFO, "Command sent successful to Wolfberry device.");
}

std::string SoapyWolfberry::getDriverKey() const
{
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyWolfberry::getDriverKey called");
	
	return "wolfberry";
}

std::string SoapyWolfberry::getHardwareKey() const
{
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyWolfberry::getHardwareKey called");
	
	return "v2.0-beta5";
}

SoapySDR::Kwargs SoapyWolfberry::getHardwareInfo() const
{
	
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyWolfberry::getHardwareInfo called");
	
	SoapySDR::Kwargs info;
	int count = 0;
	rb_info_arg_t rb_info{};
	
	do
	{
		std::memset(&rb_info, 0, sizeof(rb_info));
		if (ioctl(fd_rb, WOLFBERRY_IOC_COMMAND, &rb_info) < 0) {
			rb_info.major = 0;
			rb_info.minor = 0;
		} else break;

		std::this_thread::sleep_for(std::chrono::seconds(1));
		count++;
	} while ((rb_info.major == 0 || rb_info.major > 128) && count < 20);

	unsigned int major, minor;
	major = rb_info.major;
	minor = rb_info.minor;
	
	char firmware_version[100];
	snprintf(firmware_version, 100, "%u.%u", 0, 1); //0.1
	info["firmwareVersion"] = firmware_version;

	char gateware_version[100];
	snprintf(gateware_version, 100, "%u.%u ", major, minor);
	info["gatewareVersion"] = gateware_version;
	
	char hardware_version[100];
	snprintf(hardware_version, 100, "%u.%u", 2, 4); //2.4 beta
	info["hardwareVersion"] = hardware_version;

	char protocol_version[100];
	snprintf(protocol_version, 100, "%u.%u ", 1, 58); //1.58 protocol 1
	info["protocolVersion"] = protocol_version;

	return info;
}

size_t SoapyWolfberry::getNumChannels( const int direction ) const {
	(void) direction;
	
	if (direction == SOAPY_SDR_RX) {
		SoapySDR_log(SOAPY_SDR_INFO, "SoapyWolfberry::getNumChannels RX called");
		return(2);
	}

	if (direction == SOAPY_SDR_TX) {
		SoapySDR_log(SOAPY_SDR_INFO, "SoapyWolfberry::getNumChannels TX called");
		return(1);
	}

	SoapySDR_log(SOAPY_SDR_INFO, "SoapyWolfberry::getNumChannels called");

	return(0);
}

bool SoapyWolfberry::getFullDuplex( const int direction, const size_t channel ) const {
	(void) direction;
	(void) channel;

	SoapySDR_log(SOAPY_SDR_INFO, "SoapyWolfberry::getFullDuplex called");
	
	return(true);
}

std::vector<double> SoapyWolfberry::listBandwidths( const int direction, const size_t channel ) const {
	(void) direction;
	(void) channel;

	// radioberry does nor support bandwidth
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyWolfberry::listBandwidths called");
		
	std::vector<double> options;
	return(options);
}

std::vector<double> SoapyWolfberry::listSampleRates( const int direction, const size_t channel ) const {
	(void) channel;
	
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyWolfberry::listSampleRates called");

	std::vector<double> options;
	
	if (direction == SOAPY_SDR_RX) {
		options.push_back(0.048e6);  
		options.push_back(0.096e6);
		options.push_back(0.192e6);
		options.push_back(0.384e6);
	}
	if (direction == SOAPY_SDR_TX) {
		options.push_back(0.048e6);  
	}
	return(options);
}

double SoapyWolfberry::getBandwidth( const int direction, const size_t channel ) const {
	(void) channel;
	
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyWolfberry::getBandwidth called");
	
    long long bandwidth = 48000.0;

	if(direction==SOAPY_SDR_RX){
      
	  //depends on settings.. TODO

	}

	else if(direction==SOAPY_SDR_TX){
       bandwidth = 48000.0;
	}

	return double(bandwidth);
}

SoapySDR::RangeList SoapyWolfberry::getFrequencyRange( const int direction, const size_t channel)  const {
	(void) channel;
	(void) direction;

	SoapySDR_log(SOAPY_SDR_INFO, "SoapyWolfberry::getFrequencyRange called");
	
	SoapySDR::RangeList rangeList;
	
	rangeList.push_back(SoapySDR::Range(10000.0, 30000000.0, 1.0));
	
	return rangeList;
}

std::vector<std::string> SoapyWolfberry::listAntennas( const int direction, const size_t channel ) const {
	(void) channel;
	
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyWolfberry::listAntennas called");
	
	std::vector<std::string> options;
	if(direction == SOAPY_SDR_RX) options.push_back( "ANTENNA RX" );
	if(direction == SOAPY_SDR_TX) options.push_back( "ANTENNA TX" );
	return(options);
}


/*******************************************************************
 * Gain API
 ******************************************************************/

std::vector<std::string> SoapyWolfberry::listGains( const int direction, const size_t channel ) const {
	(void) channel;
	(void) direction;

	SoapySDR_log(SOAPY_SDR_INFO, "SoapyWolfberry::listGains called");
	
	std::vector<std::string> options;
	//options.push_back("PGA"); in pihpsdr no additional gain settings.
	return(options);
}

SoapySDR::Range SoapyWolfberry::getGainRange( const int direction, const size_t channel) const {
	(void) channel;

	SoapySDR_log(SOAPY_SDR_INFO, "SoapyWolfberry::getGainRange called");
	
	if(direction==SOAPY_SDR_RX)
		// return(SoapySDR::Range(-12, 48));
		return {-12, 48};
	// return(SoapySDR::Range(0,15));
	return {0,15};
}

void SoapyWolfberry::setGain( const int direction, const size_t channel, const double value ) {
	(void) channel;
	
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyWolfberry::setGain called");
	
	uint32_t command = 0;
	uint32_t command_data = (0x40 | (((uint32_t)value)  & 0x3F));
	
	if (direction == SOAPY_SDR_RX)	 
	{
		if (mox)
			command = 0x15;
		else
			command = 0x14;
		command_data = (0x40 | (((uint32_t)value + 12) & 0x3F));
	}
	
	if(direction==SOAPY_SDR_TX) 
	{ // 0 -7 TX RF gain
		if (!mox)
			return;

		auto z = (uint32_t)value;
		if (value > 15) z = 15;
		if (value < 0.0) z = 0;
		z = z << 28;
		command = 0x13; 
		command_data = z;
	}
	
	controlWolfberry(command, command_data);
}

/*******************************************************************
 * Frequency API 
 ******************************************************************/
void SoapyWolfberry::setFrequency( const int direction, const size_t channel,  const double frequency, const SoapySDR::Kwargs &args ) {
	(void) args;
	(void) channel;
	
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyWolfberry::setFrequency called");
	
	uint32_t command = 0;

	if (direction == SOAPY_SDR_RX)
	{
		if (mox)
			command = 5;
		else
			command = 4;
	}

	if (direction == SOAPY_SDR_TX)
	{
		if (!mox)
			return;
		command = 3;
	}

	auto command_data = (uint32_t) frequency;
	
	controlWolfberry(command, command_data);
}

void SoapyWolfberry::writeI2C(const int addr, const std::string &data)
{
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyWolfberry::writeI2C called");

	if (!i2c_available)
		return;
	i2c_ptr->addr(addr);
	try
	{
		i2c_ptr->write((uint8_t *)data.c_str(), data.size());
	}
	catch (std::string s)
	{
		printf("%s", s.c_str());
	}
}

std::string SoapyWolfberry::readI2C(const int addr, const size_t numBytes)
{
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyWolfberry::readI2C called");

	std::string data;

	if (!i2c_available)
		return {""};
	i2c_ptr->addr(addr);
	data.reserve(numBytes);
	try
	{
		i2c_ptr->read((uint8_t *)data.c_str(), numBytes);
		data.resize(numBytes);
	}
	catch (std::string s)
	{
		printf("%s", s.c_str());
	}
	return data;
}
// end of source.

