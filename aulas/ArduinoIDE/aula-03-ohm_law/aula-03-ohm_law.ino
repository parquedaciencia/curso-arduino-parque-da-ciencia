/*
 * Curso de Formação de Professores – Atividades Experimentais de Física
 * Local: Parque da Ciência Newton Freire Maia (PR, Brasil)
 * Plataforma: Arduino
 * Autor dos códigos originais: Aron da Rocha Battistella
 * Repositório: https://github.com/Dom-Aron/curso-arduino-parque-da-ciencia
 * Data da última revisão: 10/04/2026
 *
 * Descrição:
 *   /
 *   ============================================================
 *   Projeto   : Aula 09 - Ohm's Law
 *   Arquivo   : aula-09-ohm_law.ino
 *   Plataforma: Arduino Uno
 *   Este programa realiza a aquisição experimental de dados para o
 *   estudo da Lei de Ohm, medindo a tensão sobre um componente em teste
 *   e estimando a corrente que o atravessa a partir da tensão medida em
 *   um resistor shunt conhecido.
 *
 *   O objetivo do código é detectar automaticamente apenas o intervalo
 *   útil em que o potenciômetro está sendo movimentado pelo usuário,
 *   enviando pela serial somente uma varredura coerente para posterior
 *   captura no computador por meio de um script em Python.
 *
 *   Funcionamento geral:
 *   - A tensão total do circuito é lida no pino A0.
 *   - A tensão sobre o resistor shunt é lida no pino A1.
 *   - A tensão no componente em teste é obtida por diferença.
 *   - A corrente é calculada pela relação I = V_shunt / R_shunt.
 *   - O sistema identifica quando houve movimento real do potenciômetro.
 *   - Ao detectar movimento, inicia uma varredura automática.
 *   - Enquanto houver mudança significativa, os pontos são enviados pela
 *     serial em formato estruturado.
 *   - Quando o sistema percebe que o potenciômetro parou de variar por
 *     certo tempo, a varredura é encerrada automaticamente.
 *
 *   Componentes esperados:
 *   - 1 Arduino Uno (ou compatível com ADC de 10 bits e referência de 5 V)
 *   - 1 potenciômetro para variação manual da tensão de entrada
 *   - 1 componente em teste (resistor, diodo ou outro dispositivo didático)
 *   - 1 resistor shunt de 220 ohms para inferência da corrente
 *   - Jumpers e protoboard para montagem do circuito
 *   - 1 computador para leitura da serial e salvamento dos dados
 *
 *   Montagem sugerida:
 *   - 5V do Arduino  -> terminal 1 do potenciômetro
 *   - GND do Arduino -> terminal 3 do potenciômetro
 *   - Cursor do potenciômetro -> entrada do componente em teste
 *   - Saída do componente em teste -> resistor shunt de 220 ohms
 *   - Outro terminal do shunt -> GND
 *
 *   Pontos de medição:
 *   - A0: tensão total aplicada ao circuito
 *   - A1: tensão no resistor shunt
 *
 *   Relações físicas utilizadas:
 *   - V_componente = V_total - V_shunt
 *   - I_componente = V_shunt / R_shunt
 *
 *   Protocolo serial enviado ao computador:
 *   - Linhas informativas começam com '#'
 *   - Início da varredura: START\t<timestamp_ms>\t<sweep_id>
 *   - Dados: DATA\t<timestamp_ms>\t<sweep_id>\t<V_comp_V>\t<I_comp_mA>\t<V_total_V>\t<V_shunt_V>
 *   - Fim da varredura: END\t<timestamp_ms>\t<sweep_id>\t<sample_count>
 *
 *   Observações:
 *   - O Arduino não cria diretamente um arquivo .dat no computador.
 *   - Um script hospedeiro em Python deve ler a porta serial e salvar
 *     os dados recebidos.
 *   - O código aplica média de amostras analógicas para reduzir ruído.
 *   - Pequenos valores negativos espúrios são anulados por segurança
 *     numérica, pois não são fisicamente esperados nesta montagem.
 *   ============================================================
 *   /
 */

#include <math.h>

