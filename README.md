# Control-Remoto-IR-por-Voz
CURSO MICROCONTROLADORES 2 
# Control Remoto IR por Voz

Proyecto desarrollado para el curso de **Microcontroladores II**.  
El sistema permite controlar dispositivos domésticos convencionales mediante comandos de voz usando **Amazon Alexa**, **SinricPro**, un **ESP32** y transmisión infrarroja.

El ESP32 funciona como un control remoto universal. Recibe comandos desde Alexa mediante WiFi y luego transmite el código infrarrojo correspondiente hacia la televisión, la luz o el aire acondicionado.

---

## 1. Librerías utilizadas

Para el funcionamiento del proyecto se utilizaron las siguientes librerías en Arduino IDE:

| Librería | Función |
|---|---|
| `WiFi.h` | Permite conectar el ESP32 a la red WiFi. |
| `SinricPro.h` | Permite la comunicación entre Alexa, SinricPro y el ESP32. |
| `SinricProLight.h` | Se usa para controlar la luz desde Alexa. |
| `SinricProWindowAC.h` | Se usa para controlar el aire acondicionado. |
| `SinricProTV.h` | Se usa para registrar el televisor como dispositivo en SinricPro. |
| `IRremoteESP8266.h` | Librería principal para trabajar con señales infrarrojas. |
| `IRsend.h` | Permite enviar señales IR desde el ESP32. |
| `IRrecv.h` | Permite recibir señales IR desde un receptor infrarrojo. |
| `IRutils.h` | Permite imprimir y analizar códigos IR recibidos. |
| `ir_Midea.h` | Permite generar señales IR compatibles con aire acondicionado Midea. |

---

## 2. Materiales utilizados

| Material | Función |
|---|---|
| ESP32 | Microcontrolador principal del sistema. |
| Diodo emisor infrarrojo | Envía las señales IR hacia los dispositivos. |
| Módulo receptor IR KY-022 | Captura los códigos de los controles remotos. |
| Transistor NPN | Amplifica la señal para el LED infrarrojo. |
| Resistencias | Limitan corriente en la etapa emisora IR. |
| Amazon Alexa | Recibe los comandos de voz del usuario. |
| SinricPro | Plataforma intermediaria entre Alexa y el ESP32. |
| Arduino IDE | Entorno usado para programar el ESP32. |

---

## 3. Procedimiento realizado

### 3.1 Captura de códigos IR

Primero se utilizó el módulo receptor infrarrojo KY-022 para capturar los códigos de los controles remotos originales.

Al presionar un botón del control remoto, el ESP32 mostraba en el Monitor Serial:

- Protocolo utilizado.
- Código hexadecimal.
- Cantidad de bits.

Ejemplo de códigos capturados para la televisión:

| Botón | Código IR |
|---|---|
| Power | `0x20DF10EF` |
| Volumen + | `0x20DF40BF` |
| Volumen - | `0x20DFC03F` |
| Netflix | `0x20DF6A95` |
| Home | `0x20DF3EC1` |
| OK | `0x20DF22DD` |
| Regresar | `0x20DF14EB` |
| Exit | `0x20DFDA25` |

---

### 3.2 Programación del ESP32

Luego se programó el ESP32 para conectarse a la red WiFi y comunicarse con SinricPro.

En el código se configuraron:

- Credenciales WiFi.
- APP KEY y APP SECRET de SinricPro.
- ID de cada dispositivo.
- Pines del emisor y receptor infrarrojo.
- Códigos IR de la televisión y la luz.
- Lógica de control para TV, luz y aire acondicionado.

El pin utilizado para el emisor IR fue:

