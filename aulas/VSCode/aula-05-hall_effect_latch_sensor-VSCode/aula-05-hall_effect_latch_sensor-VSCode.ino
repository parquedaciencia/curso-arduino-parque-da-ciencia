/*
 * Curso de Formação de Professores – Atividades Experimentais de Física
 * Local: Parque da Ciência Newton Freire Maia (PR, Brasil)
 * Plataforma: Arduino
 * Autor dos códigos originais: Aron da Rocha Battistella
 * Repositório: https://github.com/Dom-Aron/curso-arduino-parque-da-ciencia
 * Data da última revisão: 10/04/2026
 *
 * Descrição:
 *   ============================================================
 *   Projeto   : Aula 07 - Sensor Hall tipo latch
 *   Arquivo   : aula-07-hall_effect_latch_sensor.ino
 *   Biblioteca: Arduino padrão
 *
 *   Este programa realiza a medição de rotação utilizando um sensor
 *   de efeito Hall do tipo latch, como o US1881 ou modelos compatíveis.
 *   O sensor detecta a passagem do campo magnético gerado por um ímã
 *   acoplado a um eixo em rotação, permitindo estimar a frequência de
 *   giro, a velocidade angular, a aceleração centrípeta e o valor em RPM.
 *
 *   A estratégia adotada é baseada no período entre pulsos válidos,
 *   o que melhora bastante a estabilidade das medições em baixas rotações,
 *   especialmente quando comparado a métodos baseados em contagem de pulsos
 *   em janelas fixas de tempo.
 *
 *   O programa também aplica:
 *   - interrupção externa para detectar os pulsos do sensor;
 *   - filtro simples para rejeição de pulsos espúrios ou ruídos;
 *   - suavização exponencial para reduzir oscilações na leitura;
 *   - timeout para retornar a rotação a zero quando não houver novos pulsos.
 *
 *   Os dados são enviados pela Serial em formato adequado para plotagem,
 *   permitindo o acompanhamento em tempo real no Serial Plotter do VS Code
 *   ou em ferramentas equivalentes.
 *
 *   Componentes esperados:
 *   - 1 placa Arduino compatível;
 *   - 1 sensor Hall latch (ex.: US1881 / U18);
 *   - 1 ímã acoplado ao sistema rotativo;
 *   - fios de ligação;
 *   - conexão USB para alimentação e monitoramento serial.
 *
 *   Observações:
 *   - O sensor é tratado como saída do tipo open-drain, por isso o pino
 *   é configurado com INPUT_PULLUP.
 *   - O código considera, por padrão, 1 pulso válido por revolução.
 *   - Caso a montagem gere mais de um pulso por volta, o valor de
 *   kPulsesPerRevolution deve ser ajustado.
 *   ============================================================
 */

#include <Arduino.h>

namespace config
{
    constexpr uint8_t kHallPin = 3;
    constexpr unsigned long kBaudRate = 115200UL;

    // Physical setup
    constexpr float kRadiusMeters = 0.168f;  // Radius from axis to point of interest [m]

    // Mechanical/electrical relation
    constexpr float kPulsesPerRevolution = 1.0f;

    // Timing and filtering
    constexpr unsigned long kPublishIntervalMs = 100UL;
    constexpr unsigned long kSignalTimeoutUs = 9000000UL;  // 9 s, suitable for ~10 RPM with 1 pulse/rev
    constexpr unsigned long kMinPulseIntervalUs = 2000UL;  // Reject glitches/noise
    constexpr float kFilterAlpha = 0.30f;                  // Exponential smoothing on rotational frequency

    // For an open-drain Hall latch with pull-up, the active pulse is usually LOW.
    // Counting only FALLING avoids double counting caused by CHANGE.
    constexpr int kInterruptMode = FALLING;
}

struct Measurement
{
    float rotationFrequencyHz;
    float angularVelocityRadPerSecond;
    float centripetalAccelerationMetersPerSecondSquared;
    float rpm;
};

volatile uint32_t gLastPulseMicros = 0U;
volatile uint32_t gLastPulsePeriodUs = 0U;

/* --------------------- Funções principais --------------------- */

/*!
 * @brief Trata a interrupção gerada por um pulso válido do sensor Hall.
 *
 * Esta função é chamada automaticamente sempre que ocorre a borda de
 * interrupção configurada para o sensor. Ela registra o instante do pulso
 * atual e calcula o intervalo de tempo em microssegundos desde o pulso
 * anterior.
 *
 * Pulsos muito próximos entre si são descartados para reduzir a influência
 * de ruídos elétricos, glitches ou leituras indevidas.
 */
void onHallPulse()
{
    const uint32_t nowMicros = micros();

    if (gLastPulseMicros != 0U)
    {
        const uint32_t pulseIntervalUs = nowMicros - gLastPulseMicros;

        if (pulseIntervalUs < config::kMinPulseIntervalUs)
        {
            return;
        }

        gLastPulsePeriodUs = pulseIntervalUs;
    }

    gLastPulseMicros = nowMicros;
}

/*!
 * @brief Copia de forma atômica os dados temporais dos pulsos medidos.
 *
 * Como as variáveis globais de tempo são atualizadas dentro de uma rotina
 * de interrupção, esta função desabilita temporariamente as interrupções
 * para garantir que a leitura ocorra de forma consistente.
 *
 * @param[out] lastPulseMicros Instante, em microssegundos, do último pulso detectado.
 * @param[out] lastPulsePeriodUs Período, em microssegundos, entre os dois últimos pulsos válidos.
 */
