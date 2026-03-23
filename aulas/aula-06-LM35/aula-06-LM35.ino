#include <math.h>

/*
  LM35 control with automatic ambient calibration and two-threshold hysteresis.

  Behavior:
  - Reads LM35 on A0
  - Uses INTERNAL 1.1 V reference for better resolution
  - Averages ADC readings
  - Applies exponential smoothing
  - Blinks D2 once at startup to indicate the system is alive
  - Automatically calibrates ambient temperature at startup
  - Defines two limits relative to the measured ambient temperature:
      * Turn ON below ambient - offset
      * Turn OFF above ambient + offset
  - Keeps the previous state while temperature stays between the two limits
  - Exposes noise metrics to help quantify signal fluctuation quality
  - Sends text output and plot-friendly output through Serial

  Notes:
  - Use the 1N4007 across the relay coil as flyback protection,
    not on the LM35 analog output.
  - The analog signal should go directly to A0, or at most through a small
    series resistor such as 220 ohms.
*/

static constexpr uint8_t kLm35Pin = A0;
static constexpr uint8_t kOutputPin = 2;

static constexpr float kAdcReferenceVoltage = 1.1F;

/*
  Automatic control limits based on startup ambient temperature:

  turn_on_temperature  = ambient_temperature - kTurnOnOffsetBelowAmbientC
  turn_off_temperature = ambient_temperature + kTurnOffOffsetAboveAmbientC
*/
static constexpr float kTurnOnOffsetBelowAmbientC = 0.20F;
static constexpr float kTurnOffOffsetAboveAmbientC = 0.25F;
static constexpr float kFilterAlpha = 0.15F;

static constexpr uint8_t kSamplesPerReading = 32;
static constexpr unsigned long kSampleIntervalMs = 500UL;
static constexpr unsigned long kCalibrationDurationMs = 5000UL;

/*
  Startup blink to indicate the output is working.
  Assumes active-HIGH output on D2.
*/
static constexpr unsigned long kStartupBlinkOnTimeMs = 300UL;
static constexpr unsigned long kStartupBlinkOffTimeMs = 300UL;

/*
  Optional stabilization window after switching the output.
  This helps ignore transient electrical interference from the relay/lamp.
*/
static constexpr unsigned long kSettleAfterSwitchMs = 700UL;

/* Noise monitoring configuration */
static constexpr float kNoiseAlpha = 0.20F;
static constexpr float kNoiseBadReferenceC = 0.20F;

bool g_outputState = false;
bool g_filterInitialized = false;
bool g_previousRawInitialized = false;

unsigned long g_lastSampleTimeMs = 0UL;
unsigned long g_lastSwitchTimeMs = 0UL;

float g_filteredTemperatureC = 0.0F;
float g_ambientTemperatureC = 0.0F;
float g_turnOnTemperatureC = 25.0F;
float g_turnOffTemperatureC = 26.0F;

float g_previousRawTemperatureC = 0.0F;
float g_rawStepC = 0.0F;
float g_noiseEmaC = 0.0F;
float g_rawFilterErrorC = 0.0F;
float g_noiseBadnessPct = 0.0F;

float read_adc_average()
{
  unsigned long sum = 0UL;

  for (uint8_t i = 0; i < kSamplesPerReading; ++i)
  {
    sum += static_cast<unsigned long>(analogRead(kLm35Pin));
    delay(2);
  }

  return static_cast<float>(sum) / static_cast<float>(kSamplesPerReading);
}

float convert_adc_to_voltage(const float adcValue)
{
  return (adcValue * kAdcReferenceVoltage) / 1023.0F;
}

float convert_voltage_to_temperature_c(const float voltage)
{
  return voltage * 100.0F;
}

float read_raw_temperature_c()
{
  const float adcAverage = read_adc_average();
  const float voltage = convert_adc_to_voltage(adcAverage);
  return convert_voltage_to_temperature_c(voltage);
}

float apply_exponential_filter(const float currentTemperatureC)
{
  if (!g_filterInitialized)
  {
    g_filteredTemperatureC = currentTemperatureC;
    g_filterInitialized = true;
    return g_filteredTemperatureC;
  }

  g_filteredTemperatureC =
      (kFilterAlpha * currentTemperatureC) +
      ((1.0F - kFilterAlpha) * g_filteredTemperatureC);

  return g_filteredTemperatureC;
}

