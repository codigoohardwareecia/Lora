/*
  Heltec Lora32

  Driver Sistema operacional: 
  https://www.silabs.com/software-and-tools/usb-to-uart-bridge-vcp-drivers?tab=downloads (CP210x Universal Windows Driver)
  
  Url da Board:
  https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/releases/download/0.0.9/package_heltec_esp32_index.json

  Instalação suporta da Board:
  Procure por Heltec ESP32 Series Dev Board

  Selecionar o modelo
  WiFi LoRa 32 (V2)

  Bibliotecas
  Lora: Instale a biblioteca Lora by Sandeep Mistry
  SSD1306Wire: Instale a SSD1306Wire by Alexander Pronin by Alexander Pronin
  ESP8266 and ESP32 OLED: Instale a "ESP8266 and ESP32 OLED" by ThingPulse Fabrice
*/


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
