#include <unity.h>

#ifndef NATIVE_PLATFORM
#include <Arduino.h>
#include <audio_module.h>

// ===========================================================================
// ESP32 Compilation Tests
// ===========================================================================
// These tests verify the audio module compiles and links on ESP32 hardware.
// They don't test actual audio playback (that requires hardware setup).

static void test_audio_module_instantiation() {
  // Should be able to create an audio module instance
  AudioModule *module = new AudioModule();
  TEST_ASSERT_NOT_NULL(module);
  delete module;
}

static void test_audio_module_metadata() {
  AudioModule module;
  TEST_ASSERT_EQUAL_STRING("Audio Module", module.getModuleName().c_str());
  TEST_ASSERT_EQUAL_STRING("0.1.0", module.getModuleVersion().c_str());
}

static void test_audio_module_routes() {
  AudioModule module;
  auto routes = module.getHttpRoutes();
  TEST_ASSERT_GREATER_OR_EQUAL(6, routes.size());
}

// ===========================================================================
// Test Registration
// ===========================================================================

void register_esp32_audio_tests() {
  RUN_TEST(test_audio_module_instantiation);
  RUN_TEST(test_audio_module_metadata);
  RUN_TEST(test_audio_module_routes);
}

#endif // !NATIVE_PLATFORM