void blink_startup_indicator()
{
  digitalWrite(kOutputPin, HIGH);
  delay(kStartupBlinkOnTimeMs);

  digitalWrite(kOutputPin, LOW);
  delay(kStartupBlinkOffTimeMs);
}

float calibrate_ambient_temperature()
{
  const unsigned long startTimeMs = millis();

  float sumTemperatureC = 0.0F;
  unsigned int sampleCount = 0U;

  while ((millis() - startTimeMs) < kCalibrationDurationMs)
  {
    const float rawTemperatureC = read_raw_temperature_c();
    sumTemperatureC += rawTemperatureC;
    ++sampleCount;

    Serial.print("Calibrating ambient... sample ");
    Serial.print(sampleCount);
    Serial.print(" | Temp=");
    Serial.print(rawTemperatureC, 2);
    Serial.println(" C");

    delay(120);
  }

  if (sampleCount == 0U)
  {
    return read_raw_temperature_c();
  }

  return sumTemperatureC / static_cast<float>(sampleCount);
}

void update_noise_metrics(
    const float rawTemperatureC,
    const float filteredTemperatureC)
{
  if (!g_previousRawInitialized)
  {
    g_previousRawTemperatureC = rawTemperatureC;
    g_previousRawInitialized = true;
    g_rawStepC = 0.0F;
    g_noiseEmaC = 0.0F;
    g_rawFilterErrorC = 0.0F;
    g_noiseBadnessPct = 0.0F;
    return;
  }

  g_rawStepC = fabs(rawTemperatureC - g_previousRawTemperatureC);
  g_rawFilterErrorC = fabs(rawTemperatureC - filteredTemperatureC);

  g_noiseEmaC =
      (kNoiseAlpha * g_rawStepC) +
      ((1.0F - kNoiseAlpha) * g_noiseEmaC);

  g_noiseBadnessPct = constrain(
      (g_noiseEmaC / kNoiseBadReferenceC) * 100.0F,
      0.0F,
      100.0F);

  g_previousRawTemperatureC = rawTemperatureC;
}

const char* get_noise_state_label()
{
  if (g_noiseEmaC < 0.03F)
  {
    return "VERY_STABLE";
  }

  if (g_noiseEmaC < 0.08F)
  {
    return "OK";
  }

  if (g_noiseEmaC < 0.20F)
  {
    return "NOISY";
  }

  return "BAD";
}

bool is_in_settle_window()
{
  return (millis() - g_lastSwitchTimeMs) < kSettleAfterSwitchMs;
}

void set_output_state(const bool newOutputState)
{
  if (g_outputState == newOutputState)
  {
    return;
  }

  g_outputState = newOutputState;
  digitalWrite(kOutputPin, g_outputState ? HIGH : LOW);
  g_lastSwitchTimeMs = millis();
}

void update_output_state(const float filteredTemperatureC)
{
  if (is_in_settle_window())
  {
    return;
  }

  if (!g_outputState && (filteredTemperatureC <= g_turnOnTemperatureC))
  {
    set_output_state(true);
  }
  else if (g_outputState && (filteredTemperatureC >= g_turnOffTemperatureC))
  {
    set_output_state(false);
  }
}

float get_output_marker_c()
{
  const float markerOffsetC = 2.0F;

  if (g_outputState)
  {
    return g_turnOnTemperatureC - markerOffsetC;
  }

  return g_turnOffTemperatureC + markerOffsetC;
}

void print_human_readable_status(
    const float rawTemperatureC,
    const float filteredTemperatureC)
{
  Serial.print("AmbientC=");
  Serial.print(g_ambientTemperatureC, 2);

  Serial.print(" | TurnOnBelowC=");
  Serial.print(g_turnOnTemperatureC, 2);

  Serial.print(" | TurnOffAboveC=");
  Serial.print(g_turnOffTemperatureC, 2);

  Serial.print(" | RawC=");
  Serial.print(rawTemperatureC, 2);

  Serial.print(" | FilteredC=");
  Serial.print(filteredTemperatureC, 2);

  Serial.print(" | RawStepC=");
  Serial.print(g_rawStepC, 3);

  Serial.print(" | NoiseEmaC=");
  Serial.print(g_noiseEmaC, 3);

  Serial.print(" | RawFilterErrorC=");
  Serial.print(g_rawFilterErrorC, 3);

  Serial.print(" | NoiseBadnessPct=");
  Serial.print(g_noiseBadnessPct, 1);

  Serial.print(" | NoiseState=");
  Serial.print(get_noise_state_label());

  Serial.print(" | InSettleWindow=");
  Serial.print(is_in_settle_window() ? "YES" : "NO");

  Serial.print(" | D2=");
  Serial.println(g_outputState ? "ON" : "OFF");
}

