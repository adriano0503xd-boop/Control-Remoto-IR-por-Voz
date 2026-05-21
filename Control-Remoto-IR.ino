#include <Arduino.h>
#include <WiFi.h>

#include <SinricPro.h>
#include <SinricProLight.h>
#include <SinricProWindowAC.h>
#include <SinricProTV.h>

#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <ir_Midea.h>

// =====================================================
// WIFI
// =====================================================
#define WIFI_SSID         "CANALES-1"
#define WIFI_PASS         "70379884Can"

// =====================================================
// SINRIC PRO
// =====================================================
#define APP_KEY           "75ec0de5-298e-4033-83b1-1fcc96fd1102"
#define APP_SECRET        "3af08942-95e1-4e2f-a185-2d6c74d12b99-c82ce60a-3b26-4813-bde6-1f915cf4242b"

// IDs de dispositivos
#define LIGHT_ID          "6a0e45aebaa50bf9bf38ec47"
#define AIR_CONDITIONER_ID "6a0d052ff9b5f15fa7d9732b"
#define TV_ID             "69fef824977a0619a73af6df"


// =====================================================
// PINES
// =====================================================
const uint16_t IR_SEND_PIN = 4;   // Emisor IR
const uint16_t IR_RECV_PIN = 2;   // Receptor IR KY-022

// =====================================================
// OBJETOS IR
// =====================================================
IRsend irsend(IR_SEND_PIN);
IRMideaAC ac(IR_SEND_PIN);
IRrecv irrecv(IR_RECV_PIN, 1024, 50, true);
decode_results results;

// =====================================================
// CÓDIGOS IR - LUCES
// =====================================================
const uint32_t LIGHT_IR_ON   = 0x20DF15EA;
const uint32_t LIGHT_IR_OFF  = 0x20DF11EE;
const uint32_t LIGHT_IR_MAX  = 0x20DFA956;
const uint32_t LIGHT_IR_DOWN = 0x20DFD926;
const uint32_t LIGHT_IR_UP   = 0x20DF9966;

// =====================================================
// CÓDIGOS IR - TV
// =====================================================
const uint32_t TV_POWER        = 0x20DF10EF;
const uint32_t TV_VOLUME_UP    = 0x20DF40BF;
const uint32_t TV_VOLUME_DOWN  = 0x20DFC03F;
const uint32_t TV_CHANNEL_UP   = 0x20DF00FF;
const uint32_t TV_CHANNEL_DOWN = 0x20DF807F;

const uint32_t TV_NETFLIX      = 0x20DF6A95;
const uint32_t TV_OK           = 0x20DF22DD;
const uint32_t TV_UP           = 0x20DF02FD;
const uint32_t TV_DOWN         = 0x20DF827D;
const uint32_t TV_LEFT         = 0x20DFE01F;
const uint32_t TV_RIGHT        = 0x20DF609F;
const uint32_t TV_HOME         = 0x20DF3EC1;
const uint32_t TV_BACK         = 0x20DF14EB;
const uint32_t TV_EXIT         = 0x20DFDA25;

const uint8_t IR_NEC_BITS = 32;

// =====================================================
// ESTADO LOCAL - LUCES
// =====================================================
bool lightState = false;
int lightBrightness = 100;

// Control gradual para 50% y 25%
bool lightSequenceActive = false;
int lightDownStepsPending = 0;
unsigned long lastLightStepTime = 0;
const unsigned long LIGHT_STEP_INTERVAL = 1000; // 1 segundo

// =====================================================
// ESTADO LOCAL - AIRE
// =====================================================
bool acPowerState = false;
uint8_t acTargetTemp = 25;
String acCurrentMode = "AUTO";

// =====================================================
// ESTADO LOCAL - TV
// =====================================================
int tvLastVolume = 10;
int tvLastChannel = 1;

// =====================================================
// WIFI
// =====================================================
void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("[WiFi] Conectando a ");
  Serial.println(WIFI_SSID);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("[WiFi] Conectado");
  Serial.print("[WiFi] IP: ");
  Serial.println(WiFi.localIP());
}

void reconnectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.println("[WiFi] Perdido. Reconectando...");

  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long startAttempt = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WiFi] Reconectado");
    Serial.print("[WiFi] IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[WiFi] No se pudo reconectar");
  }
}

// =====================================================
// FUNCIÓN GENERAL PARA ENVIAR IR NEC
// =====================================================
void sendNECCommand(uint32_t code, const char *label) {
  Serial.print("[IR NEC] Enviando: ");
  Serial.println(label);

  irsend.sendNEC(code, IR_NEC_BITS);
  delay(120);
}

// =====================================================
// CONTROL GRADUAL DE LUCES
// =====================================================
void cancelLightSequence() {
  lightSequenceActive = false;
  lightDownStepsPending = 0;
  lastLightStepTime = 0;
}

void startLightDownSequence(int steps) {
  lightSequenceActive = true;
  lightDownStepsPending = steps;

  // Para que el primer paso se mande casi al instante
  lastLightStepTime = millis() - LIGHT_STEP_INTERVAL;

  Serial.print("[Luz] Secuencia de bajada iniciada. Pasos: ");
  Serial.println(steps);
}

void handleLightSequence() {
  if (!lightSequenceActive) return;
  if (lightDownStepsPending <= 0) {
    cancelLightSequence();
    Serial.println("[Luz] Secuencia terminada");
    return;
  }

  unsigned long currentMillis = millis();

  if (currentMillis - lastLightStepTime >= LIGHT_STEP_INTERVAL) {
    lastLightStepTime = currentMillis;

    sendNECCommand(LIGHT_IR_DOWN, "BAJAR 1 PASO LUZ");

    lightDownStepsPending--;

    Serial.print("[Luz] Pasos restantes: ");
    Serial.println(lightDownStepsPending);

    if (lightDownStepsPending <= 0) {
      cancelLightSequence();
      Serial.println("[Luz] Secuencia terminada");
    }
  }
}

// =====================================================
// LUCES - CALLBACKS
// =====================================================
bool onLightPowerState(const String &deviceId, bool &state) {
  cancelLightSequence();

  Serial.println();
  Serial.println("========== LUZ ==========");
  Serial.printf("[Luz] Estado: %s\n", state ? "ON" : "OFF");

  lightState = state;

  if (lightState) {
    if (lightBrightness == 0) lightBrightness = 100;
    sendNECCommand(LIGHT_IR_ON, "LUZ ON");
  } else {
    lightBrightness = 0;
    sendNECCommand(LIGHT_IR_OFF, "LUZ OFF");
  }

  return true;
}

bool onLightBrightness(const String &deviceId, int &bri) {
  int newBrightness = constrain(bri, 0, 100);

  cancelLightSequence();

  Serial.println();
  Serial.println("========== LUZ BRILLO ==========");
  Serial.printf("[Luz] Brillo recibido: %d%%\n", newBrightness);

  if (newBrightness == 0) {
    lightBrightness = 0;
    lightState = false;
    sendNECCommand(LIGHT_IR_OFF, "LUZ OFF");
  }

  else if (newBrightness >= 95) {
    lightBrightness = 100;
    lightState = true;

    sendNECCommand(LIGHT_IR_MAX, "LUZ MAXIMA");
  }

  else if (newBrightness == 50) {
    lightBrightness = 50;
    lightState = true;

    Serial.println("[Luz] Modo 50%: MAX + bajar 3 pasos");

    sendNECCommand(LIGHT_IR_MAX, "LUZ MAXIMA");
    startLightDownSequence(3);
  }

  else if (newBrightness == 25) {
    lightBrightness = 25;
    lightState = true;

    Serial.println("[Luz] Modo 25%: MAX + bajar 6 pasos");

    sendNECCommand(LIGHT_IR_MAX, "LUZ MAXIMA");
    startLightDownSequence(6);
  }

  else if (newBrightness > lightBrightness) {
    lightBrightness = newBrightness;
    lightState = true;

    sendNECCommand(LIGHT_IR_UP, "SUBIR BRILLO LUZ");
  }

  else if (newBrightness < lightBrightness) {
    lightBrightness = newBrightness;
    lightState = true;

    sendNECCommand(LIGHT_IR_DOWN, "BAJAR BRILLO LUZ");
  }

  else {
    Serial.println("[Luz] El brillo ya estaba en ese valor");
  }

  return true;
}