```cpp
const uint16_t IR_SEND_PIN = 4;

### 3.3 Configuración en SinricPro y Alexa

Se crearon dispositivos virtuales en SinricPro para poder controlarlos desde Alexa:

Televisor.
Luz.
Aire acondicionado.

Luego se vinculó la cuenta de SinricPro con Alexa para que los dispositivos puedan ser reconocidos mediante comandos de voz.

El dispositivo del televisor se nombró como Control IR, para evitar que Alexa lo confunda con una Smart TV o una skill de video.
3.4 Pruebas desde Monitor Serial

Antes de probar con Alexa, se verificaron los comandos desde el Monitor Serial.

Ejemplos:

tv power
tv netflix
tv home
tv vol 91
luz on
luz 50
ac on
ac temp 24

Esto permitió comprobar que el ESP32 enviaba correctamente los códigos IR antes de integrar el sistema con comandos de voz.

3.5 Pruebas con Alexa

Finalmente se realizaron pruebas usando comandos de voz.

Ejemplos:

Alexa, prende Control IR
Alexa, apaga Control IR
Alexa, pon volumen de Control IR a 91
Alexa, pon volumen de Control IR a 92
Alexa, prende luces
Alexa, pon luces al 50 por ciento
Alexa, prende aire
Alexa, pon aire en 24 grados
4. Explicación del funcionamiento

El funcionamiento general del sistema sigue este flujo:

Usuario → Alexa → SinricPro → ESP32 → Diodo emisor IR → Dispositivo

El usuario da una orden de voz a Alexa.
Alexa envía el comando a SinricPro.
SinricPro transmite la orden al ESP32 mediante WiFi.
El ESP32 interpreta el comando recibido.
Luego selecciona el código IR correspondiente.
Finalmente, el diodo emisor infrarrojo transmite la señal hacia el dispositivo.

De esta manera, el ESP32 actúa como un control remoto físico, pero controlado por voz.
5. Control del televisor

El televisor se controla usando señales IR con protocolo NEC de 32 bits.

El comando de encendido y apagado usa el código:

TV_POWER = 0x20DF10EF;

Este botón funciona como un comando alternado. Si la televisión está apagada, la enciende; si está encendida, la apaga.

También se programó el control de volumen. El televisor no recibe directamente un valor absoluto de volumen, sino pulsos de VOL+ o VOL-. Por eso, el ESP32 compara el volumen anterior con el nuevo valor recibido y envía varios pulsos según sea necesario.

6. Comandos especiales por volumen

Durante las pruebas se observó que Alexa no enviaba correctamente comandos como “abrir Netflix” o “ir a Home”, porque los interpretaba como funciones de una Smart TV.

Para solucionar esto, se usaron valores especiales de volumen.

Volumen en Alexa	Acción ejecutada
91	Netflix
92	Home
93	OK
94	Arriba
95	Abajo
96	Izquierda
97	Derecha
98	Regresar
99	Exit

Ejemplo:

Alexa, pon volumen de Control IR a 91

Aunque Alexa envía un valor de volumen, el ESP32 interpreta el número 91 como el comando para abrir Netflix.

7. Control de luces

Para la luz se usaron comandos IR directos y secuencias.

Acción	Código IR
Encender	0x20DF15EA
Apagar	0x20DF11EE
Brillo máximo	0x20DFA956
Subir brillo	0x20DF9966
Bajar brillo	0x20DFD926

Para el brillo al 50 % y 25 %, no se usó un código fijo.
El ESP32 primero coloca la luz al máximo y luego baja varios pasos.

Comando	Funcionamiento
100 %	Brillo máximo
50 %	Brillo máximo y baja 3 pasos
25 %	Brillo máximo y baja 6 pasos
8. Control del aire acondicionado

El aire acondicionado trabaja de forma diferente a la televisión, porque no usa solamente códigos simples.

En este caso se emplea la librería ir_Midea.h, que permite enviar tramas IR con información de:

Encendido.
Apagado.
Temperatura.
Modo de operación.
Ventilación.

Ejemplo:

Alexa, pon aire en 24 grados

El ESP32 recibe el valor de temperatura, actualiza el estado interno y envía una trama IR compatible con el aire acondicionado.

9. Resultados obtenidos

El sistema logró controlar correctamente los tres dispositivos principales:

Televisión.
Luz.
Aire acondicionado.

Se comprobó que la televisión respondía correctamente cuando el emisor IR estaba bien orientado hacia el receptor del equipo.

También se comprobó que los comandos especiales por volumen permitieron controlar funciones adicionales del televisor como Netflix, Home, OK, Regresar y Exit.

En el caso de la luz, se logró controlar encendido, apagado y niveles de brillo mediante secuencias de pasos.

En el aire acondicionado, se consiguió controlar encendido, apagado y temperatura mediante señales IR generadas por la librería Midea.
