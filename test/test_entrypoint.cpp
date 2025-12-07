#include <unity.h>

// Include native test headers for interface/type tests
// These are pure C++ tests with mocked Arduino APIs via ArduinoFake
#ifdef NATIVE_PLATFORM
#include <ArduinoFake.h>
#include <testing/testing_platform_provider.h>
using namespace fakeit;

// Forward declarations from included sources
void register_audio_module_tests();

// Global provider that persists across tests (but gets reset in setUp)
static MockWebPlatformProvider *globalProvider = nullptr;

extern "C" void setUp(void) {
  ArduinoFakeReset();
  // Create a fresh mock platform instance for each test
  if (globalProvider) {
    delete globalProvider;
  }
  globalProvider = new MockWebPlatformProvider();
  IWebPlatformProvider::instance = globalProvider;

  // Stub Serial methods to avoid side effects in native tests
  When(OverloadedMethod(ArduinoFake(Serial), println, size_t(const char *)))
      .AlwaysReturn(1);
  When(OverloadedMethod(ArduinoFake(Serial), println, size_t(const String &)))
      .AlwaysReturn(1);
  // Note: Cannot stub variadic printf - it's OK, not used in testable code

  // Stub delay to avoid timing side-effects in native tests
  When(Method(ArduinoFake(), delay))
      .AlwaysReturn(); // Stub millis for timing-related tests
  When(Method(ArduinoFake(), millis)).AlwaysReturn(0UL);
}

extern "C" void tearDown(void) {
  // Reset platform instance to avoid static destructor order issues
  IWebPlatformProvider::instance = nullptr;
  // Delete the global provider to avoid memory leaks if a test fails between setUp and tearDown
  if (globalProvider) {
    delete globalProvider;
    globalProvider = nullptr;
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();

  // Register and run native tests
  register_audio_module_tests();

  UNITY_END();

  // Clean up global provider
  if (globalProvider) {
    delete globalProvider;
    globalProvider = nullptr;
  }

  return 0;
}

// ESP32 entrypoint - basic compilation test
#else
#include <Arduino.h>

// ESP32 test entrypoint: runs on-device tests located under test/esp32
// (recursive) Each on-device test file should expose a registrar function that
// runs its own RUN_TEST calls. We invoke those here.

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

// Forward declaration for ESP32 test registration
void register_esp32_audio_tests();

void setup() {
  // Allow USB CDC/Serial to enumerate
  delay(2000);
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  UNITY_BEGIN();
  // Give the serial monitor a moment to attach before printing results
  delay(500);

  // Register and run esp32 tests
  register_esp32_audio_tests();

  UNITY_END();
}

void loop() {
  // No-op
}
#endif