bool onLightAdjustBrightness(const String &deviceId, int &brightnessDelta) {
  cancelLightSequence();

  Serial.println();
  Serial.println("========== LUZ AJUSTE ==========");
  Serial.printf("[Luz] Ajuste recibido: %+d%%\n", brightnessDelta);

  if (brightnessDelta > 0) {
    lightBrightness = constrain(lightBrightness + 10, 0, 100);
    lightState = true;
    sendNECCommand(LIGHT_IR_UP, "SUBIR 1 PASO LUZ");
  }
  else if (brightnessDelta < 0) {
    lightBrightness = constrain(lightBrightness - 10, 0, 100);

    if (lightBrightness == 0) {
      lightState = false;
      sendNECCommand(LIGHT_IR_OFF, "LUZ OFF");
    } else {
      lightState = true;
      sendNECCommand(LIGHT_IR_DOWN, "BAJAR 1 PASO LUZ");
    }
  }
  else {
    Serial.println("[Luz] No hubo cambio");
  }

  Serial.print("[Luz] Brillo interno: ");
  Serial.print(lightBrightness);
  Serial.println("%");

  brightnessDelta = lightBrightness;
  return true;
}

// =====================================================
// AIRE ACONDICIONADO
// =====================================================
void applyACState() {
  Serial.println();
  Serial.println("========== AIRE ==========");

  if (acPowerState) {
    ac.on();
    Serial.println("[AC] Estado: ON");
  } else {
    ac.off();
    Serial.println("[AC] Estado: OFF");
  }

  ac.setTemp(acTargetTemp);

  if (acCurrentMode == "COOL") {
    ac.setMode(kMideaACCool);
  }
  else if (acCurrentMode == "HEAT") {
    ac.setMode(kMideaACHeat);
  }
  else if (acCurrentMode == "DRY") {
    ac.setMode(kMideaACDry);
  }
  else if (acCurrentMode == "FAN") {
    ac.setMode(kMideaACFan);
  }
  else {
    ac.setMode(kMideaACAuto);
    acCurrentMode = "AUTO";
  }

  ac.setFan(kMideaACFanAuto);

  Serial.print("[AC] Temperatura: ");
  Serial.print(acTargetTemp);
  Serial.println(" C");

  Serial.print("[AC] Modo: ");
  Serial.println(acCurrentMode);

  ac.send();
  delay(120);
}

bool isValidACTemperature(float temperature) {
  return temperature >= 17 && temperature <= 30;
}

bool onACPowerState(const String &deviceId, bool &state) {
  acPowerState = state;

  Serial.printf("[Sinric AC] Power: %s\n", acPowerState ? "ON" : "OFF");

  applyACState();
  return true;
}

bool onACTargetTemperature(const String &deviceId, float &temperature) {
  if (!isValidACTemperature(temperature)) {
    Serial.printf("[Sinric AC] Temperatura fuera de rango: %.1f\n", temperature);
    return false;
  }

  acTargetTemp = (uint8_t) round(temperature);
  acPowerState = true;

  Serial.printf("[Sinric AC] Nueva temperatura: %u C\n", acTargetTemp);

  applyACState();
  return true;
}

bool onACThermostatMode(const String &deviceId, String &mode) {
  mode.toUpperCase();

  if (mode != "COOL" && mode != "HEAT" && mode != "AUTO" &&
      mode != "DRY"  && mode != "FAN") {
    Serial.printf("[Sinric AC] Modo no válido: %s\n", mode.c_str());
    return false;
  }

  acCurrentMode = mode;
  acPowerState = true;

  Serial.printf("[Sinric AC] Nuevo modo: %s\n", acCurrentMode.c_str());

  applyACState();
  return true;
}

// =====================================================
// TV - CALLBACKS BÁSICOS DE ALEXA
// =====================================================
bool onTVPowerState(const String &deviceId, bool &state) {
  Serial.println();
  Serial.println("========== TV ==========");

  Serial.print("[TV] Alexa pidió: ");
  Serial.println(state ? "ENCENDER" : "APAGAR");

  sendNECCommand(TV_POWER, "TV POWER");

  return true;
}