namespace {

constexpr float kAdcReferenceVolts = 5.0f;
constexpr float kShuntResistanceOhms = 220.0f;
constexpr unsigned long kSampleIntervalMs = 25UL;
constexpr unsigned long kMovementSettleTimeMs = 350UL;
constexpr unsigned long kIdleMessageIntervalMs = 2000UL;
constexpr uint8_t kSamplesPerChannel = 8U;
constexpr float kMovementThresholdVolts = 0.015f;
constexpr float kNegativeClampVolts = -0.010f;

struct Measurement {
  float vTotal;
  float vShunt;
  float vComponent;
  float iMilliamp;
};

bool gIsSweepActive = false;
bool gHasReference = false;
unsigned long gLastSampleMs = 0UL;
unsigned long gLastChangeMs = 0UL;
unsigned long gLastIdleMessageMs = 0UL;
unsigned long gSweepStartMs = 0UL;
uint32_t gSweepId = 0UL;
uint32_t gSweepSampleCount = 0UL;
float gMovementReferenceVolts = 0.0f;

/* --------------------- Funções utilitárias --------------------- */

/*!
 * @brief Converte uma leitura bruta do ADC para tensão em volts.
 *
 * Esta função utiliza a referência analógica configurada no Arduino e
 * assume resolução de 10 bits, convertendo o valor inteiro retornado
 * por analogRead para sua tensão equivalente.
 *
 * @param rawValue Valor bruto lido pelo conversor analógico-digital.
 *
 * @return Tensão correspondente, em volts.
 */
float convertRawToVolts(const int rawValue) {
  return (static_cast<float>(rawValue) * kAdcReferenceVolts) / 1023.0f;
}

/*!
 * @brief Lê repetidamente um pino analógico e retorna a média das amostras.
 *
 * A função realiza múltiplas leituras consecutivas do pino especificado
 * para reduzir o efeito de ruídos instantâneos presentes na medição.
 * O valor final retornado corresponde à média aritmética das amostras.
 *
 * @param pin Pino analógico a ser lido.
 *
 * @return Valor médio bruto do ADC para o pino informado.
 */
int readAnalogAverage(const uint8_t pin) {
  long sum = 0L;

  for (uint8_t index = 0U; index < kSamplesPerChannel; ++index) {
    sum += analogRead(pin);
  }

  return static_cast<int>(sum / static_cast<long>(kSamplesPerChannel));
}

/*!
 * @brief Lê o circuito experimental e calcula as grandezas elétricas úteis.
 *
 * A função realiza a leitura dos pinos A0 e A1, converte os valores para
 * tensão, calcula a tensão no componente em teste por diferença e estima
 * a corrente elétrica com base na tensão sobre o resistor shunt.
 *
 * Também são aplicadas correções simples para impedir a propagação de
 * pequenos valores negativos causados por ruído ou imprecisão numérica.
 *
 * @return Estrutura contendo tensão total, tensão no shunt, tensão no
 * componente e corrente estimada em miliampères.
 */
Measurement readMeasurement() {
  const int rawA0 = readAnalogAverage(A0);
  const int rawA1 = readAnalogAverage(A1);

  const float vTotal = convertRawToVolts(rawA0);
  const float vShunt = convertRawToVolts(rawA1);

  float vComponent = vTotal - vShunt;

  if (vComponent < 0.0f && vComponent > kNegativeClampVolts) {
    vComponent = 0.0f;
  }

  if (vComponent < kNegativeClampVolts) {
    // Um valor claramente negativo não é fisicamente esperado nesta montagem.
    // O valor é forçado para zero para evitar registrar um ponto inválido.
    vComponent = 0.0f;
  }

  float iMilliamp = (vShunt / kShuntResistanceOhms) * 1000.0f;

  if (iMilliamp < 0.0f) {
    iMilliamp = 0.0f;
  }

  return Measurement{vTotal, vShunt, vComponent, iMilliamp};
}

/*!
 * @brief Exibe uma mensagem periódica enquanto nenhuma varredura está ativa.
 *
 * Esta função envia pela porta serial um resumo do estado atual das medições
 * somente após transcorrido o intervalo mínimo configurado entre mensagens.
 * Isso evita poluir a serial com avisos excessivos enquanto o sistema aguarda
 * o início do movimento do potenciômetro.
 *
 * @param nowMs Instante atual, em milissegundos, obtido com millis().
 * @param measurement Estrutura com a medição elétrica mais recente.
 */
void printIdleMessage(const unsigned long nowMs, const Measurement& measurement) {
  if ((nowMs - gLastIdleMessageMs) < kIdleMessageIntervalMs) {
    return;
  }

  Serial.print(F("# Waiting for potentiometer movement | V_total="));
  Serial.print(measurement.vTotal, 4);
  Serial.print(F(" V | V_component="));
  Serial.print(measurement.vComponent, 4);
  Serial.print(F(" V | I_component="));
  Serial.print(measurement.iMilliamp, 4);
  Serial.println(F(" mA"));

  gLastIdleMessageMs = nowMs;
}

/*!
 * @brief Inicia uma nova varredura experimental.
 *
 * A função marca internamente o começo de uma nova sequência de aquisição,
 * reinicia os contadores associados, incrementa o identificador da varredura
 * e envia pela serial os marcadores informativos e estruturados de início.
 *
 * @param nowMs Instante atual, em milissegundos, em que a varredura começou.
 */
void startSweep(const unsigned long nowMs) {
  gIsSweepActive = true;
  gSweepStartMs = nowMs;
  gLastChangeMs = nowMs;
  gSweepSampleCount = 0UL;
  ++gSweepId;

  Serial.print(F("# Sweep started: id="));
  Serial.print(gSweepId);
  Serial.print(F(" at "));
  Serial.print(nowMs);
  Serial.println(F(" ms"));

  Serial.print(F("START\t"));
  Serial.print(nowMs);
  Serial.print(F("\t"));
  Serial.println(gSweepId);
}

/*!
 * @brief Encerra a varredura experimental em andamento.
 *
 * A função envia o marcador estruturado de término da varredura, informa
 * pela serial a duração total e o número de amostras registradas, e por fim
 * desativa o estado interno que indica aquisição ativa.
 *
 * @param nowMs Instante atual, em milissegundos, em que a varredura terminou.
 */
void finishSweep(const unsigned long nowMs) {
  Serial.print(F("END\t"));
  Serial.print(nowMs);
  Serial.print(F("\t"));
  Serial.print(gSweepId);
  Serial.print(F("\t"));
  Serial.println(gSweepSampleCount);

  Serial.print(F("# Sweep finished: id="));
  Serial.print(gSweepId);
  Serial.print(F(" | duration="));
  Serial.print(nowMs - gSweepStartMs);
  Serial.print(F(" ms | samples="));
  Serial.println(gSweepSampleCount);

  gIsSweepActive = false;
}

/*!
 * @brief Envia uma linha de dados da varredura atual pela serial.
 *
 * Cada chamada gera uma linha estruturada contendo carimbo de tempo,
 * identificador da varredura e os valores elétricos calculados para o ponto
 * experimental corrente. Ao final, o contador de amostras é incrementado.
 *
 * @param nowMs Instante atual, em milissegundos, associado à amostra.
 * @param measurement Estrutura contendo as medições calculadas do ponto atual.
 */
void emitData(const unsigned long nowMs, const Measurement& measurement) {
  Serial.print(F("DATA\t"));
  Serial.print(nowMs);
  Serial.print(F("\t"));
  Serial.print(gSweepId);
  Serial.print(F("\t"));
  Serial.print(measurement.vComponent, 6);
  Serial.print(F("\t"));
  Serial.print(measurement.iMilliamp, 6);
  Serial.print(F("\t"));
  Serial.print(measurement.vTotal, 6);
  Serial.print(F("\t"));
  Serial.println(measurement.vShunt, 6);

  ++gSweepSampleCount;
}

}  // namespace

