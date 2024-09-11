#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <time.h>
#include <ETH.h>
#include <driver/ledc.h>

// Configuración de red
const char* ssid = "Totalplay-AFAA";
const char* password = "AFAA95E4XAedzyGM";

// Configuración del tiempo
const long utcOffsetInSeconds = -6 * 3600;
struct tm timeinfo;

// Pines de LEDs y sensores
const int ledPins[4] = {14, 17, 33, 5};  // Pines para los LEDs
const int sensorPins[4] = {2, 4, 12, 15};  // Pines para los sensores
volatile bool sensorTriggered[4] = {false, false, false, false};  // Flags para saber si el sensor fue activado
// Variable global para identificar el índice del sensor activado
volatile int activeSensorIndex = -1;

//-----------------------------------------------------------Variables para Servidor-------------------------------------------------------------
unsigned long sensorTriggerTimes[4] = {0}; // Almacena el tiempo cuando se activó cada sensor
bool ledStates[4] = {false}; // Estado de los LEDs: encendido o apagado
int mode[4], intensity[4], trigger[4], ledDuration[4], pwm_delay[4];
const int pwmFrequency = 1000; // Frecuencia PWM en Hz
const int pwmResolution = 8; // Resolución PWM en bits

//-------------------------------------------------------------Pagina HTML-----------------------------------------------------------
const char* htmlPage = R"rawliteral(
<!DOCTYPE html><html lang="es"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Interfaz de Control - CL-IR-A01 Helio</title><style>body{font-family:Times New Roman,Times,serif;margin:20px;background-color:#f4f4f4}h1{color:#000;text-align:center;font-size:3em}.section{margin-bottom:30px;padding:20px;background:#fff;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,.1)}.button{background-color:#600822;color:#fff;padding:10px 20px;border:none;border-radius:5px;cursor:pointer;margin:5px}.button:hover{background-color:#600822}.data-display{font-size:1.2em;color:#000}h2{text-align:center;font-size:1.9em}h3{text-align:left;font-size:1.5em}.rectangle-text{font-size:1.2em;display:flex;justify-content:space-around;width:100%}a{color:#600822;text-decoration:none;padding:20px;display:inline-block}a:hover{color:#1727b3;text-decoration:underline}.channel-box{display:inline-block;background-color:#600822;color:#fff;padding:10px;border-radius:5px;text-align:center;margin:5px;cursor:default}</style></head><body><div class="section"><h1>Bienvenido al centro de control de tu dispositivo CL-IR-A01 Helio</h1><div class="rectangle-text"><span><a href="#control">Control de luminarias</a></span><span><a href="#estado">Estado actual</a></span><span><a href="#diag">Diagnóstico/Recuperación</a></span></div></div><div class="section"><h2 id="control">Control de luminarias</h2><h3>Canal 1</h3><label for="channel1">Modo:</label><select id="channel1" name="channel1" onchange="updateDisplay()"><option value="1">Continuo</option><option value="2">Pulsado</option><option value="3">Conmutado</option></select><br><label for="intensity1">Intensidad (%):</label><input type="number" id="intensity1" name="intensity1" min="0" max="100" onchange="updateDisplay()"><br><label for="time-on1">Tiempo de encendido (ms):</label><input type="number" id="time-on1" name="time-on1" min="0" max="3600000" onchange="updateDisplay()"><br><label for="trigger1">No. Trigger:</label><select id="trigger1" name="trigger1" onchange="updateDisplay()"><option value="1">1</option><option value="2">2</option><option value="3">3</option><option value="4">4</option></select><br><label for="pwm-delay1">Retardo del pulso (ms):</label><input type="number" id="pwm-delay1" name="pwm-delay1" min="0" max="1000000" onchange="updateDisplay()"><br><br><h3>Canal 2</h3><br><label for="channel2">Modo:</label><select id="channel2" name="channel2" onchange="updateDisplay()"><option value="1">Continuo</option><option value="2">Pulsado</option><option value="3">Conmutado</option></select><br><label for="intensity2">Intensidad (%):</label><input type="number" id="intensity2" name="intensity2" min="0" max="100" onchange="updateDisplay()"><br><label for="time-on2">Tiempo de encendido (ms):</label><input type="number" id="time-on2" name="time-on2" min="0" max="3600000" onchange="updateDisplay()"><br><label for="trigger2">No. Trigger:</label><select id="trigger2" name="trigger2" onchange="updateDisplay()"><option value="1">1</option><option value="2">2</option><option value="3">3</option><option value="4">4</option></select><br><label for="pwm-delay2">Retardo del pulso (ms):</label><input type="number" id="pwm-delay2" name="pwm-delay2" min="0" max="1000000" onchange="updateDisplay()"><br><br><h3>Canal 3</h3><br><label for="channel3">Modo:</label><select id="channel3" name="channel3" onchange="updateDisplay()"><option value="1">Continuo</option><option value="2">Pulsado</option><option value="3">Conmutado</option></select><br><label for="intensity3">Intensidad (%):</label><input type="number" id="intensity3" name="intensity3" min="0" max="100" onchange="updateDisplay()"><br><label for="time-on3">Tiempo de encendido (ms):</label><input type="number" id="time-on3" name="time-on3" min="0" max="3600000" onchange="updateDisplay()"><br><label for="trigger3">No. Trigger:</label><select id="trigger3" name="trigger3" onchange="updateDisplay()"><option value="1">1</option><option value="2">2</option><option value="3">3</option><option value="4">4</option></select><br><label for="pwm-delay3">Retardo del pulso (ms):</label><input type="number" id="pwm-delay3" name="pwm-delay3" min="0" max="1000000" onchange="updateDisplay()"><br><br><h3>Canal 4</h3><br><label for="channel4">Modo:</label><select id="channel4" name="channel4" onchange="updateDisplay()"><option value="1">Continuo</option><option value="2">Pulsado</option><option value="3">Conmutado</option></select><br><label for="intensity4">Intensidad (%):</label><input type="number" id="intensity4" name="intensity4" min="0" max="100" onchange="updateDisplay()"><br><label for="time-on4">Tiempo de encendido (ms):</label><input type="number" id="time-on4" name="time-on4" min="0" max="3600000" onchange="updateDisplay()"><br><label for="trigger4">No. Trigger:</label><select id="trigger4" name="trigger4" onchange="updateDisplay()"><option value="1">1</option><option value="2">2</option><option value="3">3</option><option value="4">4</option></select><br><label for="pwm-delay4">Retardo del pulso (ms):</label><input type="number" id="pwm-delay4" name="pwm-delay4" min="0" max="1000000" onchange="updateDisplay()"><br><br><div style="text-align:center"><button class="button" onclick="saveConfiguration()">Guardar</button></div></div><div class="section"><h2 id="estado">Estado actual</h2><p class="status">Operativo</p><p>Última verificación:<span id="last-check">--:--</span></p><h3>Canal 1</h3><div class="config-display"><label for="display-mode1">Modo:</label><span id="display-mode1">-</span><br><label for="display-intensity1">Intensidad:</label><span id="display-intensity1">-</span><br><label for="display-time-on1">Tiempo de encendido:</label><span id="display-time-on1">-</span><br><label for="display-trigger1">No. Trigger:</label><span id="display-trigger1">-</span><br><label for="display-pwm-delay1">Retardo del pulso:</label><span id="display-pwm-delay1">-</span><br></div><h3>Canal 2</h3><div class="config-display"><label for="display-mode2">Modo:</label><span id="display-mode2">-</span><br><label for="display-intensity2">Intensidad:</label><span id="display-intensity2">-</span><br><label for="display-time-on2">Tiempo de encendido:</label><span id="display-time-on2">-</span><br><label for="display-trigger2">No. Trigger:</label><span id="display-trigger2">-</span><br><label for="display-pwm-delay2">Retardo del pulso:</label><span id="display-pwm-delay2">-</span><br></div><h3>Canal 3</h3><div class="config-display"><label for="display-mode3">Modo:</label><span id="display-mode3">-</span><br><label for="display-intensity3">Intensidad:</label><span id="display-intensity3">-</span><br><label for="display-time-on3">Tiempo de encendido:</label><span id="display-time-on3">-</span><br><label for="display-trigger3">No. Trigger:</label><span id="display-trigger3">-</span><br><label for="display-pwm-delay3">Retardo del pulso:</label><span id="display-pwm-delay3">-</span><br></div><h3>Canal 4</h3><div class="config-display"><label for="display-mode4">Modo:</label><span id="display-mode4">-</span><br><label for="display-intensity4">Intensidad:</label><span id="display-intensity4">-</span><br><label for="display-time-on4">Tiempo de encendido:</label><span id="display-time-on4">-</span><br><label for="display-trigger4">No. Trigger:</label><span id="display-trigger4">-</span><br><label for="display-pwm-delay4">Retardo del pulso:</label><span id="display-pwm-delay4">-</span><br></div><div style="text-align:center"><button class="button" onclick="modifyConfiguration()">Modificar</button></div></div><div class="section"><h2 id="diag">Diagnóstico y recuperación</h2><h3>Recuperación de IP</h3><p>Dirección IP actual:<span id="current-ip"> </span></p><button class="button" onclick="recoverIP()">Recuperar IP</button><h3>Reiniciar el Sistema</h3><p>Si el sistema ha quedado inoperable, intenta un reinicio.</p><button class="button" onclick="restartSystem()">Reiniciar</button><h3>Registro de Errores</h3><p>Consulta los errores recientes para más detalles.</p><textarea id="error-log" rows="10" cols="50" readonly="readonly">
        </textarea><script>function updateDisplay() {
          for (let i = 1; i <= 4; i++) {
            // Obtener los valores de los campos de entrada para cada canal
            let mode = document.getElementById(`channel${i}`).value;
            let intensity = document.getElementById(`intensity${i}`).value;
            let timeOn = document.getElementById(`time-on${i}`).value;
            let trigger = document.getElementById(`trigger${i}`).value;
            let pwmDelay = document.getElementById(`pwm-delay${i}`).value;

            // Actualizar la sección de visualización para cada canal
            document.getElementById(`display-mode${i}`).textContent = ` ${mode}`;
            document.getElementById(`display-intensity${i}`).textContent = ` ${intensity}%`;
            document.getElementById(`display-time-on${i}`).textContent = ` ${timeOn} milisegundos`;
            document.getElementById(`display-trigger${i}`).textContent = `${trigger}`;
            document.getElementById(`display-pwm-delay${i}`).textContent = ` ${pwmDelay} milisegundos`;
          }
	      }
      // Llamar a updateDisplay al cargar la página para mostrar los valores iniciales
      window.onload = updateDisplay;

      function saveConfiguration() {
        let url = '/Control?';
        url += 'channel1=' + encodeURIComponent(document.getElementById('channel1').value);
        url += '&intensity1=' + encodeURIComponent(document.getElementById('intensity1').value);
        url += '&time_on1=' + encodeURIComponent(document.getElementById('time-on1').value);
        url += '&trigger1=' + encodeURIComponent(document.getElementById('trigger1').value);
        url += '&pwm_delay1=' + encodeURIComponent(document.getElementById('pwm-delay1').value);

        url += '&channel2=' + encodeURIComponent(document.getElementById('channel2').value);
        url += '&intensity2=' + encodeURIComponent(document.getElementById('intensity2').value);
        url += '&time_on2=' + encodeURIComponent(document.getElementById('time-on2').value);
        url += '&trigger2=' + encodeURIComponent(document.getElementById('trigger2').value);
        url += '&pwm_delay2=' + encodeURIComponent(document.getElementById('pwm-delay2').value);

        url += '&channel3=' + encodeURIComponent(document.getElementById('channel3').value);
        url += '&intensity3=' + encodeURIComponent(document.getElementById('intensity3').value);
        url += '&time_on3=' + encodeURIComponent(document.getElementById('time-on3').value);
        url += '&trigger3=' + encodeURIComponent(document.getElementById('trigger3').value);
        url += '&pwm_delay3=' + encodeURIComponent(document.getElementById('pwm-delay3').value);

        url += '&channel4=' + encodeURIComponent(document.getElementById('channel4').value);
        url += '&intensity4=' + encodeURIComponent(document.getElementById('intensity4').value);
        url += '&time_on4=' + encodeURIComponent(document.getElementById('time-on4').value);
        url += '&trigger4=' + encodeURIComponent(document.getElementById('trigger4').value);
        url += '&pwm_delay4=' + encodeURIComponent(document.getElementById('pwm-delay4').value);
        fetch(url)
          .then(response => response.text())
          .then(data => {
              console.log('Guardado');
              alert("Configuración guardada");
          })
          .catch(error => {
              console.error('Error:', error);
          });
      }
      function modifyConfiguration() {
        document.getElementById('control').scrollIntoView();
        console.log('Modify configuration');
      }
      // Función para recuperar la IP manualmente cuando se hace clic en el botón
      function recoverIP() {
        fetch('/recover_ip')
          .then(response => response.json())
          .then(data => {
            document.getElementById('current-ip').textContent = data.ip;
          })
          .catch(error => console.error('Error al recuperar la IP:', error));
      }
      function restartSystem() {
        console.log('Restart system');
        // Enviar una solicitud al servidor para reiniciar el sistema
        fetch('/restart')
          .then(response => response.text())
          .then(data => {
              console.log('Respuesta del servidor:', data);
              alert('El sistema se está reiniciando...');
          })
          .catch(error => {
              console.error('Error:', error);
          });
      }
      function updateDiagnostics() {
        fetch('/diagnostics')
          .then(response => response.text())
          .then(data => {
            document.getElementById('error-log').value = data;
          })
          .catch(error => console.error('Error fetching diagnostics:', error));
      }
      // Actualizar cada 10 segundos
      window.onload = function() {
        updateDiagnostics();  // Actualizar al cargar la página
        setInterval(updateDiagnostics, 10000); // Actualizar cada 10 segundos
      };
      // Actualizar la última verificación
      document.getElementById('last-check').textContent = new Date().toLocaleTimeString();</script></body></html>
)rawliteral";
//---------------------------------------------Funciones externas para recibir lo del HTML-------------------------------------------------------
// Función para manejar la ruta /Control
void handleControl(AsyncWebServerRequest *request) {
  String response;
  bool uniqueTriggers = true;
  bool triggersUsed[4] = { false, false, false, false };

  for (int i = 0; i < 4; i++) {
    // Procesar el modo
    if (request->hasParam("channel" + String(i + 1))) {
      mode[i] = request->getParam("channel" + String(i + 1))->value().toInt();
      response += "Canal " + String(i + 1) + " - Modo: " + String(mode[i]) + "\n";
    }

    // Procesar intensidad
    String paramName = "intensity" + String(i + 1);
    if (request->hasParam(paramName)) {
      int intensityValue = request->getParam(paramName)->value().toInt();
      intensity[i] = intensityValue;
      response += "Intensidad del canal " + String(i + 1) + ": " + String(intensityValue) + "\n";
    } else {
      response += "No se recibió valor para intensidad del canal " + String(i + 1) + "\n";
    }

    // Procesar tiempo de encendido
    if (request->hasParam("time-on" + String(i + 1))) {
      ledDuration[i] = request->getParam("time-on" + String(i + 1))->value().toInt() * 1000; // Convertir a milisegundos
      response += "Tiempo de Encendido: " + String(ledDuration[i]) + " ms\n";
    } else {
      response += "No se recibió valor para tiempo de encendido del canal " + String(i + 1) + "\n";
    }

    // Procesar trigger
    if (request->hasParam("trigger" + String(i + 1))) {
      trigger[i] = request->getParam("trigger" + String(i + 1))->value().toInt();
      response += "Trigger: " + String(trigger[i]) + "\n";
      triggersUsed[trigger[i] - 1] = true; // Marcar trigger como usado
    } else {
      response += "No se recibió valor para trigger del canal " + String(i + 1) + "\n";
    }

    // Procesar delay PWM
    if (request->hasParam("pwm-delay" + String(i + 1))) {
      pwm_delay[i] = request->getParam("pwm-delay" + String(i + 1))->value().toInt() * 1000; // Convertir a milisegundos
      response += "PWM Delay: " + String(pwm_delay[i]) + "\n";
    } else {
      response += "No se recibió valor para PWM Delay del canal " + String(i + 1) + "\n";
    }
  }

  // Enviar la respuesta al cliente
  request->send(200, "text/plain", response);
}