bool onTVVolume(const String &deviceId, int &volume) {
  Serial.println();
  Serial.println("========== TV VOLUMEN ==========");

  Serial.print("[TV] Volumen anterior: ");
  Serial.println(tvLastVolume);

  Serial.print("[TV] Volumen nuevo: ");
  Serial.println(volume);

  if (volume > tvLastVolume) {
    sendNECCommand(TV_VOLUME_UP, "TV VOLUMEN ARRIBA");
  }
  else if (volume < tvLastVolume) {
    sendNECCommand(TV_VOLUME_DOWN, "TV VOLUMEN ABAJO");
  }
  else {
    Serial.println("[TV] El volumen no cambió");
  }

  tvLastVolume = volume;

  return true;
}

bool onTVChangeChannel(const String &deviceId, String &channel) {
  int newChannel = channel.toInt();

  Serial.println();
  Serial.println("========== TV CANAL ==========");

  Serial.print("[TV] Canal recibido: ");
  Serial.println(channel);

  if (newChannel <= 0) {
    Serial.println("[TV] Canal inválido");
    return false;
  }

  if (newChannel > tvLastChannel) {
    sendNECCommand(TV_CHANNEL_UP, "TV CANAL ARRIBA");
  }
  else if (newChannel < tvLastChannel) {
    sendNECCommand(TV_CHANNEL_DOWN, "TV CANAL ABAJO");
  }
  else {
    Serial.println("[TV] El canal no cambió");
  }

  tvLastChannel = newChannel;

  return true;
}