void snapshotPulseData(
    uint32_t& lastPulseMicros,
    uint32_t& lastPulsePeriodUs)
{
    noInterrupts();
    lastPulseMicros = gLastPulseMicros;
    lastPulsePeriodUs = gLastPulsePeriodUs;
    interrupts();
}

/*!
 * @brief Calcula as grandezas físicas associadas à rotação medida.
 *
 * A partir do período entre pulsos válidos, esta função estima a frequência
 * de rotação, a velocidade angular, a aceleração centrípeta e a rotação em
 * RPM. Também aplica uma suavização exponencial para tornar a leitura menos
 * sensível a flutuações instantâneas.
 *
 * Caso nenhum pulso recente tenha sido detectado dentro do tempo limite
 * configurado, a função força a frequência filtrada para zero.
 *
 * @param[in] lastPulseMicros Instante do último pulso válido detectado.
 * @param[in] lastPulsePeriodUs Período entre pulsos válidos consecutivos.
 * @param[in] nowMicros Tempo atual em microssegundos.
 * @param[in,out] filteredRotationFrequencyHz Valor filtrado da frequência de rotação em hertz.
 *
 * @return Estrutura contendo frequência de rotação, velocidade angular,
 * aceleração centrípeta e RPM.
 */
Measurement computeMeasurement(
    const uint32_t lastPulseMicros,
    const uint32_t lastPulsePeriodUs,
    const uint32_t nowMicros,
    float& filteredRotationFrequencyHz)
{
    const bool hasValidPeriod = (lastPulsePeriodUs > 0U);
    const bool timedOut =
        (lastPulseMicros == 0U) ||
        ((nowMicros - lastPulseMicros) > config::kSignalTimeoutUs);

    float rawRotationFrequencyHz = 0.0f;

    if (!timedOut && hasValidPeriod)
    {
        const float pulseFrequencyHz =
            1.0e6f / static_cast<float>(lastPulsePeriodUs);

        rawRotationFrequencyHz =
            pulseFrequencyHz / config::kPulsesPerRevolution;
    }

    if (timedOut)
    {
        filteredRotationFrequencyHz = 0.0f;
    }
    else if (!hasValidPeriod)
    {
        // Um primeiro pulso já foi detectado, mas ainda não há um período completo.
        filteredRotationFrequencyHz = 0.0f;
    }
    else if (filteredRotationFrequencyHz == 0.0f)
    {
        filteredRotationFrequencyHz = rawRotationFrequencyHz;
    }
    else
    {
        filteredRotationFrequencyHz =
            (config::kFilterAlpha * rawRotationFrequencyHz) +
            ((1.0f - config::kFilterAlpha) * filteredRotationFrequencyHz);
    }

    const float angularVelocityRadPerSecond =
        2.0f * PI * filteredRotationFrequencyHz;

    const float centripetalAccelerationMetersPerSecondSquared =
        angularVelocityRadPerSecond *
        angularVelocityRadPerSecond *
        config::kRadiusMeters;

    const float rpm = filteredRotationFrequencyHz * 60.0f;

    return {
        filteredRotationFrequencyHz,
        angularVelocityRadPerSecond,
        centripetalAccelerationMetersPerSecondSquared,
        rpm
    };
}

/*!
 * @brief Envia uma linha formatada para visualização em plotter serial.
 *
 * Esta função transmite pela porta Serial os valores calculados para a
 * medição atual, usando um formato textual compatível com extensões de
 * plotagem serial, como o Serial Plotter do VS Code.
 *
 * @param[in] measurement Estrutura contendo os valores físicos calculados.
 */
void printForSerialPlotter(const Measurement& measurement)
{
    Serial.print(">freq_hz:");
    Serial.print(measurement.rotationFrequencyHz, 4);

    Serial.print(",omega_rad_s:");
    Serial.print(measurement.angularVelocityRadPerSecond, 4);

    Serial.print(",ac_m_s2:");
    Serial.print(measurement.centripetalAccelerationMetersPerSecondSquared, 4);

    Serial.print(",rpm:");
    Serial.println(measurement.rpm, 2);
}

/*!
 * @brief Realiza a configuração inicial do sistema.
 *
 * Esta função inicializa a comunicação serial, configura o pino do sensor
 * Hall com resistor de pull-up interno e associa a rotina de interrupção
 * ao pino utilizado para a leitura dos pulsos.
 */
void setup()
{
    Serial.begin(config::kBaudRate);

    pinMode(config::kHallPin, INPUT_PULLUP);

    attachInterrupt(
        digitalPinToInterrupt(config::kHallPin),
        onHallPulse,
        config::kInterruptMode);

    delay(300);
    Serial.println("US1881 Hall latch monitor started");
}

/*!
 * @brief Executa continuamente a aquisição, o processamento e a publicação dos dados.
 *
 * O laço principal controla o intervalo de atualização das medidas. A cada
 * período configurado, ele obtém uma cópia segura dos tempos dos pulsos,
 * calcula as grandezas físicas da rotação e envia os resultados para a
 * porta serial.
 */
void loop()
{
    static uint32_t previousPublishMillis = 0U;
    static float filteredRotationFrequencyHz = 0.0f;

    const uint32_t nowMillis = millis();

    if ((nowMillis - previousPublishMillis) < config::kPublishIntervalMs)
    {
        return;
    }

    previousPublishMillis = nowMillis;

    uint32_t lastPulseMicros = 0U;
    uint32_t lastPulsePeriodUs = 0U;

    snapshotPulseData(lastPulseMicros, lastPulsePeriodUs);

    const uint32_t nowMicros = micros();

    const Measurement measurement = computeMeasurement(
        lastPulseMicros,
        lastPulsePeriodUs,
        nowMicros,
        filteredRotationFrequencyHz);

    printForSerialPlotter(measurement);
}
