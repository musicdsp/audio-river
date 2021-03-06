/** @file
 * @author Edouard DUPIN 
 * @copyright 2015, Edouard DUPIN, all right reserved
 * @license MPL v2.0 (see license file)
 */
#pragma once

#ifdef AUDIO_RIVER_BUILD_PORTAUDIO

#include <audio/river/Interface.hpp>
#include <audio/river/io/Node.hpp>
#include <portaudio/portaudio.hpp>

namespace audio {
	namespace river {
		namespace io {
			class Manager;
			//! @not_in_doc
			class NodePortAudio : public Node {
				protected:
					/**
					 * @brief Constructor
					 */
					NodePortAudio(const etk::String& _name, const ejson::Object& _config);
				public:
					static ememory::SharedPtr<NodePortAudio> create(const etk::String& _name, const ejson::Object& _config);
					/**
					 * @brief Destructor
					 */
					virtual ~NodePortAudio();
					virtual bool isHarwareNode() {
						return true;
					};
				protected:
					PaStream* m_stream;
				public:
					int32_t duplexCallback(const void* _inputBuffer,
					                       const audio::Time& _timeInput,
					                       void* _outputBuffer,
					                       const audio::Time& _timeOutput,
					                       uint32_t _nbChunk,
					                       PaStreamCallbackFlags _status);
				protected:
					virtual void start();
					virtual void stop();
			};
		}
	}
}
#endif

