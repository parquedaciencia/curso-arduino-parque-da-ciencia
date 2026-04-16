/*
 * Curso de Formação de Professores – Atividades Experimentais de Física
 * Local: Parque da Ciência Newton Freire Maia (PR, Brasil)
 * Plataforma: Arduino
 * Ambiente alvo: Arduino IDE
 * Autor dos códigos originais: Aron da Rocha Battistella
 * Repositório: https://github.com/Dom-Aron/curso-arduino-parque-da-ciencia
 * Caminho no repositório: aulas/ArduinoIDE/aula-05-hall_effect_latch_sensor-ArduinoIDE/aula-05-Hall_Effect_Latch_Sensor-ArduinoIDE.ino
 * Data da última revisão: 16/04/2026
 *
 * Descrição:
 *   ============================================================
 *   Projeto   : Aula 05 - Hall Effect Latch Sensor
 *   Arquivo   : aula-05-Hall_Effect_Latch_Sensor-ArduinoIDE.ino
 *   Pasta     : aula-05-hall_effect_latch_sensor-ArduinoIDE
 *   Este sketch mede a rotação de um sistema com auxílio de um sensor
 *   de efeito Hall do tipo latch, como o US1881 ou equivalentes.
 *
 *   A partir do tempo entre pulsos válidos, o programa estima
 *   frequência, velocidade angular, aceleração centrípeta e RPM,
 *   enviando os resultados pela Serial em formato compatível com o
 *   Serial Plotter do Arduino IDE.
 *   ============================================================
 */
constexpr uint8_t kHallPin = 3;
constexpr unsigned long kBaudRate = 115200UL;

// Configuração física do experimento
constexpr float kRadiusMeters = 0.05f;
constexpr float kEdgesPerRevolution = 2.0f;

// Temporização e filtragem do sinal
constexpr unsigned long kPublishIntervalMs = 200UL;
constexpr unsigned long kSignalTimeoutUs = 3000000UL;
constexpr unsigned long kMinEdgeIntervalUs = 1000UL;
constexpr float kFilterAlpha = 1.0f;

struct Measurement
{
  float rotationFrequencyHz;
  float angularVelocityRadPerSecond;
  float centripetalAccelerationMetersPerSecondSquared;
  float rpm;
};

volatile uint32_t gEdgeCount = 0U;
volatile uint32_t gLastEdgeMicros = 0U;
volatile uint32_t gLastEdgePeriodUs = 0U;

/* --------------------- Funções utilitárias --------------------- */

/*!
 * @brief Trata a interrupção gerada pelas bordas do sensor Hall.
 *
 * Esta função é chamada automaticamente sempre que ocorre uma
 * mudança de estado no pino do sensor. Ela registra o instante da
 * borda detectada, calcula o intervalo em microssegundos entre a
 * borda atual e a anterior e descarta intervalos muito curtos, que
 * podem corresponder a ruídos espúrios ou leituras indevidas.
 */
void onHallEdge()
{
  const uint32_t nowMicros = micros();

  if (gLastEdgeMicros != 0U)
  {
    const uint32_t edgeIntervalUs = nowMicros - gLastEdgeMicros;

    if (edgeIntervalUs < kMinEdgeIntervalUs)
    {
      return;
    }

    gLastEdgePeriodUs = edgeIntervalUs;
  }

  gLastEdgeMicros = nowMicros;
  ++gEdgeCount;
}

/*!
 * @brief Copia de forma atômica os dados voláteis das bordas detectadas.
 *
 * Como as variáveis globais associadas ao sensor podem ser alteradas
 * dentro da rotina de interrupção, esta função desabilita temporariamente
 * as interrupções para realizar uma leitura consistente desses valores.
 *
 * @param edgeCount Variável de saída que receberá o número total de bordas válidas detectadas.
 * @param lastEdgeMicros Variável de saída que receberá o instante, em microssegundos, da última borda válida.
 * @param lastEdgePeriodUs Variável de saída que receberá o intervalo, em microssegundos, entre as duas últimas bordas válidas.
 */
void snapshotEdgeData(
  uint32_t& edgeCount,
  uint32_t& lastEdgeMicros,
  uint32_t& lastEdgePeriodUs)
{
  noInterrupts();
  edgeCount = gEdgeCount;
  lastEdgeMicros = gLastEdgeMicros;
  lastEdgePeriodUs = gLastEdgePeriodUs;
  interrupts();
}

/*!
 * @brief Calcula as grandezas físicas associadas à rotação medida.
 *
 * A função utiliza o número de bordas detectadas, o tempo entre
 * publicações e o período mais recente do sinal para estimar a
 * frequência de bordas. Em seguida, converte essa frequência em
 * frequência de rotação, velocidade angular, aceleração centrípeta
 * e RPM. Também há suporte a suavização exponencial por meio do
 * parâmetro de filtro configurado no código.
 *
 * @param totalEdges Número total de bordas válidas detectadas até o instante atual.
 * @param lastEdgeMicros Instante, em microssegundos, da última borda válida detectada.
 * @param lastEdgePeriodUs Intervalo, em microssegundos, entre as duas últimas bordas válidas.
 * @param nowMicros Instante atual, em microssegundos.
 * @param previousEdgeCount Referência para o total de bordas da última publicação, atualizado ao final da função.
 * @param previousPublishMicros Referência para o instante da última publicação, atualizado ao final da função.
 * @param filteredEdgeFrequencyHz Referência para o valor filtrado da frequência de bordas, atualizado ao final da função.
 *
 * @return Estrutura Measurement contendo frequência de rotação,
 * velocidade angular, aceleração centrípeta e RPM.
 */
