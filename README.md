# Curso de Arduino — Parque da Ciência Newton Freire Maia

<p align="center">
  <strong>Formação de professores em atividades experimentais de Física com Arduino</strong>
</p>

<p align="center">
  <a href="https://github.com/Parque-da-Ciencia/curso-arduino-parque-da-ciencia">
    <img src="https://img.shields.io/badge/GitHub-Parque--da--Ci%C3%AAncia-181717?style=for-the-badge&logo=github" alt="GitHub do Parque da Ciência">
  </a>
  <img src="https://img.shields.io/badge/Arduino-Experimenta%C3%A7%C3%A3o-00979D?style=for-the-badge&logo=arduino&logoColor=white" alt="Arduino">
  <img src="https://img.shields.io/badge/F%C3%ADsica-Ensino%20Experimental-blue?style=for-the-badge" alt="Física Experimental">
</p>

---

## Sobre o projeto

Este repositório reúne os códigos, materiais de apoio e exemplos utilizados no curso de formação de professores do **Parque da Ciência Newton Freire Maia**, com foco no uso da plataforma **Arduino** em atividades experimentais de Física.

A proposta do curso é aproximar professores da Educação Básica de práticas acessíveis de instrumentação, aquisição de dados, automação e visualização de fenômenos físicos, utilizando componentes eletrônicos simples e programação em C/C++ para Arduino.

O material foi organizado para servir tanto como apoio durante a formação quanto como referência posterior para planejamento de aulas, oficinas, demonstrações experimentais e projetos escolares.

---

## Objetivos

* Introduzir o uso do Arduino como ferramenta de ensino experimental.
* Relacionar programação, eletrônica básica e conceitos de Física.
* Desenvolver atividades práticas com sensores, atuadores e displays.
* Estimular a construção de experimentos de baixo custo para a sala de aula.
* Apoiar professores na criação, adaptação e documentação de práticas investigativas.

---

## Estrutura do repositório

O repositório está organizado em duas versões principais dos códigos:

```text
curso-arduino-parque-da-ciencia/
├── ArduinoIDE/          # Códigos preparados para uso direto na Arduino IDE
├── VSCode/              # Códigos preparados para uso no VS Code / PlatformIO ou fluxo similar
├── docs/                # Documentação, materiais auxiliares e arquivos LaTeX
├── tools/               # Scripts utilitários para imagens, documentação e processamento
└── README.md
```

A separação entre **ArduinoIDE** e **VSCode** permite que o mesmo conteúdo seja usado em diferentes ambientes de desenvolvimento, respeitando o perfil e a familiaridade dos participantes.

---

## Aulas e experimentos

| Aula    | Tema                    | Conceitos trabalhados                                               |
| ------- | ----------------------- | ------------------------------------------------------------------- |
| Aula 01 | LED piscante            | Saídas digitais, temporização e estrutura básica de um sketch       |
| Aula 02 | Potenciômetro           | Entradas analógicas, leitura ADC e controle de variáveis            |
| Aula 03 | Lei de Ohm              | Tensão, corrente, resistência, aquisição de dados e gráficos        |
| Aula 04 | Termodinâmica com LM35  | Temperatura, sensores analógicos, calibração e visualização serial  |
| Aula 05 | Cronômetro              | Medidas de tempo, botões, LCD I2C e organização de estados          |
| Aula 06 | Sensor LM35             | Leitura de temperatura, histerese e Plotter Serial                  |
| Aula 07 | Display LCD             | Interface I2C, textos, caracteres customizados e animações          |
| Aula 08 | Sensor Hall             | Movimento circular, RPM, velocidade angular e aceleração centrípeta |
| Aula 09 | Lei de Ohm com Python   | Integração Arduino–Python, coleta automática e análise de dados     |
| Aula 10 | Cronômetro com sensores | Fototransistores, interrupções, Timer1 e medidas de passagem        |

---

## Componentes utilizados

Ao longo das aulas, são utilizados componentes como:

* Arduino Uno ou compatível;
* LEDs e resistores;
* potenciômetro;
* sensor de temperatura LM35;
* display LCD 16×2 com módulo I2C;
* botão tátil;
* sensor Hall;
* fototransistores infravermelhos;
* relé 5 V;
* transistor BC548;
* diodo 1N4007;
* jumpers e protoboard.

A lista exata de componentes pode variar de acordo com a aula e com a adaptação feita pelo professor.

---

## Como usar

### 1. Clonar o repositório

```bash
git clone https://github.com/Parque-da-Ciencia/curso-arduino-parque-da-ciencia.git
cd curso-arduino-parque-da-ciencia
```

### 2. Escolher o ambiente

Para usar com a **Arduino IDE**, acesse:

```text
ArduinoIDE/
```

Para usar com **VS Code** ou outro fluxo de desenvolvimento, acesse:

```text
VSCode/
```

### 3. Abrir a aula desejada

Cada aula possui sua própria pasta, contendo o código principal e, quando necessário, arquivos auxiliares de documentação, scripts ou coleta de dados.

### 4. Enviar o código para o Arduino

Conecte a placa Arduino ao computador, selecione a placa e a porta correta no ambiente de desenvolvimento e faça o upload do código.

---

## Requisitos de software

Para acompanhar o curso, recomenda-se ter instalado:

* [Arduino IDE](https://www.arduino.cc/en/software);
* driver da placa Arduino, quando necessário;
* VS Code, opcionalmente;
* Python 3, para aulas com coleta e processamento de dados;
* bibliotecas específicas indicadas nos códigos de cada aula.

Algumas aulas utilizam bibliotecas adicionais, como suporte a display LCD I2C. Quando necessário, a biblioteca esperada é indicada no cabeçalho do código.

---

## Organização das branches

Este repositório utiliza duas branches principais:

| Branch    | Função                                                           |
| --------- | ---------------------------------------------------------------- |
| `main`    | Versão estável e pública do material                             |
| `develop` | Versão de desenvolvimento e preparação das próximas atualizações |

O fluxo recomendado é desenvolver novas aulas, correções e melhorias em `develop` e enviar para `main` apenas quando o material estiver revisado.

---

## Padrão de documentação

Os códigos do curso seguem um padrão de documentação em português, incluindo:

* cabeçalho institucional;
* descrição do experimento;
* identificação dos componentes esperados;
* comentários explicativos no código;
* documentação de funções em estilo Doxygen quando aplicável.

Esse cuidado busca tornar o material mais acessível para professores, estudantes e colaboradores que desejem estudar, adaptar ou ampliar as atividades.

---

## Sobre o Parque da Ciência Newton Freire Maia

O **Parque da Ciência Newton Freire Maia**, localizado em Pinhais, Paraná, é um espaço dedicado à divulgação científica, à educação e à aproximação entre ciência, tecnologia, cultura e sociedade.

Este repositório faz parte das ações de formação, experimentação e produção de materiais educacionais associados ao uso de tecnologias abertas no ensino de Ciências e Física.

---

## Créditos

**Instituição:** Parque da Ciência Newton Freire Maia
**Curso:** Formação de Professores — Atividades Experimentais de Física com Arduino
**Autor dos códigos originais:** Aron da Rocha Battistella
**GitHub:** [Dom-Aron](https://github.com/Dom-Aron)
**Organização:** [Parque-da-Ciencia](https://github.com/Parque-da-Ciencia)

---

## Contribuições

Contribuições são bem-vindas, especialmente para:

* correção de erros;
* melhoria dos comentários e da documentação;
* adaptação das atividades para diferentes contextos escolares;
* inclusão de novos experimentos;
* criação de materiais de apoio;
* padronização dos códigos e exemplos.

Antes de contribuir, recomenda-se abrir uma issue ou discutir a proposta com os responsáveis pelo repositório.

---

## Licença

A licença deste material deve ser definida pela equipe responsável pelo projeto.

Enquanto isso, recomenda-se não reutilizar o conteúdo em materiais públicos sem citar a origem e a autoria.

---

<p align="center">
  <strong>Parque da Ciência Newton Freire Maia</strong><br>
  Ciência, tecnologia e educação pública em ação.
</p>
