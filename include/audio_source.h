#ifndef AUDIO_SOURCE_H
#define AUDIO_SOURCE_H

#include <Arduino.h>

/**
 * @brief Abstract interface for audio sources
 *
 * This interface allows testing audio module logic without
 * requiring actual I2S hardware or Arduino-specific libraries.
 *
 * Implementations:
 * - SineWaveSource: Generates test tones
 * - FileSource: Reads WAV files from storage
 * - StreamSource: Network audio streams (future)
 */
class IAudioSource {
public:
  virtual ~IAudioSource() = default;

  /**
   * @brief Get the source type identifier
   */
  virtual String getType() const = 0;

  /**
   * @brief Initialize the source
   * @return true if successful
   */
  virtual bool init() = 0;

  /**
   * @brief Start playback
   * @return true if successful
   */
  virtual bool start() = 0;

  /**
   * @brief Stop playback
   */
  virtual void stop() = 0;

  /**
   * @brief Check if source is currently active
   */
  virtual bool isActive() const = 0;

  /**
   * @brief Get human-readable description of source
   * Example: "440 Hz Sine Wave" or "recording.wav"
   */
  virtual String getDescription() const = 0;
};

/**
 * @brief Abstract interface for audio output
 *
 * This interface isolates I2S hardware dependencies,
 * allowing the audio module to be tested without hardware.
 *
 * Implementations:
 * - I2SOutput: Real hardware I2S output
 * - MockOutput: Testing output (silence)
 */
class IAudioOutput {
public:
  virtual ~IAudioOutput() = default;

  /**
   * @brief Initialize hardware
   * @return true if successful
   */
  virtual bool init() = 0;

  /**
   * @brief Set volume level
   * @param level Volume 0-100
   */
  virtual void setVolume(int level) = 0;

  /**
   * @brief Get current volume
   * @return Volume 0-100
   */
  virtual int getVolume() const = 0;

  /**
   * @brief Check if output is ready
   */
  virtual bool isReady() const = 0;
};

#endif // AUDIO_SOURCE_H
