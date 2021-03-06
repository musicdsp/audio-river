/** @file
 * @author Edouard DUPIN 
 * @copyright 2015, Edouard DUPIN, all right reserved
 * @license MPL v2.0 (see license file)
 */

#include <test-debug/debug.hpp>
#include <audio/river/river.hpp>
#include <audio/river/Manager.hpp>
#include <audio/river/Interface.hpp>
#include <etest/etest.hpp>
#include <etk/etk.hpp>
extern "C" {
	#include <math.h>
}

#include <ethread/Thread.hpp>
#include <ethread/tools.hpp>

namespace river_test_playback_write {
	static const etk::String configurationRiver =
		"{\n"
		"	speaker:{\n"
		"		io:'output',\n"
		"		map-on:{\n"
		"			interface:'auto',\n"
		"			name:'default',\n"
		"		},\n"
		"		frequency:0,\n"
		"		channel-map:['front-left', 'front-right'],\n"
		"		type:'auto',\n"
		"		nb-chunk:1024,\n"
		"		volume-name:'MASTER'\n"
		"	}\n"
		"}\n";
	
	class testOutWrite {
		public:
			etk::Vector<audio::channel> m_channelMap;
			ememory::SharedPtr<audio::river::Manager> m_manager;
			ememory::SharedPtr<audio::river::Interface> m_interface;
		public:
			testOutWrite(ememory::SharedPtr<audio::river::Manager> _manager) :
			  m_manager(_manager) {
				//Set stereo output:
				m_channelMap.pushBack(audio::channel_frontLeft);
				m_channelMap.pushBack(audio::channel_frontRight);
				m_interface = m_manager->createOutput(48000,
				                                      m_channelMap,
				                                      audio::format_int16,
				                                      "speaker");
				if(m_interface == null) {
					TEST_ERROR("null interface");
					return;
				}
				m_interface->setReadwrite();
			}
			void run() {
				if(m_interface == null) {
					TEST_ERROR("null interface");
					return;
				}
				double phase=0;
				etk::Vector<int16_t> data;
				data.resize(1024*m_channelMap.size());
				double baseCycle = 2.0*M_PI/48000.0 * 440.0;
				// start fill buffer
				for (int32_t kkk=0; kkk<10; ++kkk) {
					for (int32_t iii=0; iii<data.size()/m_channelMap.size(); iii++) {
						for (int32_t jjj=0; jjj<m_channelMap.size(); jjj++) {
							data[m_channelMap.size()*iii+jjj] = cos(phase) * 30000.0;
						}
						phase += baseCycle;
						if (phase >= 2*M_PI) {
							phase -= 2*M_PI;
						}
					}
					m_interface->write(&data[0], data.size()/m_channelMap.size());
				}
				m_interface->start();
				for (int32_t kkk=0; kkk<100; ++kkk) {
					for (int32_t iii=0; iii<data.size()/m_channelMap.size(); iii++) {
						for (int32_t jjj=0; jjj<m_channelMap.size(); jjj++) {
							data[m_channelMap.size()*iii+jjj] = cos(phase) * 30000.0;
						}
						phase += baseCycle;
						if (phase >= 2*M_PI) {
							phase -= 2*M_PI;
						}
					}
					m_interface->write(&data[0], data.size()/m_channelMap.size());
					// TODO : Add a function to get number of time we need to wait enought time ...
					ethread::sleepMilliSeconds((15));
				}
				m_interface->stop();
			}
	};
	
	TEST(TestALL, testOutputWrite) {
		audio::river::initString(configurationRiver);
		ememory::SharedPtr<audio::river::Manager> manager;
		manager = audio::river::Manager::create("testApplication");
		
		TEST_INFO("test output (write mode)");
		ememory::SharedPtr<testOutWrite> process = ememory::makeShared<testOutWrite>(manager);
		process->run();
		process.reset();
		ethread::sleepMilliSeconds((500));
		audio::river::unInit();
	}
	
	class testOutWriteCallback {
		public:
			ememory::SharedPtr<audio::river::Manager> m_manager;
			ememory::SharedPtr<audio::river::Interface> m_interface;
			double m_phase;
		public:
			testOutWriteCallback(ememory::SharedPtr<audio::river::Manager> _manager) :
			  m_manager(_manager),
			  m_phase(0) {
				etk::Vector<audio::channel> channelMap;
				//Set stereo output:
				channelMap.pushBack(audio::channel_frontLeft);
				channelMap.pushBack(audio::channel_frontRight);
				m_interface = m_manager->createOutput(48000,
				                                      channelMap,
				                                      audio::format_int16,
				                                      "speaker");
				if(m_interface == null) {
					TEST_ERROR("null interface");
					return;
				}
				m_interface->setReadwrite();
				m_interface->setWriteCallback([=](const audio::Time& _time,
				                                  size_t _nbChunk,
				                                  enum audio::format _format,
				                                  uint32_t _frequency,
				                                  const etk::Vector<audio::channel>& _map) {
				                                  	onDataNeeded(_time, _nbChunk, _format, _frequency, _map);
				                                  });
			}
			void onDataNeeded(const audio::Time& _time,
			                  size_t _nbChunk,
			                  enum audio::format _format,
			                  uint32_t _frequency,
			                  const etk::Vector<audio::channel>& _map) {
				if (_format != audio::format_int16) {
					TEST_ERROR("call wrong type ... (need int16_t)");
				}
				etk::Vector<int16_t> data;
				data.resize(1024*_map.size());
				double baseCycle = 2.0*M_PI/48000.0 * 440.0;
				// start fill buffer
				for (int32_t iii=0; iii<data.size()/_map.size(); iii++) {
					for (int32_t jjj=0; jjj<_map.size(); jjj++) {
						data[_map.size()*iii+jjj] = cos(m_phase) * 30000.0;
					}
					m_phase += baseCycle;
					if (m_phase >= 2*M_PI) {
						m_phase -= 2*M_PI;
					}
				}
				m_interface->write(&data[0], data.size()/_map.size());
			}
			void run() {
				if(m_interface == null) {
					TEST_ERROR("null interface");
					return;
				}
				m_interface->start();
				ethread::sleepMilliSeconds(1000*(1));
				m_interface->stop();
			}
	};
	
	TEST(TestALL, testOutputWriteWithCallback) {
		audio::river::initString(configurationRiver);
		ememory::SharedPtr<audio::river::Manager> manager;
		manager = audio::river::Manager::create("testApplication");
		
		TEST_INFO("test output (write with callback event mode)");
		ememory::SharedPtr<testOutWriteCallback> process = ememory::makeShared<testOutWriteCallback>(manager);
		process->run();
		process.reset();
		ethread::sleepMilliSeconds((500));
		audio::river::unInit();
	}

};

