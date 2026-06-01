#include <portaudio.h>

export module lxlib.AudioSystem;

PaDeviceIndex findInputDevice()
{
	const PaDeviceIndex defaultDevice = Pa_GetDefaultInputDevice();
	if (defaultDevice != paNoDevice) {
		return defaultDevice;
	}

	const int deviceCount = Pa_GetDeviceCount();
	if (deviceCount < 0) {
		return paNoDevice;
	}

	for (PaDeviceIndex deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex) {
		const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(deviceIndex);
		if (deviceInfo != nullptr && deviceInfo->maxInputChannels > 0) {
			return deviceIndex;
		}
	}

	return paNoDevice;
}

namespace lx {
	export class AudioSystem {
	private:
		PaStream* micStream = nullptr;
		bool portAudioInitialized = false;
		using UserAudioCallback = void(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer);
		std::function<UserAudioCallback> userAudioCallback;

	public:
		void setupMic(std::function<UserAudioCallback> const& callback) {
			userAudioCallback = callback;

			PaError err;

			err = Pa_Initialize();
			if (err != paNoError)
			{
				const std::string message = "PortAudio init failed: " + std::string(Pa_GetErrorText(err));
				std::cerr << message << "\n";
				throw std::runtime_error(message);
			}
			portAudioInitialized = true;

			const PaDeviceIndex inputDevice = findInputDevice();
			if (inputDevice == paNoDevice) {
				shutdownMic();
				const std::string message = "No input audio device available.";
				std::cerr << message << "\n";
				throw std::runtime_error(message);
			}

			const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(inputDevice);
			if (deviceInfo == nullptr) {
				shutdownMic();
				const std::string message = "PortAudio returned no info for the selected input device.";
				std::cerr << message << "\n";
				throw std::runtime_error(message);
			}

			PaStreamParameters inputParameters{};
			inputParameters.device = inputDevice;
			inputParameters.channelCount = 1;
			inputParameters.sampleFormat = paFloat32;
			inputParameters.suggestedLatency = deviceInfo->defaultLowInputLatency;
			inputParameters.hostApiSpecificStreamInfo = nullptr;

			const double sampleRate = deviceInfo->defaultSampleRate > 0.0
				? deviceInfo->defaultSampleRate
				: 48000.0;
			cout << "Using audio input device: " << deviceInfo->name << " with sample rate: " << sampleRate << std::endl;
			err = Pa_OpenStream(
				&micStream,
				&inputParameters,
				nullptr,
				sampleRate,
				256,
				paNoFlag,
				audioCallback,
				this
			);

			if (err != paNoError)
			{
				const std::string message = "Failed to open input stream: " + std::string(Pa_GetErrorText(err));
				std::cerr << message << "\n";
				shutdownMic();
				throw std::runtime_error(message);
			}

			err = Pa_StartStream(micStream);
			if (err != paNoError)
			{
				const std::string message = "Failed to start input stream: " + std::string(Pa_GetErrorText(err));
				std::cerr << message << "\n";
				shutdownMic();
				throw std::runtime_error(message);
			}
		}

		void shutdownMic()
		{
			if (micStream != nullptr) {
				Pa_StopStream(micStream);
				Pa_CloseStream(micStream);
				micStream = nullptr;
			}
			if (portAudioInitialized) {
				Pa_Terminate();
				portAudioInitialized = false;
			}
		}

		static int audioCallback(const void* inputBuffer,
			void* outputBuffer,
			unsigned long framesPerBuffer,
			const PaStreamCallbackTimeInfo* timeInfo,
			PaStreamCallbackFlags statusFlags,
			void* userData)
		{
			(void)outputBuffer;
			(void)timeInfo;
			(void)statusFlags;
			AudioSystem* audioSystem = static_cast<AudioSystem*>(userData);

			const float* samples = (const float*)inputBuffer;

			if (samples != nullptr)
			{
				if (audioSystem->userAudioCallback) {
					audioSystem->userAudioCallback(inputBuffer, outputBuffer, framesPerBuffer);
				}
			}

			return paContinue;
		}
	};
};