// =====================================================
// MONITOR SERIAL
// =====================================================
void handleSerialCommands() {
  if (!Serial.available()) return;

  String input = Serial.readStringUntil('\n');
  input.trim();
  input.toLowerCase();

  // =====================================================
  // COMANDOS DE LUZ
  // =====================================================
  if (input == "luz on") {
    cancelLightSequence();

    lightState = true;
    if (lightBrightness == 0) lightBrightness = 100;
    sendNECCommand(LIGHT_IR_ON, "LUZ ON");
  }
  else if (input == "luz off") {
    cancelLightSequence();

    lightState = false;
    lightBrightness = 0;
    sendNECCommand(LIGHT_IR_OFF, "LUZ OFF");
  }
  else if (input == "luz max") {
    cancelLightSequence();

    lightBrightness = 100;
    lightState = true;
    sendNECCommand(LIGHT_IR_MAX, "LUZ MAXIMA");
  }
  else if (input == "luz 50") {
    cancelLightSequence();

    lightBrightness = 50;
    lightState = true;

    sendNECCommand(LIGHT_IR_MAX, "LUZ MAXIMA");
    startLightDownSequence(3);
  }
  else if (input == "luz 25") {
    cancelLightSequence();

    lightBrightness = 25;
    lightState = true;

    sendNECCommand(LIGHT_IR_MAX, "LUZ MAXIMA");
    startLightDownSequence(6);
  }
  else if (input == "luz up") {
    cancelLightSequence();

    lightBrightness = constrain(lightBrightness + 10, 0, 100);
    lightState = true;
    sendNECCommand(LIGHT_IR_UP, "SUBIR LUZ");
  }
  else if (input == "luz down") {
    cancelLightSequence();

    lightBrightness = constrain(lightBrightness - 10, 0, 100);

    if (lightBrightness == 0) {
      lightState = false;
      sendNECCommand(LIGHT_IR_OFF, "LUZ OFF");
    } else {
      lightState = true;
      sendNECCommand(LIGHT_IR_DOWN, "BAJAR LUZ");
    }
  }
  else if (input.startsWith("luz bri ")) {
    int value = input.substring(8).toInt();
    int tempBrightness = constrain(value, 0, 100);
    onLightBrightness(LIGHT_ID, tempBrightness);
  }

  // =====================================================
  // COMANDOS DE AIRE
  // =====================================================
  else if (input == "ac on") {
    acPowerState = true;
    acTargetTemp = 25;
    acCurrentMode = "AUTO";
    applyACState();
  }
  else if (input == "ac off") {
    acPowerState = false;
    applyACState();
  }
  else if (input.startsWith("ac temp ")) {
    int t = input.substring(8).toInt();

    if (t >= 17 && t <= 30) {
      acTargetTemp = t;
      acPowerState = true;
      applyACState();
    } else {
      Serial.println("[AC] Temperatura inválida. Usa de 17 a 30");
    }
  }
  else if (input.startsWith("ac mode ")) {
    String m = input.substring(8);
    m.toUpperCase();

    if (m == "COOL" || m == "HEAT" || m == "AUTO" || m == "DRY" || m == "FAN") {
      acCurrentMode = m;
      acPowerState = true;
      applyACState();
    } else {
      Serial.println("[AC] Modo inválido. Usa: cool, heat, auto, dry, fan");
    }
  }

  // =====================================================
  // COMANDOS DE TV
  // =====================================================
  else if (input == "tv power") {
    sendNECCommand(TV_POWER, "TV POWER");
  }
  else if (input == "tv volup") {
    sendNECCommand(TV_VOLUME_UP, "TV VOLUMEN ARRIBA");
  }
  else if (input == "tv voldown") {
    sendNECCommand(TV_VOLUME_DOWN, "TV VOLUMEN ABAJO");
  }
  else if (input == "tv chup") {
    sendNECCommand(TV_CHANNEL_UP, "TV CANAL ARRIBA");
  }
  else if (input == "tv chdown") {
    sendNECCommand(TV_CHANNEL_DOWN, "TV CANAL ABAJO");
  }
  else if (input == "tv netflix") {
    sendNECCommand(TV_NETFLIX, "TV NETFLIX");
  }
  else if (input == "tv ok") {
    sendNECCommand(TV_OK, "TV OK");
  }
  else if (input == "tv up") {
    sendNECCommand(TV_UP, "TV ARRIBA");
  }
  else if (input == "tv down") {
    sendNECCommand(TV_DOWN, "TV ABAJO");
  }
  else if (input == "tv left") {
    sendNECCommand(TV_LEFT, "TV IZQUIERDA");
  }
  else if (input == "tv right") {
    sendNECCommand(TV_RIGHT, "TV DERECHA");
  }
  else if (input == "tv home") {
    sendNECCommand(TV_HOME, "TV PANTALLA PRINCIPAL");
  }
  else if (input == "tv back") {
    sendNECCommand(TV_BACK, "TV REGRESAR");
  }
  else if (input == "tv exit") {
    sendNECCommand(TV_EXIT, "TV EXIT");
  }

  // =====================================================
  // COMANDO NO VÁLIDO
  // =====================================================
  else {
    Serial.println();
    Serial.println("Comando no válido.");
    Serial.println("Usa estos comandos:");
    Serial.println();

    Serial.println("LUCES:");
    Serial.println("  luz on");
    Serial.println("  luz off");
    Serial.println("  luz max");
    Serial.println("  luz 50");
    Serial.println("  luz 25");
    Serial.println("  luz up");
    Serial.println("  luz down");
    Serial.println("  luz bri 50");
    Serial.println("  luz bri 25");
    Serial.println();

    Serial.println("AIRE:");
    Serial.println("  ac on");
    Serial.println("  ac off");
    Serial.println("  ac temp 24");
    Serial.println("  ac mode cool");
    Serial.println("  ac mode heat");
    Serial.println("  ac mode auto");
    Serial.println("  ac mode dry");
    Serial.println("  ac mode fan");
    Serial.println();

    Serial.println("TV:");
    Serial.println("  tv power");
    Serial.println("  tv volup");
    Serial.println("  tv voldown");
    Serial.println("  tv chup");
    Serial.println("  tv chdown");
    Serial.println("  tv netflix");
    Serial.println("  tv ok");
    Serial.println("  tv up");
    Serial.println("  tv down");
    Serial.println("  tv left");
    Serial.println("  tv right");
    Serial.println("  tv home");
    Serial.println("  tv back");
    Serial.println("  tv exit");
  }
}

