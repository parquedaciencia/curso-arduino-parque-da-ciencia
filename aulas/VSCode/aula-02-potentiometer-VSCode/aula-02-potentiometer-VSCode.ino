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
 *   Projeto   : Aula 02 - Potentiometer
 *   Arquivo   : aula-02-potentiometer.ino
 *   Este programa realiza a leitura de um potenciômetro conectado
 *   à entrada analógica A0 e utiliza esse valor para controlar o
 *   brilho de um LED por meio de modulação por largura de pulso
 *   (PWM) no pino digital 3.
 *
 *   Durante a execução, o Arduino:
 *   - lê continuamente a posição do potenciômetro;
 *   - converte a leitura analógica (0 a 1023) para a faixa de PWM
 *     utilizada pelo analogWrite (0 a 255);
 *   - ajusta o brilho do LED conforme a posição do potenciômetro;
 *   - envia periodicamente pela Serial os valores de leitura e o
 *     nível de brilho aplicado.
 *
 *   Componentes esperados:
 *   - 1 placa Arduino compatível;
 *   - 1 potenciômetro ligado entre 5 V, GND e cursor em A0;
 *   - 1 LED ligado ao pino 3;
 *   - 1 resistor em série com o LED para limitar a corrente.
 *
 *   Observações:
 *   - O pino 3 deve oferecer suporte a PWM.
 *   - A leitura serial é apenas de acompanhamento e não interfere
 *     diretamente no controle do brilho.
 *   ============================================================
 *   /
 */

static constexpr uint8_t LED_PIN = 3;
static constexpr uint8_t POTENTIOMETER_PIN = A0;
static constexpr long SERIAL_BAUD_RATE = 9600L;
static constexpr unsigned long SERIAL_INTERVAL_MS = 200UL;

unsigned long g_lastSerialTimeMs = 0UL;

/*!
 * @brief Lê o valor atual do potenciômetro na entrada analógica.
 *
 * Esta função realiza uma leitura analógica do pino configurado
 * para o potenciômetro e retorna o valor bruto fornecido pelo
 * conversor analógico-digital do Arduino.
 *
 * @return Valor inteiro da leitura analógica, normalmente entre
 * 0 e 1023.
 */
int readPotentiometerValue()
{
    return analogRead(POTENTIOMETER_PIN);
}

/*!
 * @brief Converte a leitura do potenciômetro para a faixa de PWM.
 *
 * O valor lido na entrada analógica é convertido da faixa de
 * 0 a 1023 para a faixa de 0 a 255, compatível com o comando
 * analogWrite utilizado no controle do brilho do LED.
 *
 * @param potentiometerValue Valor bruto lido do potenciômetro.
 *
 * @return Valor de brilho em PWM, entre 0 e 255.
 */
int convertPotentiometerToPwm(const int potentiometerValue)
{
    return map(potentiometerValue, 0, 1023, 0, 255);
}

/*!
 * @brief Aplica ao LED o brilho calculado.
 *
 * Esta função envia ao pino do LED o valor de PWM correspondente,
 * ajustando a intensidade luminosa conforme o valor recebido.
 *
 * @param brightness Valor de brilho em PWM, entre 0 e 255.
 */
void applyLedBrightness(const int brightness)
{
    analogWrite(LED_PIN, brightness);
}

/*!
 * @brief Exibe no monitor serial os valores atuais do sistema.
 *
 * Esta função imprime o valor analógico lido do potenciômetro e o
 * valor de PWM aplicado ao LED, permitindo acompanhar o experimento
 * e verificar a relação entre entrada analógica e saída PWM.
 *
 * @param potentiometerValue Valor lido do potenciômetro.
 * @param brightness Valor de PWM aplicado ao LED.
 */
void printStatus(const int potentiometerValue, const int brightness)
{
    Serial.print("Valor do potenciometro: ");
    Serial.print(potentiometerValue);
    Serial.print(" | Brilho PWM: ");
    Serial.println(brightness);
}

/*!
 * @brief Realiza a configuração inicial do Arduino.
 *
 * Esta função é executada uma única vez no início do programa.
 * Nela são configurados o pino do LED como saída, o pino do
 * potenciômetro como entrada e a comunicação serial para
 * acompanhamento do experimento.
 */
void setup()
{
    pinMode(LED_PIN, OUTPUT);
    pinMode(POTENTIOMETER_PIN, INPUT);

    Serial.begin(SERIAL_BAUD_RATE);

    Serial.println("Sistema iniciado.");
    Serial.println("Controle de brilho do LED por potenciometro.");
}

/*!
 * @brief Executa continuamente a lógica principal do experimento.
 *
 * Em cada iteração do loop, o programa lê o potenciômetro,
 * converte o valor para PWM, aplica o brilho ao LED e, em
 * intervalos regulares, envia os dados atuais pela Serial.
 */
void loop()
{
    const int potentiometerValue = readPotentiometerValue();
    const int brightness = convertPotentiometerToPwm(potentiometerValue);

    applyLedBrightness(brightness);

    const unsigned long currentTimeMs = millis();

    if (currentTimeMs - g_lastSerialTimeMs >= SERIAL_INTERVAL_MS)
    {
        g_lastSerialTimeMs = currentTimeMs;
        printStatus(potentiometerValue, brightness);
    }
}
