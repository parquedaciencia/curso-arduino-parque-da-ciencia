/*
  Temperature monitoring for 6 LM35 sensors using Arduino IDE only.

  Features:
  - Reads 6 analog sensors (A0 to A5)
  - Samples every 125 ms
  - Computes average temperature over a 15-second window
  - Sends data through Serial
  - Can be used with Serial Monitor or Serial Plotter

  Notes:
  - This version replaces the old Processing GUI.
  - For a graph inside the Arduino IDE, open: Tools -> Serial Plotter
  - Formula assumes:
      * LM35 sensor
      * 5.0 V ADC reference
      * 10-bit ADC (0..1023), such as Arduino Uno/Nano
*/

enum OutputMode
{
  OUTPUT_HUMAN_READABLE,
  OUTPUT_SERIAL_PLOTTER
};

// -----------------------------------------------------------------------------
// User configuration
// -----------------------------------------------------------------------------
static constexpr OutputMode kOutputMode = OUTPUT_SERIAL_PLOTTER;

static constexpr uint8_t kSensorCount = 6;
static constexpr uint8_t kSensorPins[kSensorCount] = {A0, A1, A2, A3, A4, A5};

static constexpr unsigned long kSampleIntervalMs = 125UL;
static constexpr unsigned long kReportIntervalMs = 15000UL;  // 15 seconds

static constexpr float kAdcReferenceVoltage = 5.0f;   // Use 5.0 for Uno/Nano at 5 V
static constexpr float kAdcResolution = 1023.0f;      // 10-bit ADC
static constexpr float kLm35MilliVoltsPerC = 10.0f;   // LM35 = 10 mV / °C

// -----------------------------------------------------------------------------
// Global state
// -----------------------------------------------------------------------------
float g_lastTemperatureC[kSensorCount] = {0.0f};
float g_sumTemperatureC[kSensorCount] = {0.0f};
float g_averageTemperatureC[kSensorCount] = {0.0f};

uint16_t g_sampleCount = 0;
unsigned long g_lastSampleMs = 0UL;
unsigned long g_windowStartMs = 0UL;

// -----------------------------------------------------------------------------
// Function declarations
// -----------------------------------------------------------------------------
float readTemperatureC(uint8_t analogPin);
void sampleAllSensors();
void computeAverages();
void resetAveragingWindow();
void printHumanReadableReport();
void printSerialPlotterReport();
void printStartupMessage();
void printSeparator();

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------
void setup()
{
  Serial.begin(115200);

  for (uint8_t i = 0; i < kSensorCount; ++i)
  {
    pinMode(kSensorPins[i], INPUT);
  }

  g_lastSampleMs = millis();
  g_windowStartMs = millis();

  printStartupMessage();
}

// -----------------------------------------------------------------------------
// Main loop
// -----------------------------------------------------------------------------
void loop()
{
  const unsigned long currentMs = millis();

  // Keep sampling at a fixed interval.
  if (currentMs - g_lastSampleMs >= kSampleIntervalMs)
  {
    g_lastSampleMs += kSampleIntervalMs;
    sampleAllSensors();
  }

  // Report one averaged point every 15 seconds.
  if (currentMs - g_windowStartMs >= kReportIntervalMs)
  {
    computeAverages();

    if (kOutputMode == OUTPUT_HUMAN_READABLE)
    {
      printHumanReadableReport();
    }
    else
    {
      printSerialPlotterReport();
    }

    resetAveragingWindow();
    g_windowStartMs += kReportIntervalMs;
  }
}

// -----------------------------------------------------------------------------
// Reads one LM35 sensor and converts the ADC value to Celsius.
// -----------------------------------------------------------------------------
float readTemperatureC(uint8_t analogPin)
{
  const int adcValue = analogRead(analogPin);

  const float voltage = (static_cast<float>(adcValue) * kAdcReferenceVoltage) / kAdcResolution;
  const float temperatureC = (voltage * 1000.0f) / kLm35MilliVoltsPerC;

  return temperatureC;
}