void send_plot_data(
    const float rawTemperatureC,
    const float filteredTemperatureC)
{
  Serial.print(">raw_c:");
  Serial.print(rawTemperatureC, 2);

  Serial.print(",filtered_c:");
  Serial.print(filteredTemperatureC, 2);

  Serial.print(",ambient_c:");
  Serial.print(g_ambientTemperatureC, 2);

  Serial.print(",turn_on_c:");
  Serial.print(g_turnOnTemperatureC, 2);

  Serial.print(",turn_off_c:");
  Serial.print(g_turnOffTemperatureC, 2);

  Serial.print(",d2_marker_c:");
  Serial.print(get_output_marker_c(), 2);

  Serial.print(",raw_step_c:");
  Serial.print(g_rawStepC, 3);

  Serial.print(",noise_ema_c:");
  Serial.print(g_noiseEmaC, 3);

  Serial.print(",raw_filter_error_c:");
  Serial.print(g_rawFilterErrorC, 3);

  Serial.print(",noise_badness_pct:");
  Serial.println(g_noiseBadnessPct, 1);
}

void setup()
{
  pinMode(kOutputPin, OUTPUT);
  digitalWrite(kOutputPin, LOW);
  g_outputState = false;
  g_lastSwitchTimeMs = 0UL;

  Serial.begin(115200);

  analogReference(INTERNAL);
  delay(100);

  for (uint8_t i = 0; i < 10; ++i)
  {
    analogRead(kLm35Pin);
    delay(5);
  }

  Serial.println();
  Serial.println("LM35 startup test");
  Serial.println("Blinking D2 once before calibration...");
  blink_startup_indicator();

  Serial.println();
  Serial.println("LM35 automatic calibration started");
  Serial.println("Keep the sensor untouched during startup calibration");

  g_ambientTemperatureC = calibrate_ambient_temperature();
  g_turnOnTemperatureC =
      g_ambientTemperatureC - kTurnOnOffsetBelowAmbientC;
  g_turnOffTemperatureC =
      g_ambientTemperatureC + kTurnOffOffsetAboveAmbientC;

  if (g_turnOnTemperatureC >= g_turnOffTemperatureC)
  {
    const float centerTemperatureC = g_ambientTemperatureC;
    g_turnOnTemperatureC = centerTemperatureC - 0.5F;
    g_turnOffTemperatureC = centerTemperatureC + 0.5F;
  }

  g_filteredTemperatureC = g_ambientTemperatureC;
  g_filterInitialized = true;
  g_previousRawTemperatureC = g_ambientTemperatureC;
  g_previousRawInitialized = true;
  g_rawStepC = 0.0F;
  g_noiseEmaC = 0.0F;
  g_rawFilterErrorC = 0.0F;
  g_noiseBadnessPct = 0.0F;

  digitalWrite(kOutputPin, LOW);
  g_outputState = false;
  g_lastSwitchTimeMs = millis();

  Serial.println();
  Serial.println("Calibration finished");
  Serial.print("Ambient temperature: ");
  Serial.print(g_ambientTemperatureC, 2);
  Serial.println(" C");

  Serial.print("Turn ON below: ");
  Serial.print(g_turnOnTemperatureC, 2);
  Serial.println(" C");

  Serial.print("Turn OFF above: ");
  Serial.print(g_turnOffTemperatureC, 2);
  Serial.println(" C");

  Serial.print("Settle window after switching: ");
  Serial.print(kSettleAfterSwitchMs);
  Serial.println(" ms");

  Serial.println();
  Serial.println("Monitoring started");
}

void loop()
{
  const unsigned long currentTimeMs = millis();

  if ((currentTimeMs - g_lastSampleTimeMs) >= kSampleIntervalMs)
  {
    g_lastSampleTimeMs = currentTimeMs;

    const float rawTemperatureC = read_raw_temperature_c();
    const float filteredTemperatureC = apply_exponential_filter(rawTemperatureC);

    update_noise_metrics(rawTemperatureC, filteredTemperatureC);
    update_output_state(filteredTemperatureC);
    print_human_readable_status(rawTemperatureC, filteredTemperatureC);
    send_plot_data(rawTemperatureC, filteredTemperatureC);
  }
}