void handleRecoverIP(AsyncWebServerRequest *request) {
  String ip = WiFi.localIP().toString();
  String jsonResponse = "{\"ip\": \"" + ip + "\"}";
  request->send(200, "application/json", jsonResponse);  // Enviar respuesta en formato JSON
}

// Función para manejar el reinicio del sistema
void handleRestart(AsyncWebServerRequest *request) {
  request->send(200, "text/html", "Reiniciando...");
  delay(1000);  // Esperar un segundo antes de reiniciar
  ESP.restart();  // Reiniciar el ESP32
}
String performDiagnostics() {
  String diagnostics = "Errores:";

  // Diagnóstico de sensores
  for (int i = 0; i < 4; i++) {
    if (digitalRead(sensorPins[i]) != HIGH) {
      diagnostics += "\nError en el sensor " + String(i + 1);
    }
  }

  // Diagnóstico de LEDs
  for (int i = 0; i < 4; i++) {
    if (digitalRead(ledPins[i]) != HIGH) {
      diagnostics += "\nError en el LED " + String(i + 1);
    }
  }
  return diagnostics;
}

void handleDiagnostics(AsyncWebServerRequest *request) {
  String diagnostics = performDiagnostics();
  request->send(200, "text/plain", diagnostics);
}

AsyncWebServer server(80);
//---------------------------------------------------------Funciones externas para el circuito----------------------------------------------