/*!
 * @brief Inicializa a comunicação serial e apresenta as instruções do experimento.
 *
 * Esta função é executada uma única vez no início do programa. Ela configura
 * a serial em 115200 bps, define a referência analógica padrão do Arduino e
 * envia mensagens iniciais informando ao usuário como o sistema opera e qual
 * é o formato das colunas transmitidas pela serial.
 */
void setup() {
  Serial.begin(115200);

  while (!Serial) {
    ;
  }

  analogReference(DEFAULT);

  Serial.println(F("# Ohm's law acquisition initialized."));
  Serial.println(F("# Move the potentiometer to start a sweep."));
  Serial.println(F("# Columns: DATA<TAB>timestamp_ms<TAB>sweep_id<TAB>V_component_V<TAB>I_component_mA<TAB>V_total_V<TAB>V_shunt_V"));
}

/*!
 * @brief Executa continuamente a lógica principal de aquisição do experimento.
 *
 * A função controla o ritmo de amostragem, realiza a leitura do circuito,
 * verifica se houve movimento significativo do potenciômetro, inicia ou encerra
 * varreduras automaticamente e decide se deve enviar dados úteis ou apenas
 * mensagens de espera pela serial.
 *
 * O encerramento da varredura ocorre quando o sistema detecta ausência de
 * mudanças relevantes por um tempo suficiente para considerar que o usuário
 * parou de ajustar o potenciômetro.
 */
void loop() {
  const unsigned long nowMs = millis();

  if ((nowMs - gLastSampleMs) < kSampleIntervalMs) {
    return;
  }

  gLastSampleMs = nowMs;

  const Measurement measurement = readMeasurement();

  if (!gHasReference) {
    gMovementReferenceVolts = measurement.vTotal;
    gHasReference = true;
  }

  const float movementDelta = fabsf(measurement.vTotal - gMovementReferenceVolts);
  const bool significantChange = movementDelta >= kMovementThresholdVolts;

  if (significantChange) {
    gMovementReferenceVolts = measurement.vTotal;
    gLastChangeMs = nowMs;

    if (!gIsSweepActive) {
      startSweep(nowMs);
    }
  }

  if (gIsSweepActive) {
    emitData(nowMs, measurement);

    if ((nowMs - gLastChangeMs) >= kMovementSettleTimeMs) {
      finishSweep(nowMs);
    }
  } else {
    printIdleMessage(nowMs, measurement);
  }
}