// -----------------------------------------------------------------------------
// Reads all sensors and accumulates the values for averaging.
// -----------------------------------------------------------------------------
void sampleAllSensors()
{
  for (uint8_t i = 0; i < kSensorCount; ++i)
  {
    g_lastTemperatureC[i] = readTemperatureC(kSensorPins[i]);
    g_sumTemperatureC[i] += g_lastTemperatureC[i];
  }

  ++g_sampleCount;
}

// -----------------------------------------------------------------------------
// Computes the average temperature for the current 15-second window.
// -----------------------------------------------------------------------------
void computeAverages()
{
  if (g_sampleCount == 0)
  {
    for (uint8_t i = 0; i < kSensorCount; ++i)
    {
      g_averageTemperatureC[i] = 0.0f;
    }
    return;
  }

  for (uint8_t i = 0; i < kSensorCount; ++i)
  {
    g_averageTemperatureC[i] = g_sumTemperatureC[i] / static_cast<float>(g_sampleCount);
  }
}

// -----------------------------------------------------------------------------
// Clears the accumulation buffers for the next averaging window.
// -----------------------------------------------------------------------------
void resetAveragingWindow()
{
  for (uint8_t i = 0; i < kSensorCount; ++i)
  {
    g_sumTemperatureC[i] = 0.0f;
  }

  g_sampleCount = 0;
}

// -----------------------------------------------------------------------------
// Prints a human-readable report for the Serial Monitor.
// -----------------------------------------------------------------------------
void printHumanReadableReport()
{
  const unsigned long elapsedSeconds = g_windowStartMs / 1000UL;

  printSeparator();
  Serial.print(F("Time: "));
  Serial.print(elapsedSeconds);
  Serial.println(F(" s"));

  Serial.print(F("Samples in window: "));
  Serial.println(g_sampleCount);

  for (uint8_t i = 0; i < kSensorCount; ++i)
  {
    Serial.print(F("Sensor "));
    Serial.print(i + 1);
    Serial.print(F(" | Avg: "));
    Serial.print(g_averageTemperatureC[i], 1);
    Serial.print(F(" C | Last: "));
    Serial.print(g_lastTemperatureC[i], 1);
    Serial.println(F(" C"));
  }
}

// -----------------------------------------------------------------------------
// Prints one line formatted for the Arduino IDE Serial Plotter.
//
// Each line becomes one point in the graph.
// -----------------------------------------------------------------------------
void printSerialPlotterReport()
{
  for (uint8_t i = 0; i < kSensorCount; ++i)
  {
    Serial.print(F("S"));
    Serial.print(i + 1);
    Serial.print(F(":"));
    Serial.print(g_averageTemperatureC[i], 1);

    if (i < (kSensorCount - 1))
    {
      Serial.print('\t');
    }
  }

  Serial.println();
}

// -----------------------------------------------------------------------------
// Prints initial information.
// -----------------------------------------------------------------------------
void printStartupMessage()
{
  delay(300);

  printSeparator();
  Serial.println(F("6-channel LM35 temperature monitor started."));
  Serial.print(F("Sample interval: "));
  Serial.print(kSampleIntervalMs);
  Serial.println(F(" ms"));

  Serial.print(F("Average window: "));
  Serial.print(kReportIntervalMs / 1000UL);
  Serial.println(F(" s"));

  Serial.print(F("Output mode: "));
  if (kOutputMode == OUTPUT_HUMAN_READABLE)
  {
    Serial.println(F("Human readable"));
  }
  else
  {
    Serial.println(F("Serial Plotter"));
  }

  Serial.println(F("Pins: A0, A1, A2, A3, A4, A5"));
  printSeparator();
}

// -----------------------------------------------------------------------------
// Prints a separator line.
// -----------------------------------------------------------------------------
void printSeparator()
{
  Serial.println(F("--------------------------------------------------"));
}