// =====================================================
// RECEPTOR IR OPCIONAL
// =====================================================
void handleIRReceiver() {
  if (irrecv.decode(&results)) {
    Serial.println();
    Serial.println("========== IR RECIBIDO ==========");

    Serial.print("Protocolo: ");
    Serial.println(typeToString(results.decode_type));

    Serial.print("Código HEX: 0x");
    serialPrintUint64(results.value, HEX);
    Serial.println();

    Serial.print("Bits: ");
    Serial.println(results.bits);

    if (results.decode_type == MIDEA) {
      Serial.println("[IR RX] Se detectó una trama MIDEA");
    }

    irrecv.resume();
  }
}

// =====================================================
// SETUP SINRICPRO
// =====================================================
void setupSinricPro() {
  // ---------------------
  // LUZ
  // ---------------------
  SinricProLight &myLight = SinricPro[LIGHT_ID];

  myLight.onPowerState(onLightPowerState);
  myLight.onBrightness(onLightBrightness);
  myLight.onAdjustBrightness(onLightAdjustBrightness);

  // ---------------------
  // AIRE
  // ---------------------
  SinricProWindowAC &myAC = SinricPro[AIR_CONDITIONER_ID];

  myAC.onPowerState(onACPowerState);
  myAC.onTargetTemperature(onACTargetTemperature);
  myAC.onThermostatMode(onACThermostatMode);

  // ---------------------
  // TV
  // ---------------------
  SinricProTV &myTV = SinricPro[TV_ID];

  myTV.onPowerState(onTVPowerState);
  myTV.onSetVolume(onTVVolume);
  myTV.onChangeChannel(onTVChangeChannel);

  // ---------------------
  // EVENTOS SINRIC
  // ---------------------
  SinricPro.onConnected([]() {
    Serial.println();
    Serial.println("[SinricPro] CONECTADO");
  });

  SinricPro.onDisconnected([]() {
    Serial.println();
    Serial.println("[SinricPro] DESCONECTADO");
  });

  SinricPro.restoreDeviceStates(true);
  SinricPro.begin(APP_KEY, APP_SECRET);

  Serial.println("[SinricPro] Configuración completa");
}

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("==============================================");
  Serial.println(" ESP32 + SINRICPRO + LUZ + AIRE + TV + IR");
  Serial.println("==============================================");

  irsend.begin();
  ac.begin();
  irrecv.enableIRIn();

  acPowerState = false;
  acTargetTemp = 25;
  acCurrentMode = "AUTO";

  setupWiFi();
  setupSinricPro();

  Serial.println();
  Serial.println("Sistema listo.");
  Serial.println();

  Serial.println("Comandos Alexa sugeridos:");
  Serial.println("  Alexa, prende luces");
  Serial.println("  Alexa, apaga luces");
  Serial.println("  Alexa, pon luces al 100 por ciento");
  Serial.println("  Alexa, pon luces al 50 por ciento");
  Serial.println("  Alexa, pon luces al 25 por ciento");
  Serial.println("  Alexa, prende aire");
  Serial.println("  Alexa, apaga aire");
  Serial.println("  Alexa, pon aire en 24 grados");
  Serial.println("  Alexa, prende televisor");
  Serial.println("  Alexa, apaga televisor");
  Serial.println("  Alexa, sube volumen del televisor");
  Serial.println();

  Serial.println("Comandos por Monitor Serial:");
  Serial.println("  luz on / luz off / luz max / luz 50 / luz 25");
  Serial.println("  luz up / luz down / luz bri 50 / luz bri 25");
  Serial.println("  ac on / ac off / ac temp 24 / ac mode cool");
  Serial.println("  tv power / tv volup / tv voldown / tv chup / tv chdown");
  Serial.println("  tv netflix / tv ok / tv up / tv down / tv left / tv right");
  Serial.println("  tv home / tv back / tv exit");
  Serial.println("==============================================");
}

// =====================================================
// LOOP
// =====================================================
void loop() {
  SinricPro.handle();

  reconnectWiFi();

  handleSerialCommands();

  handleIRReceiver();

  handleLightSequence();
}