// Función de interrupción genérica
void IRAM_ATTR handleSensorInterrupt() {
  if (activeSensorIndex >= 0 && activeSensorIndex < 4) {
    sensorTriggered[activeSensorIndex] = true;
  }
}
// Configuración de pines
void setupPins() {
  for (int i = 0; i < 4; i++) {
    pinMode(sensorPins[i], INPUT);   // Configurar pines de sensores como entradas
    pinMode(ledPins[i], OUTPUT);     // Configurar pines de LEDs como salidas
  }
}
// Asignar interrupciones a los pines de sensores
void setupInterrupts() {
  for (int i = 0; i < 4; i++) {
    activeSensorIndex = i; // Asignar el índice del sensor activo
    attachInterrupt(digitalPinToInterrupt(sensorPins[i]), handleSensorInterrupt, RISING);
  }
}
void handleContinuousMode(int triggerNum, int pines, int channel) {
  // Convertir el porcentaje a valor PWM
  int pwmValue = map(intensity[channel], 0, 100, 0, 255);
  if (pwmValue > 0) {
    ledcWrite(pines, pwmValue);
  } else {
    Serial.println("Intensidad inválida o 0, revisa los valores de intensidad");
  }

  // Imprimir valores de depuración
  Serial.print("Modo continuo - Led: ");
  Serial.print(pines);
  Serial.print(" Trigger: ");
  Serial.print(triggerNum);
  Serial.print(" - Canal: ");
  Serial.print(channel + 1);
  Serial.print(" - Intensidad PWM: ");
  Serial.println(pwmValue);
}
void handlePulsedMode(int triggerNum, int pines, int channel) {
  unsigned long currentMillis = millis();
  static unsigned long previousMillis[4] = {0}; // Guardar tiempos anteriores
  static bool ledState[4] = {false}; // Guardar el estado actual del LED

  // Verificar si el LED está encendido
  if (ledState[channel]) {
    // El LED está encendido, revisar si es tiempo de apagarlo
    if (currentMillis - previousMillis[channel] >= ledDuration[channel]) {
      ledcWrite(pines, 0); // Apagar LED
      ledState[channel] = false; // Actualizar estado del LED
      previousMillis[channel] = currentMillis; // Reiniciar el tiempo
    }
  } else {
    // El LED está apagado, revisar si es tiempo de encenderlo
    if (currentMillis - previousMillis[channel] >= pwm_delay[channel]) {
      int pwmValue = map(intensity[channel], 0, 100, 0, 255);
      ledcWrite(pines, pwmValue); // pwmValue controla la intensidad
      ledState[channel] = true; // Actualizar estado del LED
      previousMillis[channel] = currentMillis; // Reiniciar el tiempo
    }
  }
  // Imprimir valores de depuración
  Serial.print("Modo pulsado - Trigger: ");
  Serial.print(triggerNum);
  Serial.print(" - Canal: ");
  Serial.print(channel + 1);
  Serial.print(" - Intensidad PWM: ");
  Serial.println(intensity[channel]);
}
void handleToggledMode(int triggerNum, int channel) {
  if (sensorTriggered[channel]) {
    digitalWrite(ledPins[channel], HIGH);
    Serial.println("LED " + String(channel + 1) + " encendido por trigger " + String(triggerNum));
  } else {
    digitalWrite(ledPins[channel], LOW);
    Serial.println("LED " + String(channel + 1) + " apagado");
  }
  // Imprimir valores de depuración
  Serial.print("Modo conmutado - Trigger: ");
  Serial.print(triggerNum);
  Serial.print(" - Canal: ");
  Serial.print(channel + 1);
  Serial.print(" - Sensor Triggered: ");
  Serial.println(sensorTriggered[channel]);
}
bool isTimeToActivate() {
  if (timeinfo.tm_hour >= 1 && timeinfo.tm_hour < 24) {
    return true;
  }
  return false;
}

