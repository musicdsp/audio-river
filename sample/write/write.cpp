/** @file
 * @author Edouard DUPIN 
 * @copyright 2015, Edouard DUPIN, all right reserved
 * @license MPL v2.0 (see license file)
 */

//! [audio_river_sample_write_all]

#include <audio/river/river.hpp>
#include <audio/river/Manager.hpp>
#include <audio/river/Interface.hpp>
#include <etk/etk.hpp>
#include <ethread/Thread.hpp>
#include <ethread/tools.hpp>
#include <test-debug/debug.hpp>


//! [audio_river_sample_write_config_file]
static const etk::String configurationRiver =
		"{\n"
		"	speaker:{\n"
		"		io:'output',\n"
		"		map-on:{\n"
		"			interface:'alsa',\n"
		"			name:'hw:0,3',\n"
		"		},\n"
		"		frequency:48000,\n"
		//"		channel-map:['front-left', 'front-right', 'rear-left', 'rear-right'],\n"
		"		channel-map:['front-left', 'front-right'],\n"
		"		type:'int16',\n"
		"		nb-chunk:1024,\n"
		"		volume-name:'MASTER'\n"
		"	}\n"
		"}\n";
//! [audio_river_sample_write_config_file]

static const int32_t nbChannelMax=8;

//! [audio_river_sample_callback_implement]
void onDataNeeded(void* _data,
                  const audio::Time& _time,
                  size_t _nbChunk,
                  enum audio::format _format,
                  uint32_t _sampleRate,
                  const etk::Vector<audio::channel>& _map) {
	static double phase[8] = {0,0,0,0,0,0,0,0};
	
	if (_format != audio::format_int16) {
		TEST_ERROR("Call wrong type ... (need int16_t)");
	}
	//TEST_VERBOSE("Map " << _map);
	int16_t* data = static_cast<int16_t*>(_data);
	double baseCycle = 2.0*M_PI/double(48000) * double(440);
	for (int32_t iii=0; iii<_nbChunk; iii++) {
		for (int32_t jjj=0; jjj<_map.size(); jjj++) {
			data[_map.size()*iii+jjj] = cos(phase[jjj]) * 30000;
			phase[jjj] += baseCycle*jjj;
			if (phase[jjj] >= 2*M_PI) {
				phase[jjj] -= 2*M_PI;
			}
		}
	}
}
//! [audio_river_sample_callback_implement]

int main(int _argc, const char **_argv) {
	// the only one init for etk:
	etk::init(_argc, _argv);
	for (int32_t iii=0; iii<_argc ; ++iii) {
		etk::String data = _argv[iii];
		if (    data == "-h"
		     || data == "--help") {
			TEST_PRINT("Help:");
			TEST_PRINT("    ./xxx ---");
			exit(0);
		}
	}
	// initialize river interface
	audio::river::initString(configurationRiver);
	// Create the River manager for tha application or part of the application.
	ememory::SharedPtr<audio::river::Manager> manager = audio::river::Manager::create("river_sample_read");
	//! [audio_river_sample_create_write_interface]
	// create interface:
	ememory::SharedPtr<audio::river::Interface> interface;
	//Get the generic input:
	interface = manager->createOutput(48000,
	                                  etk::Vector<audio::channel>(),
	                                  audio::format_int16,
	                                  "speaker");
	if(interface == null) {
		TEST_ERROR("null interface");
		return -1;
	}
	//! [audio_river_sample_create_write_interface]
	//! [audio_river_sample_set_callback]
	// set callback mode ...
	interface->setOutputCallback([=](void* _data,
	                                 const audio::Time& _time,
	                                 size_t _nbChunk,
	                                 enum audio::format _format,
	                                 uint32_t _frequency,
	                                 const etk::Vector<audio::channel>& _map) {
	                                 	onDataNeeded(_data, _time, _nbChunk, _format, _frequency, _map);
	                                 });
	//! [audio_river_sample_set_callback]
	// start the stream
	interface->start();
	// wait 10 second ...
	ethread::sleepMilliSeconds(1000*(10));
	// stop the stream
	interface->stop();
	// remove interface and manager.
	interface.reset();
	manager.reset();
	return 0;
}

//! [audio_river_sample_write_all]

