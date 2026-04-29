/*
 * Curso de Formação de Professores – Atividades Experimentais de Física
 * Local: Parque da Ciência Newton Freire Maia (PR, Brasil)
 * Plataforma: Arduino
 * Ambiente alvo: Arduino IDE
 * Autores do curso: Aron da Rocha Battistella; Marcos Rocha; Alan Henrique Abreu Dias
 * Colaboradores do curso: Letícia Trzaskos Abbeg; Gabriel Cordeiro Chileider
 * Autoria dos códigos: Aron da Rocha Battistella e Marcos Rocha
 * Colaboração nos códigos: Letícia Trzaskos Abbeg, Gabriel Cordeiro Chileider e Alan Henrique Abreu Dias
 * Repositório: https://github.com/parquedaciencia/curso-arduino-parque-da-ciencia
 * Caminho no repositório: aulas/ArduinoIDE/aula-02-potentiometer-ArduinoIDE/aula-02-potentiometer-ArduinoIDE.ino
 * Data da última revisão: 29/04/2026
 *
 * Descrição:
 *   ============================================================
 *   Projeto   : Aula 02 - Potentiometer
 *   Arquivo   : aula-02-potentiometer-ArduinoIDE.ino
 *   Pasta     : aula-02-potentiometer-ArduinoIDE
 *   Este sketch lê a posição de um potenciômetro conectado à entrada
 *   analógica A0 e utiliza esse valor para controlar o brilho de um
 *   LED por PWM.
 *
 *   Durante a execução, os valores também podem ser enviados pela
 *   Serial para acompanhamento didático da relação entre leitura
 *   analógica e intensidade luminosa.
 *   ============================================================
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