//------------------------------------------------------------------------Setup--------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  // Conexión Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  // Una vez conectado, imprime la dirección IP
  Serial.println("Connected to WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  // Imprime la dirección IP asignada al ESP32
  // Configuración de tiempo
  configTime(-21600, 0, "pool.ntp.org", "time.nist.gov");
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  //Serial.println(&timeinfo, "Son las %H:%M:%S");
    // Llamada a las funciones de configuración
  setupPins();      // Configura pines de sensores y LEDs
  setupInterrupts(); // Configura interrupciones para los sensores
  // Configura los canales PWM 
  for (int i = 0; i < 4; i++) {
    ledcAttach(ledPins[i], pwmFrequency, pwmResolution); // Canal, frecuencia, resolución
  }
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", htmlPage);
  });

  // Configurar las rutas
  server.on("/Control", HTTP_GET, handleControl);
  server.on("/recoverIP", HTTP_GET, handleRecoverIP);
  server.on("/restart", HTTP_GET, handleRestart);
  server.on("/diagnostics", HTTP_GET, handleDiagnostics);
  server.begin();



}
//----------------------------------------------------------------Loop-----------------------------------------------------------------------
void loop() {
  if (isTimeToActivate()) {
    for (int i = 0; i < 4; i++) {
      int currentMode = mode[i]; // Modo de operación del canal i
      int triggerNum = trigger[i]; // Trigger asociado al canal i
      int pines = ledPins[i];

      if (triggerNum >= 1 && triggerNum <= 4) {
        switch (currentMode) {
          case 1: // Continuo
            handleContinuousMode(triggerNum, pines, i);
            break;
          case 2: // Pulsado
            handlePulsedMode(triggerNum, pines, i);
            break;
          case 3: // Conmutado
            handleToggledMode(triggerNum, i);
            break;
          default:
            break;
        }
      }
    }
  }

    for (int i = 0; i < 4; i++) {
    Serial.print("Intensidad ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(intensity[i]);

    Serial.print("Modo ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(mode[i]);

    Serial.print("Tiempo de encendido ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(ledDuration[i]);

    Serial.print("Trigger ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(trigger[i]);
    
    Serial.print("Delay de encendido ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(pwm_delay[i]);

  }
  delay(3000);
}
