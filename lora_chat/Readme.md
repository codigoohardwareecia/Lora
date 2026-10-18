#### Requisitos

- Arduino IDE
- 2 unidades de Lora32 da Heltec
- Para os testes vai precisar de dois celulares ou notebook para se conectar ao Wifi do Lora
#### 1. Adicionar a URL da Heltec às Preferências

1. Abra o **Arduino IDE**.    
2. Vá em **Arquivo > Preferências** (no macOS: _Arduino IDE > Settings_).    
3. No campo **URLs Adicionais do Gerenciador de Placas**, cole a seguinte URL: [https://resource.heltec.cn/download/package_heltec_esp32_index.json] 
	(Se já houver outra URL, separe-as com uma vírgula).    
4. Clique em **OK**.
  
#### 2. Instalar o Pacote de Placas Heltec

1. No menu lateral esquerdo, clique no ícone do **Gerenciador de Placas** (ou vá em _Ferramentas > Placa > Gerenciador de Placas_).    
2. Na barra de pesquisa, digite **Heltec ESP32 Series Dev-boards**.    
3. Localize o pacote oficial da **Heltec Automation** e clique em **Instalar**.    
4. Aguarde a conclusão do download e da instalação.

#### 3. Instalar o driver para Windows
1. Nos meus testes o meu Lora deu erro de driver, caso ocorra, e existe uma grande probabilidade acesse o link a seguir:  https://www.silabs.com/software-and-tools/usb-to-uart-bridge-vcp-drivers?tab=downloads
2. Em "Software Downloads" procure por CP210x Universal Windows Driver e faça o download
3. Descompacte o arquivo zip, nele contém os drivers
4. No Windows vá em Gerenciamento do Computador > Device Manager
5. Localize o driver com o nome semelhante a "CP210x Universal Windows Driver" e clique com o botâo direito e seleciona Update Driver
6. Clique em "Procurar os drivers no meu computador", aponte o diretório onde vc descompactou os drivers, e clique para continuar.
7. Nesta etapa o driver deve ser instalado sem nenhum alerta.   
#### 4. Selecionar a Placa no Arduino IDE

1. Vá ao menu **Ferramentas > Placa > Heltec ESP32 Series Dev-boards**.    
2. Na lista, selecione **WiFi LoRa 32 (V2)**.
3. Vá em Ports e selecione a porta correspondete, pode aparecer várias portas, nesce caso tem que ir tentando, selecione uma.
4. Vamos mandar um código vazio apenas como ex:
```C++
	void setup() {
	
	}
	
	void loop() {

	}
```
5. Clique no icone "Upload", logo abaixo em "Output deverá aparecer informações do Upload"
6. Se der erro, troque a porta até conseguir sucesso.

#### 5. Bibliotecas

1. Será necessário instalar a biblioteca Lora, clique em Tools > Manager Libraries
2. Na caixa de busca procure por "Lora"
3. Selecione "Lora by Sandeep Mistry"
4. Clique Install
5. Procure por "SSD1306Wire by Alexander Pronin" clique em Install.
6. Procure por "ESP8266 and ESP32 OLED by ThingPulse Fabrice" clique em Install 
#### 5. O código

1. Copie o código abaixo e cole no editor do Arduino
2. Clique em "Upload" para fazer o envio
3. Conecte o segundo Lora
4. Altere a variável "ssidAP" para outro nome, os nomes não podem ser iguais paa evitar confusão
5. Clique em "Upload" novamente para fazer o envio

````C++
#include <WiFi.h>
#include <WebServer.h>
#include <LoRa.h>
#include <SPI.h>
#include "SSD1306Wire.h"

// --- PINOS HELTEC V2 ---
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26
#define OLED_RST 16

// --- CONFIGURAÇÃO DO WIFI (CADA PLACA UMA REDE DIFERENTE) ---
const char* ssidAP = "Jarvis_Link_B"; // Mude para Link_B na outra placa
const char* passwordAP = "12345678";

WebServer server(80);
SSD1306Wire display(0x3c, 4, 15);
String chatHistory = "";

// Função para atualizar o visor OLED
void atualizarDisplay(String titulo, String msg, bool alerta = false) {
  display.clear();
  display.drawString(0, 0, titulo);
  display.drawString(0, 16, msg);
  display.display();
}

// Página Web Principal
void handleRoot() {
  String html = "<html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Jarvis LoRa Terminal</title>";
  // Script AJAX: Busca atualizações do chat a cada 2 segundos sem recarregar a página
  html += "<script>setInterval(function(){ fetch('/update').then(r=>r.text()).then(t=>{ if(t!==''){document.getElementById('log').innerHTML=t;} }); }, 2000);</script>";
  html += "<style>body{background:#000; color:#0f0; font-family:monospace; padding:20px;} ";
  html += "#log{border:1px solid #0f0; padding:15px; height:300px; overflow-y:scroll; background:#050505; margin-bottom:20px;} ";
  html += "input{background:#000; color:#0f0; border:1px solid #0f0; padding:10px; width:70%;} ";
  html += "button{padding:10px; background:#0f0; color:#000; border:none; cursor:pointer; font-weight:bold;}</style></head><body>";
  
  html += "<h2>> JARVIS_LORA_SYSTEM v2.0</h2>";
  html += "<div id='log'>" + chatHistory + "</div>";
  
  // O formulário envia para /send e o servidor redireciona de volta
  html += "<form action='/send' method='GET' autocomplete='off'>";
  html += "<span>> </span><input type='text' name='text' autofocus> <button type='submit'>TRANSMITIR</button>";
  html += "</form></body></html>";
  
  server.send(200, "text/html", html);
}

// Retorna apenas o conteúdo do chat para o AJAX
void handleUpdate() {
  server.send(200, "text/plain", chatHistory);
}

// Processa o envio e redireciona para a home (evita a tela de "OK")
void handleSend() {
  String text = server.arg("text");
  if (text.length() > 0) {
    LoRa.beginPacket();
    LoRa.print(text);
    LoRa.endPacket();
    
    chatHistory += "<span style='color:#888;'>[EU]: " + text + "</span><br>";
    atualizarDisplay("ENVIANDO...", text);
  }
  // Redirecionamento HTTP 303 para voltar à página principal imediatamente
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(115200);

  // Reset Físico do Display OLED
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW); delay(50); digitalWrite(OLED_RST, HIGH);

  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  atualizarDisplay("SISTEMA", "Iniciando modo AP...");

  // Configura como Access Point (Roteador Próprio)
  WiFi.softAP(ssidAP, passwordAP);
  IPAddress IP = WiFi.softAPIP();

  // Inicializa Rádio LoRa
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(915E6)) {
    atualizarDisplay("ERRO", "Falha no Chip LoRa!");
    while (1);
  }
  LoRa.setSyncWord(0xF3);

  // Rotas do Servidor
  server.on("/", handleRoot);
  server.on("/update", handleUpdate);
  server.on("/send", handleSend);
  server.begin();

  atualizarDisplay("ONLINE", "Conecte em: " + String(ssidAP) + "\nIP: " + IP.toString());
}

void loop() {
  server.handleClient();

  // Verifica recebimento LoRa
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String message = "";
    while (LoRa.available()) {
      message += (char)LoRa.read();
    }
    
    // Adiciona ao histórico do chat
    chatHistory += "<span style='color:#0f0;'>[REMOTO]: " + message + "</span><br>";
    
    // Alerta no display da placa
    atualizarDisplay("RECEBIDO!", message, true);
    delay(1500);
    atualizarDisplay("AGUARDANDO...", "WiFi: " + String(ssidAP));
  }
}
````

#### 5. Como testar

1. Ligue os dois Loras
2. Em um celular ou notebook se conecte ao primeiro Lora como se fosse uma conexão Wifi normal usando o SSID do primeiro Lora e a senha "12345678"
3. No segundo celular ou notebook faça o mesmo se conectando ao segundo Lora
4. Em cada Lora será exibido o endereçco IP
5. Abra o navegador e digite http://{ endreço exibido no lora}
6. Vai abrir uma página no nvageador com um campo
7. Digite a mensagem, o outro lora deve receber a mensagem
8. Faça o mesmo no outro lora, a mensagem deverá chegar ao lora correspondente