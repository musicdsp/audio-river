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

namespace river_test_aec {
	
	class Linker {
		private:
			ememory::SharedPtr<audio::river::Manager> m_manager;
			ememory::SharedPtr<audio::river::Interface> m_interfaceOut;
			ememory::SharedPtr<audio::river::Interface> m_interfaceIn;
			audio::drain::CircularBuffer m_buffer;
		public:
			Linker(ememory::SharedPtr<audio::river::Manager> _manager, const etk::String& _input, const etk::String& _output) :
			  m_manager(_manager) {
				//Set stereo output:
				etk::Vector<audio::channel> channelMap;
				if (false) { //"speaker" == _output) {
					channelMap.pushBack(audio::channel_frontCenter);
				} else {
					channelMap.pushBack(audio::channel_frontLeft);
					channelMap.pushBack(audio::channel_frontRight);
				}
				m_buffer.setCapacity(echrono::milliseconds(2000), sizeof(int16_t)*channelMap.size(), 48000);
				
				m_interfaceOut = m_manager->createOutput(48000,
				                                         channelMap,
				                                         audio::format_int16,
				                                         _output);
				if(m_interfaceOut == null) {
					TEST_ERROR("null interface");
					return;
				}
				// set callback mode ...
				m_interfaceOut->setOutputCallback([=](void* _data,
				                                      const audio::Time& _time,
				                                      size_t _nbChunk,
				                                      enum audio::format _format,
				                                      uint32_t _frequency,
				                                      const etk::Vector<audio::channel>& _map) {
				                                      	onDataNeeded(_data, _time, _nbChunk, _format, _frequency, _map);
				                                      });
				m_interfaceOut->addVolumeGroup("FLOW");
				if ("speaker" == _output) {
					m_interfaceOut->setParameter("volume", "FLOW", "0dB");
				}
				
				m_interfaceIn = m_manager->createInput(48000,
				                                       channelMap,
				                                       audio::format_int16,
				                                       _input);
				if(m_interfaceIn == null) {
					TEST_ERROR("null interface");
					return;
				}
				// set callback mode ...
				m_interfaceIn->setInputCallback([=](const void* _data,
				                                    const audio::Time& _time,
				                                    size_t _nbChunk,
				                                    enum audio::format _format,
				                                    uint32_t _frequency,
				                                    const etk::Vector<audio::channel>& _map) {
				                                    	onDataReceived(_data, _time, _nbChunk, _format, _frequency, _map);
				                                    });
				
			}
			void onDataNeeded(void* _data,
			                  const audio::Time& _time,
			                  size_t _nbChunk,
			                  enum audio::format _format,
			                  uint32_t _frequency,
			                  const etk::Vector<audio::channel>& _map) {
				if (_format != audio::format_int16) {
					TEST_ERROR("call wrong type ... (need int16_t)");
				}
				m_buffer.read(_data, _nbChunk);
			}
			void onDataReceived(const void* _data,
			                    const audio::Time& _time,
			                    size_t _nbChunk,
			                    enum audio::format _format,
			                    uint32_t _frequency,
			                    const etk::Vector<audio::channel>& _map) {
				if (_format != audio::format_int16) {
					TEST_ERROR("call wrong type ... (need int16_t)");
				}
				m_buffer.write(_data, _nbChunk);
			}
			void start() {
				if(m_interfaceIn == null) {
					TEST_ERROR("null interface");
					return;
				}
				if(m_interfaceOut == null) {
					TEST_ERROR("null interface");
					return;
				}
				m_interfaceOut->start();
				m_interfaceIn->start();
			}
			void stop() {
				if(m_interfaceIn == null) {
					TEST_ERROR("null interface");
					return;
				}
				if(m_interfaceOut == null) {
					TEST_ERROR("null interface");
					return;
				}
				m_manager->generateDotAll("activeProcess.dot");
				m_interfaceOut->stop();
				m_interfaceIn->stop();
			}
	};
	
	static const etk::String configurationRiver =
		"{\n"
		"	speaker:{\n"
		"		io:'output',\n"
		"		map-on:{\n"
		"			interface:'auto',\n"
		"			name:'hw:0,0',\n"
		"			timestamp-mode:'trigered',\n"
		"		},\n"
		"		frequency:0,\n"
		"		channel-map:['front-left', 'front-right'],\n"
		"		type:'auto',\n"
		"		nb-chunk:1024,\n"
		"	},\n"
		"	microphone:{\n"
		"		io:'input',\n"
		"		map-on:{\n"
		"			interface:'auto',\n"
		"			name:'hw:0,0',\n"
		"			timestamp-mode:'trigered',\n"
		"		},\n"
		"		frequency:0,\n"
		"		channel-map:['front-left', 'front-right'],\n"
		"		type:'auto',\n"
		"		nb-chunk:1024\n"
		"	},\n"
		"	speaker-test:{\n"
		"		io:'output',\n"
		"		map-on:{\n"
		"			interface:'alsa',\n"
		"			name:'hw:2,0',\n"
		"			timestamp-mode:'trigered',\n"
		"		},\n"
		//"		group:'groupSynchro',\n"
		"		frequency:0,\n"
		"		channel-map:['front-left', 'front-right'],\n"
		"		type:'auto',\n"
		"		nb-chunk:1024\n"
		"	},\n"
		"	microphone-test:{\n"
		"		io:'input',\n"
		"		map-on:{\n"
		"			interface:'alsa',\n"
		"			name:'hw:2,0',\n"
		"			timestamp-mode:'trigered',\n"
		"		},\n"
		//"		group:'groupSynchro',\n"
		"		frequency:0,\n"
		"		channel-map:['front-center'],\n"
		"		type:'auto',\n"
		"		nb-chunk:1024\n"
		"	},\n"
		"	# virtual Nodes :\n"
		"	microphone-clean:{\n"
		"		io:'aec',\n"
		"		map-on-microphone:{\n"
		"			io:'input',\n"
		"			map-on:'microphone-test'\n"
		"		},\n"
		"		map-on-feedback:{\n"
		"			io:'feedback',\n"
		"			map-on:'speaker-test',\n"
		"		},\n"
		"		frequency:48000,\n"
		"		channel-map:[\n"
		"			'front-left', 'front-right'\n"
		//"			'front-center'\n"
		"		],\n"
		"		nb-chunk:1024,\n"
		"		type:'int16',\n"
		"		algo:'river-remover',\n"
		"		algo-mode:'cutter',\n"
		"		feedback-delay:10000,\n"
		"		mux-demux-type:'int16'\n"
		"	}\n"
		"}\n";
	
	TEST(TestUser, testAECManually) {
		audio::river::initString(configurationRiver);
		ememory::SharedPtr<audio::river::Manager> manager;
		manager = audio::river::Manager::create("testApplication");
		ememory::SharedPtr<Linker> processLink1 = ememory::makeShared<Linker>(manager, "microphone-clean", "speaker");
		ememory::SharedPtr<Linker> processLink2 = ememory::makeShared<Linker>(manager, "microphone", "speaker-test");
		processLink1->start();
		processLink2->start();
		ethread::sleepMilliSeconds(1000*(20));
		processLink1->stop();
		processLink2->stop();
		
		processLink1.reset();
		processLink2.reset();
		manager.reset();
		audio::river::unInit();
	}
};


