// PlaybackEngine is hardware-specific and only compiles for ESP32/Arduino
// Native tests don't need this code - they test AudioModule logic only
#if defined(ARDUINO) || defined(ESP_PLATFORM)

#include "playback_engine.h"
#include "AudioTools.h"

using namespace audio_tools;

PlaybackEngine::PlaybackEngine()
    : state(State::STOPPED), sourceType(SourceType::NONE), volume(75),
      i2sOutput(nullptr), sineGen(nullptr), generatedStream(nullptr),
      sampleRate(16000), bitsPerSample(16), channels(1) {}

PlaybackEngine::~PlaybackEngine() { cleanup(); }

bool PlaybackEngine::begin(int bck_pin, int ws_pin, int data_pin,
                           int sample_rate, int bits_per_sample,
                           int channels_count) {
  // Store configuration
  sampleRate = sample_rate;
  bitsPerSample = bits_per_sample;
  channels = channels_count;

  // Create I2S output stream
  i2sOutput = new I2SStream();

  // Configure I2S
  I2SConfig i2sConfig = i2sOutput->defaultConfig(TX_MODE);
  i2sConfig.sample_rate = sampleRate;
  i2sConfig.bits_per_sample = bitsPerSample;
  i2sConfig.channels = channels;
  i2sConfig.pin_bck = bck_pin;
  i2sConfig.pin_ws = ws_pin;
  i2sConfig.pin_data = data_pin;

  // Start I2S
  if (!i2sOutput->begin(i2sConfig)) {
    Serial.println("Audio: Failed to initialize I2S output");
    delete i2sOutput;
    i2sOutput = nullptr;
    return false;
  }

  Serial.println("Audio: I2S output initialized");
  Serial.printf("  Sample Rate: %d Hz\n", sampleRate);
  Serial.printf("  Bits/Sample: %d\n", bitsPerSample);
  Serial.printf("  Channels: %d\n", channels);
  Serial.printf("  BCK Pin: %d, WS Pin: %d, Data Pin: %d\n", bck_pin, ws_pin,
                data_pin);

  return true;
}

void PlaybackEngine::handle() {
  // Future: handle file playback streaming here
  // For now, sine wave generation is automatic
}

bool PlaybackEngine::playSineWave(float frequency) {
  if (!i2sOutput) {
    Serial.println("Audio: I2S not initialized");
    return false;
  }

  // Stop any existing playback
  stop();

  Serial.printf("Audio: Starting sine wave at %.1f Hz\n", frequency);

  // Create sine wave generator
  sineGen = new SineWaveGenerator<int16_t>();
  sineGen->begin(channels, sampleRate, frequency);

  // Create generated sound stream
  generatedStream = new GeneratedSoundStream<int16_t>();
  generatedStream->begin(*sineGen, *i2sOutput);

  // Update state
  state = State::PLAYING;
  sourceType = SourceType::SINE_WAVE;

  Serial.println("Audio: Sine wave playback started");
  return true;
}

bool PlaybackEngine::playFile(const String &filePath) {
  // TODO: Implement file playback in next phase
  Serial.println("Audio: File playback not yet implemented");
  return false;
}

void PlaybackEngine::stop() {
  if (state == State::STOPPED) {
    return;
  }

  Serial.println("Audio: Stopping playback");

  // Clean up audio sources
  if (generatedStream) {
    generatedStream->end();
    delete generatedStream;
    generatedStream = nullptr;
  }

  if (sineGen) {
    delete sineGen;
    sineGen = nullptr;
  }

  state = State::STOPPED;
  sourceType = SourceType::NONE;
}

void PlaybackEngine::pause() {
  if (state == State::PLAYING) {
    // TODO: Implement pause functionality
    state = State::PAUSED;
    Serial.println("Audio: Playback paused");
  }
}

void PlaybackEngine::resume() {
  if (state == State::PAUSED) {
    state = State::PLAYING;
    Serial.println("Audio: Playback resumed");
  }
}

void PlaybackEngine::setVolume(int level) {
  volume = constrain(level, 0, 100);
  // TODO: Implement actual volume control
  Serial.printf("Audio: Volume set to %d%%\n", volume);
}

void PlaybackEngine::cleanup() {
  stop();

  if (i2sOutput) {
    i2sOutput->end();
    delete i2sOutput;
    i2sOutput = nullptr;
  }
}

#endif // ARDUINO || ESP_PLATFORM
