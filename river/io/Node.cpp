/** @file
 * @author Edouard DUPIN 
 * @copyright 2015, Edouard DUPIN, all right reserved
 * @license APACHE v2.0 (see license file)
 */

#include "Node.h"
#include <river/debug.h>

#include <memory>

#undef __class__
#define __class__ "io::Node"

#ifndef INT16_MAX
	#define INT16_MAX 0x7fff
#endif
#ifndef INT16_MIN
	#define INT16_MIN (-INT16_MAX - 1)
#endif
#ifndef INT32_MAX
	#define INT32_MAX 0x7fffffffL
#endif
#ifndef INT32_MIN
	#define INT32_MIN (-INT32_MAX - 1L)
#endif

int32_t river::io::Node::rtAudioCallback(void* _outputBuffer,
                                          void* _inputBuffer,
                                          unsigned int _nBufferFrames,
                                          double _streamTime,
                                          airtaudio::status _status) {
	std::unique_lock<std::mutex> lock(m_mutex);
	std::chrono::system_clock::time_point ttime = std::chrono::system_clock::time_point();//std::chrono::system_clock::now();
	
	if (_outputBuffer != nullptr) {
		RIVER_VERBOSE("data Output");
		std::vector<int32_t> output;
		output.resize(_nBufferFrames*m_interfaceFormat.getMap().size(), 0);
		const int32_t* outputTmp = nullptr;
		std::vector<uint8_t> outputTmp2;
		outputTmp2.resize(sizeof(int32_t)*m_interfaceFormat.getMap().size()*_nBufferFrames, 0);
		for (auto &it : m_list) {
			if (it != nullptr) {
				// clear datas ...
				memset(&outputTmp2[0], 0, sizeof(int32_t)*m_interfaceFormat.getMap().size()*_nBufferFrames);
				RIVER_VERBOSE("    IO : " /* << std::distance(m_list.begin(), it)*/ << "/" << m_list.size() << " name="<< it->getName());
				it->systemNeedOutputData(ttime, &outputTmp2[0], _nBufferFrames, sizeof(int32_t)*m_interfaceFormat.getMap().size());
				outputTmp = reinterpret_cast<const int32_t*>(&outputTmp2[0]);
				//it->systemNeedOutputData(ttime, _outputBuffer, _nBufferFrames, sizeof(int16_t)*m_map.size());
				// Add data to the output tmp buffer :
				for (size_t kkk=0; kkk<output.size(); ++kkk) {
					output[kkk] += outputTmp[kkk];
				}
				break;
			}
		}
		int16_t* outputBuffer = static_cast<int16_t*>(_outputBuffer);
		for (size_t kkk=0; kkk<output.size(); ++kkk) {
			*outputBuffer++ = static_cast<int16_t>(std::min(std::max(INT16_MIN, output[kkk]), INT16_MAX));
		}
	}
	if (_inputBuffer != nullptr) {
		RIVER_INFO("data Input");
		int16_t* inputBuffer = static_cast<int16_t *>(_inputBuffer);
		for (size_t iii=0; iii< m_list.size(); ++iii) {
			if (m_list[iii] != nullptr) {
				RIVER_INFO("    IO : " << iii+1 << "/" << m_list.size() << " name="<< m_list[iii]->getName());
				m_list[iii]->systemNewInputData(ttime, inputBuffer, _nBufferFrames);
			}
		}
	}
	return 0;
}


std::shared_ptr<river::io::Node> river::io::Node::create(const std::string& _name, const std::shared_ptr<const ejson::Object>& _config) {
	return std::shared_ptr<river::io::Node>(new river::io::Node(_name, _config));
}

