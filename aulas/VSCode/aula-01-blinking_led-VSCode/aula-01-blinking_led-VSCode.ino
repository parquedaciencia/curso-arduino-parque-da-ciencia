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
 *   Projeto   : Aula 01 - Blinking LED
 *   Arquivo   : aula-01-blinking_led.ino
 *   Este programa realiza o acionamento cíclico de um LED conectado
 *   a uma saída digital do Arduino, alternando entre os estados
 *   ligado e desligado em intervalos fixos de 1 segundo.
 *
 *   O objetivo deste sketch é apresentar uma introdução ao uso de
 *   portas digitais de saída, à estrutura básica de um programa para
 *   Arduino e ao controle temporal simples por meio da função delay().
 *
 *   Componentes esperados:
 *   - 1 placa Arduino compatível
 *   - 1 LED
 *   - 1 resistor em série para limitação de corrente
 *   - Jumpers para conexão
 *
 *   Ligações esperadas:
 *   - Ânodo do LED ligado ao pino digital 2 por meio de resistor
 *   - Cátodo do LED ligado ao GND
 *
 *   Observações:
 *   - O valor do resistor deve ser adequado para proteger o LED e
 *   limitar a corrente fornecida pela porta digital.
 *   - O intervalo de acionamento pode ser alterado modificando as
 *   constantes de tempo definidas no início do código.
 *   ============================================================
 */

static constexpr uint8_t PINO_LED = 2;
static constexpr unsigned long TEMPO_ACESO_MS = 1000;
static constexpr unsigned long TEMPO_APAGADO_MS = 1000;

/*!
 * @brief Configura os recursos iniciais do Arduino.
 *
 * Esta função é executada uma única vez após a energização ou o reset
 * da placa. Neste programa, ela configura o pino utilizado pelo LED
 * como saída digital para permitir seu acionamento pelo microcontrolador.
 */
void setup()
{
    // Define o pino do LED como saída digital.
    pinMode(PINO_LED, OUTPUT);
}

/*!
 * @brief Executa continuamente o ciclo de acionamento do LED.
 *
 * Esta função implementa o comportamento principal do experimento.
 * O LED é ligado por um intervalo fixo e, em seguida, desligado por
 * outro intervalo fixo, produzindo o efeito visual de piscar de forma
 * periódica e contínua.
 */
void loop()
{
    // Liga o LED.
    digitalWrite(PINO_LED, HIGH);
    delay(TEMPO_ACESO_MS);

    // Desliga o LED.
    digitalWrite(PINO_LED, LOW);
    delay(TEMPO_APAGADO_MS);
}