Measurement computeMeasurement(
  const uint32_t totalEdges,
  const uint32_t lastEdgeMicros,
  const uint32_t lastEdgePeriodUs,
  const uint32_t nowMicros,
  uint32_t& previousEdgeCount,
  uint32_t& previousPublishMicros,
  float& filteredEdgeFrequencyHz)
{
  const uint32_t elapsedUs = nowMicros - previousPublishMicros;
  const uint32_t deltaEdges = totalEdges - previousEdgeCount;

  const bool timedOut =
    (lastEdgeMicros == 0U) ||
    ((nowMicros - lastEdgeMicros) > kSignalTimeoutUs);

  float edgeFrequencyHz = 0.0f;

  if (deltaEdges > 0U && elapsedUs > 0U)
  {
    edgeFrequencyHz =
      (1.0e6f * static_cast<float>(deltaEdges)) /
      static_cast<float>(elapsedUs);
  }
  else if (!timedOut && lastEdgePeriodUs > 0U)
  {
    edgeFrequencyHz = 1.0e6f / static_cast<float>(lastEdgePeriodUs);
  }

  if (timedOut)
  {
    filteredEdgeFrequencyHz = 0.0f;
  }
  else
  {
    filteredEdgeFrequencyHz =
      (kFilterAlpha * edgeFrequencyHz) +
      ((1.0f - kFilterAlpha) * filteredEdgeFrequencyHz);
  }

  previousEdgeCount = totalEdges;
  previousPublishMicros = nowMicros;

  const float rotationFrequencyHz =
    filteredEdgeFrequencyHz / kEdgesPerRevolution;

  const float angularVelocityRadPerSecond =
    2.0f * PI * rotationFrequencyHz;

  const float centripetalAccelerationMetersPerSecondSquared =
    angularVelocityRadPerSecond *
    angularVelocityRadPerSecond *
    kRadiusMeters;

  const float rpm = rotationFrequencyHz * 60.0f;

  Measurement measurement;
  measurement.rotationFrequencyHz = rotationFrequencyHz;
  measurement.angularVelocityRadPerSecond = angularVelocityRadPerSecond;
  measurement.centripetalAccelerationMetersPerSecondSquared =
    centripetalAccelerationMetersPerSecondSquared;
  measurement.rpm = rpm;

  return measurement;
}

/*!
 * @brief Envia os valores calculados em formato compatível com o Serial Plotter.
 *
 * Esta função organiza a saída serial em pares no formato
 * nome:valor, separados por vírgulas, de modo a facilitar a leitura
 * e a plotagem dos dados na IDE do Arduino.
 *
 * @param measurement Estrutura contendo as grandezas físicas já calculadas.
 */
void printForArduinoPlotter(const Measurement& measurement)
{
  Serial.print("freq_hz:");
  Serial.print(measurement.rotationFrequencyHz, 4);
  Serial.print(',');

  Serial.print("omega_rad_s:");
  Serial.print(measurement.angularVelocityRadPerSecond, 4);
  Serial.print(',');

  Serial.print("ac_m_s2:");
  Serial.print(measurement.centripetalAccelerationMetersPerSecondSquared, 4);
  Serial.print(',');

  Serial.print("rpm:");
  Serial.println(measurement.rpm, 2);
}

/*!
 * @brief Inicializa a comunicação serial e configura o sensor Hall.
 *
 * Nesta etapa o código inicia a porta serial, define o pino do
 * sensor como entrada com pull-up interno e associa a rotina de
 * interrupção à mudança de estado do sinal.
 */
void setup()
{
  Serial.begin(kBaudRate);
  pinMode(kHallPin, INPUT_PULLUP);

  attachInterrupt(
    digitalPinToInterrupt(kHallPin),
    onHallEdge,
    CHANGE);

  delay(300);
}

/*!
 * @brief Executa continuamente a aquisição, o cálculo e a publicação dos dados.
 *
 * O laço principal respeita o intervalo de publicação configurado,
 * obtém uma cópia segura dos dados compartilhados com a interrupção,
 * calcula as grandezas físicas da rotação e envia os resultados pela
 * porta serial para visualização e análise.
 */
void loop()
{
  static uint32_t previousPublishMillis = 0U;
  static uint32_t previousPublishMicros = 0U;
  static uint32_t previousEdgeCount = 0U;
  static float filteredEdgeFrequencyHz = 0.0f;

  const uint32_t nowMillis = millis();

  if ((nowMillis - previousPublishMillis) < kPublishIntervalMs)
  {
    return;
  }

  previousPublishMillis = nowMillis;

  uint32_t edgeCount = 0U;
  uint32_t lastEdgeMicros = 0U;
  uint32_t lastEdgePeriodUs = 0U;

  snapshotEdgeData(edgeCount, lastEdgeMicros, lastEdgePeriodUs);

  const uint32_t nowMicros = micros();

  if (previousPublishMicros == 0U)
  {
    previousPublishMicros = nowMicros;
  }

  const Measurement measurement = computeMeasurement(
    edgeCount,
    lastEdgeMicros,
    lastEdgePeriodUs,
    nowMicros,
    previousEdgeCount,
    previousPublishMicros,
    filteredEdgeFrequencyHz);

  printForArduinoPlotter(measurement);
}