river::io::Node::Node(const std::string& _name, const std::shared_ptr<const ejson::Object>& _config) :
  m_config(_config),
  m_name(_name),
  m_isInput(false) {
	RIVER_INFO("-----------------------------------------------------------------");
	RIVER_INFO("--                       CREATE NODE                           --");
	RIVER_INFO("-----------------------------------------------------------------");
	/**
		io:"input", # input or output
		map-on:{ # select hardware interface and name
			interface:"alsa", # interface : "alsa", "pulse", "core", ...
			name:"default", # name of the interface
		},
		frequency:48000, # frequency to open device
		channel-map:[ # mapping of the harware device (to change map if needed)
			"front-left", "front-right",
			"read-left", "rear-right",
		],
		type:"int16", # format to open device (int8, int16, int16-on-ont32, int24, int32, float)
		nb-chunk:1024 # number of chunk to open device (create the latency anf the frequency to call user)
	*/
	m_isInput = m_config->getStringValue("io") == "input";
	enum airtaudio::type typeInterface = airtaudio::type_undefined;
	std::string streamName = "default";
	const std::shared_ptr<const ejson::Object> tmpObject = m_config->getObject("map-on");
	if (tmpObject == nullptr) {
		RIVER_WARNING("missing node : 'map-on' ==> auto map : 'alsa:default'");
	} else {
		std::string value = tmpObject->getStringValue("interface", "default");
		typeInterface = airtaudio::getTypeFromString(value);
		streamName = tmpObject->getStringValue("name", "default");
	}
	int32_t frequency = m_config->getNumberValue("frequency", 48000);
	std::string type = m_config->getStringValue("type", "int16");
	int32_t nbChunk = m_config->getNumberValue("nb-chunk", 1024);
	std::string volumeName = m_config->getStringValue("volume-name", "");
	if (volumeName != "") {
		RIVER_INFO("add node volume stage : '" << volumeName << "'");
		// use global manager for volume ...
		m_volume = river::io::Manager::getInstance()->getVolumeGroup(volumeName);
	}
	
	enum audio::format formatType = audio::format_int16;
	if (type == "int16") {
		formatType = audio::format_int16;
	} else {
		RIVER_WARNING("not managed type : '" << type << "'");
	}
	// TODO : MAP ...
	
	// intanciate specific API ...
	m_adac.instanciate(typeInterface);
	// TODO : Check return ...
	
	if (streamName == "") {
		streamName = "default";
	}
	std::vector<audio::channel> map;
	// set default channel property :
	map.push_back(audio::channel_frontLeft);
	map.push_back(audio::channel_frontRight);
	
	m_hardwareFormat.set(map, formatType, frequency);
	// TODO : Better view of interface type float -> float, int16 -> int16/int32,  ...
	if (m_isInput == true) {
		// for input we just transfert audio with no transformation
		m_interfaceFormat.set(map, audio::format_int16, frequency);
	} else {
		// for output we will do a mix ...
		m_interfaceFormat.set(map, audio::format_int16_on_int32, frequency);
	}

	// search device ID :
	RIVER_INFO("Open :");
	RIVER_INFO("    m_streamName=" << streamName);
	RIVER_INFO("    m_freq=" << m_hardwareFormat.getFrequency());
	RIVER_INFO("    m_map=" << m_hardwareFormat.getMap());
	RIVER_INFO("    m_format=" << m_hardwareFormat.getFormat());
	RIVER_INFO("    m_isInput=" << m_isInput);
	int32_t deviceId = 0;
	RIVER_INFO("Device list:");
	for (int32_t iii=0; iii<m_adac.getDeviceCount(); ++iii) {
		m_info = m_adac.getDeviceInfo(iii);
		RIVER_INFO("    " << iii << " name :" << m_info.name);
		if (m_info.name == streamName) {
			RIVER_INFO("        Select ...");
			deviceId = iii;
		}
	}
	// Open specific ID :
	m_info = m_adac.getDeviceInfo(deviceId);
	// display property :
	{
		RIVER_INFO("Device " << deviceId << " property :");
		RIVER_INFO("    probe=" << m_info.probed);
		RIVER_INFO("    name=" << m_info.name);
		RIVER_INFO("    outputChannels=" << m_info.outputChannels);
		RIVER_INFO("    inputChannels=" << m_info.inputChannels);
		RIVER_INFO("    duplexChannels=" << m_info.duplexChannels);
		RIVER_INFO("    isDefaultOutput=" << m_info.isDefaultOutput);
		RIVER_INFO("    isDefaultInput=" << m_info.isDefaultInput);
		//std::string rrate;
		std::stringstream rrate;
		for (int32_t jjj=0; jjj<m_info.sampleRates.size(); ++jjj) {
			rrate << m_info.sampleRates[jjj] << ";";
		}
		RIVER_INFO("    rates=" << rrate.str());
		RIVER_INFO("    native Format: " << m_info.nativeFormats);
	}
	
	// open Audio device:
	airtaudio::StreamParameters params;
	params.deviceId = deviceId;
	if (m_isInput == true) {
		m_info.inputChannels = 2;
		params.nChannels = 2;
	} else {
		m_info.outputChannels = 2;
		params.nChannels = 2;
	}
	
	m_rtaudioFrameSize = nbChunk;
	RIVER_INFO("Open output stream nbChannels=" << params.nChannels);
	enum airtaudio::error err = airtaudio::error_none;
	if (m_isInput == true) {
		err = m_adac.openStream(nullptr, &params,
		                        audio::format_int16, m_hardwareFormat.getFrequency(), &m_rtaudioFrameSize,
		                        std::bind(&river::io::Node::rtAudioCallback,
		                                  this,
		                                  std::placeholders::_1,
		                                  std::placeholders::_2,
		                                  std::placeholders::_3,
		                                  std::placeholders::_4,
		                                  std::placeholders::_5)
		                        );
	} else {
		err = m_adac.openStream(&params, nullptr,
		                        audio::format_int16, m_hardwareFormat.getFrequency(), &m_rtaudioFrameSize,
		                        std::bind(&river::io::Node::rtAudioCallback,
		                                  this,
		                                  std::placeholders::_1,
		                                  std::placeholders::_2,
		                                  std::placeholders::_3,
		                                  std::placeholders::_4,
		                                  std::placeholders::_5)
		                        );
	}
	if (err != airtaudio::error_none) {
		RIVER_ERROR("Create stream : '" << m_name << "' mode=" << (m_isInput?"input":"output") << " can not create stream " << err);
	}
}

