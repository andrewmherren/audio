#ifndef PLAYBACK_ENGINE_H
#define PLAYBACK_ENGINE_H

#include <Arduino.h>

// Forward declare audio tools classes
namespace audio_tools {
class I2SStream;
class SineWaveGenerator;
class GeneratedSoundStream;
} // namespace audio_tools

/**
 * @brief Playback Engine - Handles audio output through I2S
 *
 * Modular component responsible for:
 * - I2S hardware initialization and management
 * - Audio source selection (file, stream, generated)
 * - Playback state management
 * - Volume control
 */
class PlaybackEngine {
public:
  enum class State { STOPPED, PLAYING, PAUSED };

  enum class SourceType {
    NONE,
    SINE_WAVE, // Test tone
    FILE,      // WAV file from LittleFS
    STREAM     // Future: network stream
  };

  PlaybackEngine();
  ~PlaybackEngine();

  /**
   * @brief Initialize I2S output hardware
   * @param bck_pin Bit clock pin
   * @param ws_pin Word select (LRCLK) pin
   * @param data_pin Data pin
   * @param sample_rate Sample rate in Hz
   * @param bits_per_sample Bit depth (16, 24, or 32)
   * @param channels Number of channels (1=mono, 2=stereo)
   * @return true if initialization successful
   */
  bool begin(int bck_pin, int ws_pin, int data_pin, int sample_rate = 16000,
             int bits_per_sample = 16, int channels = 1);

  /**
   * @brief Must be called in loop() to process audio
   */
  void handle();

  /**
   * @brief Play a sine wave test tone
   * @param frequency Frequency in Hz (default 440Hz = A4 note)
   * @return true if started successfully
   */
  bool playSineWave(float frequency = 440.0);

  /**
   * @brief Play audio from a file
   * @param filePath Path to WAV file in LittleFS
   * @return true if started successfully
   */
  bool playFile(const String &filePath);

  /**
   * @brief Stop playback
   */
  void stop();

  /**
   * @brief Pause playback (can be resumed)
   */
  void pause();

  /**
   * @brief Resume paused playback
   */
  void resume();

  /**
   * @brief Set volume level
   * @param level Volume 0-100
   */
  void setVolume(int level);

  /**
   * @brief Get current volume level
   * @return Volume 0-100
   */
  int getVolume() const { return volume; }

  /**
   * @brief Get current playback state
   */
  State getState() const { return state; }

  /**
   * @brief Get current source type
   */
  SourceType getSourceType() const { return sourceType; }

  /**
   * @brief Check if playing
   */
  bool isPlaying() const { return state == State::PLAYING; }

  /**
   * @brief Check if paused
   */
  bool isPaused() const { return state == State::PAUSED; }

private:
  State state;
  SourceType sourceType;
  int volume; // 0-100

  // Audio Tools components (using pointers to avoid header dependency)
  audio_tools::I2SStream *i2sOutput;
  audio_tools::SineWaveGenerator<int16_t> *sineGen;
  audio_tools::GeneratedSoundStream<int16_t> *generatedStream;

  // Configuration
  int sampleRate;
  int bitsPerSample;
  int channels;

  // Internal helpers
  void cleanup();
};

#endif // PLAYBACK_ENGINE_H