river::io::Node::~Node() {
	std::unique_lock<std::mutex> lock(m_mutex);
	RIVER_INFO("-----------------------------------------------------------------");
	RIVER_INFO("--                      DESTROY NODE                           --");
	RIVER_INFO("-----------------------------------------------------------------");
	RIVER_INFO("close input stream");
	if (m_adac.isStreamOpen() ) {
		m_adac.closeStream();
	}
};

void river::io::Node::start() {
	std::unique_lock<std::mutex> lock(m_mutex);
	RIVER_INFO("Start stream : '" << m_name << "' mode=" << (m_isInput?"input":"output") );
	enum airtaudio::error err = m_adac.startStream();
	if (err != airtaudio::error_none) {
		RIVER_ERROR("Start stream : '" << m_name << "' mode=" << (m_isInput?"input":"output") << " can not start stream ... " << err);
	}
}

void river::io::Node::stop() {
	std::unique_lock<std::mutex> lock(m_mutex);
	RIVER_INFO("Stop stream : '" << m_name << "' mode=" << (m_isInput?"input":"output") );
	enum airtaudio::error err = m_adac.stopStream();
	if (err != airtaudio::error_none) {
		RIVER_ERROR("Stop stream : '" << m_name << "' mode=" << (m_isInput?"input":"output") << " can not stop stream ... " << err);
	}
}

void river::io::Node::registerAsRemote(const std::shared_ptr<river::Interface>& _interface) {
	auto it = m_listAvaillable.begin();
	while (it != m_listAvaillable.end()) {
		if (it->expired() == true) {
			it = m_listAvaillable.erase(it);
		}
		++it;
	}
	m_listAvaillable.push_back(_interface);
}

void river::io::Node::interfaceAdd(const std::shared_ptr<river::Interface>& _interface) {
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		for (size_t iii=0; iii< m_list.size(); ++iii) {
			if (_interface == m_list[iii]) {
				return;
			}
		}
		RIVER_INFO("ADD interface for stream : '" << m_name << "' mode=" << (m_isInput?"input":"output") );
		m_list.push_back(_interface);
	}
	if (m_list.size() == 1) {
		start();
	}
}

void river::io::Node::interfaceRemove(const std::shared_ptr<river::Interface>& _interface) {
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		for (size_t iii=0; iii< m_list.size(); ++iii) {
			if (_interface == m_list[iii]) {
				m_list.erase(m_list.begin()+iii);
				RIVER_INFO("RM interface for stream : '" << m_name << "' mode=" << (m_isInput?"input":"output") );
				break;
			}
		}
	}
	if (m_list.size() == 0) {
		stop();
	}
	return;
}


void river::io::Node::volumeChange() {
	for (auto &it : m_listAvaillable) {
		std::shared_ptr<river::Interface> node = it.lock();
		if (node != nullptr) {
			node->systemVolumeChange();
		}
	}
}