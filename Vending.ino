/*
Maquina Vending
Programa de una maquina vending refrigerada con las siguientes caracteristicas:
--Refrigeracin Bizona y seleccionable
--Deteccion de caida de objeto
--Detector de monedas
--Gestion de la maquina mediante comandos Shell()





6546546
Control de refrigeracion:
– Cada zona de refrigeración tendrá un control de temperatura individual. La activación de las zonas se hará de forma
individual. Cada circuito de líquido refrigerante se encontrará controlado por una electroválvula.
– Si una de las zonas se activa deberá activarse el compresor.
– Si la temperatura de cualquiera de las zonas alcanza la temperatura máxima se activará una señal de error y se parará
  el funcionamiento de la máquina.
– La máquina tendrá un control horario que permitirá introducir a ésta en un modo de bajo consumo donde no aceptara
  dinero y no dispensará productos.
– La comunicación con la máquina y la selección de productos se hará ahora mediante la consola de comandos Shell.
– La temperatura y el tiempo de activación serán configurables desde la consola de comandos Shell.
– Los parámetros y rangos de los sensores analógicos son configurables mediante comandos Shell (ganancia, offset, histéresis).

Deteccion de fallos:
– Si la temperatura de cualquiera de las zonas alcanza la temperatura máxima se activará una señal de error.
– Se monitorizará la humedad de las zonas y se detectará error cuando superé el valor especificado tal como se describe
  más adelante.
– Se supervisará la alimentación del sistema
– El mal funcionamiento, producido por cualquiera de los tres errores anteriores, llevará la máquina al paro y 
  enviará un mensaje de error específico por la consola. Además, de la indicación luminosa pertinente.

Salvado de datos en memoria no Volatil:
– Todos los parámetros de configuración se almacenaran en la EEPROM.
– El botón de Reset configurará el sistema con unos parámetros por defecto almacenados en la EEPROM.
– Los parámetros de funcionamiento y los parámetros por defecto no tendrán porqué ser coincidentes por lo que
  se almacenarán de forma independiente en le EEPROM.
– Almacenar número de productos dispensados para la obtención de estadísticas.





Vending Machine
Program of a refrigerated vending machine with the following characteristics:
--Bizone cooling and selectable
--Falling object detection
--Coin detector
--Management of the machine through Shell() commands

Refrigeration control:
– Each cooling zone will have an individual temperature control. The activation of the zones will be done
individual. Each refrigerant liquid circuit will be controlled by a solenoid valve.
– If one of the zones is activated, the compressor must be activated.
– If the temperature of any of the zones reaches the maximum temperature, an error signal will be activated and it will stop
  the operation of the machine.
– The machine will have a time control that will allow it to enter a low consumption mode where it will not accept
  money and will not dispense products.
– Communication with the machine and product selection will now be done through the Shell command console.
– The temperature and activation time will be configurable from the Shell command console.
– The parameters and ranges of the analog sensors are configurable through shell commands (gain, offset, hysteresis).

Fault detection:
– If the temperature of any of the zones reaches the maximum temperature, an error signal will be activated.
– The humidity of the zones will be monitored and an error will be detected when it exceeds the specified value as described
  later.
– System power will be monitored
– The malfunction, caused by any of the three previous errors, will stop the machine and
  will send a specific error message to the console. In addition, of the pertinent luminous indication.

Save data in non-volatile memory:
– All configuration parameters will be stored in the EEPROM.
– The Reset button will configure the system with default parameters stored in the EEPROM.
– The operating parameters and the default parameters will not have to be the same, so
  they will be stored independently in the EEPROM.
– Store number of dispensed products to obtain statistics.
*/



#include <arduino.h>
#include "MapFloat.h"
#include <stdio.h>
#include <string.h>
#include <TimerOne.h>
#include <EEPROM.h>

const int b_reset = 2;
const int ventilator1 = 3;
const int ventilator2 = 4;
const int engine1 = 5;
const int engine2 = 6;
const int solenoid1 = 7;
const int solenoid2 = 8;
const int p_compressor = 9;
const int led_error = 10;
const int caiProduct = 11;
const int selec_coins_led = 12;
const int selec_coins_button = 13;
const int pinUps = A0;
const int temp_1 = A1;
const int temp_2 = A2;
const int hum_1 = A3;
const int hum_2 = A4;

const unsigned long tDrop = 20000;
const unsigned long securityDrop = 1000;

enum Control_Hours
{
  L_COSUM,
  MODE_ON
};
Control_Hours contr_h;

enum Machine_State
{
  BATTERY_OK,
  LOW_VOLTAGE,
  ERROR_T_Z1,
  ERROR_T_Z2,
  ERROR_H_Z1,
  ERROR_H_Z2
};

Machine_State machine_state;

enum Dispenser
{
  AMOUNT,
  M_PROD,
  SELECTION,
  COMPARISON,
  ENGINE,
  DROP,
  REFUND_AMOUNT,
  RETURN_EXCHAN
};
Dispenser sale = AMOUNT;

typedef struct Clock
{
  byte hora;
  byte minuto;
  byte segundo;
  byte milisegundo;
};
Clock reloj1;

typedef struct product
{
  char description[15];
  char code[4];
  float price;
  int stock;
  int pin_engine;
  unsigned long int ton;
};

typedef struct sensor
{
  int pin;
  float hysteresis;
  float objective;
  float danger;
  float min;
  float max;
  unsigned long ms_error;
  float ofset;
};

typedef struct var_measurements
{
  float ups;
  float temperature[2];
  float humidity[2];
};
var_measurements l_various;

typedef struct error_various
{
  float ups;
  float temperature[2];
  float humidity[2];
};
error_various e_various;

typedef struct control_zones
{
  sensor humidity;
  sensor temperature;
  product products[5];
  bool refrigeration;
  bool ventilator;
};

typedef struct vending
{
  Clock on;
  Clock of;
  Clock actual_hour;
  control_zones zone[2];
  sensor ups;
  float credit;
  float change;
  bool power_mode;
};
vending machine;


void serial_delay(String *str, unsigned long time);
void millisClock(void);
float measurement(sensor *temp);
bool estate(sensor *temp, float *measurement);
bool error_ups(sensor *temp, float *measurement);
bool error(sensor *temp, float *measurement, unsigned long *cont);
int ledBlink(unsigned long ton, unsigned long tof);
unsigned long readButton(int input);
bool comparison(float price, float amount);
float amount(unsigned long time);
float change(float price, float euro);
float changeSeg(float cam);
byte serialEvent(String *com);
Clock config_schedule(String *cmd);
void config_parameters_temp(String *cmd, sensor *temp);
int trace_product(product *temp, String *str);
Control_Hours schedule(Clock *on, Clock *of, Clock *actual);
float parameter_introduced(String *cmd, String delimiter);
int trace_zone(String *cmd);

unsigned long con_temp_z1;
unsigned long con_temp_z2;
unsigned long con_hum_z1;
unsigned long con_hum_z2;
unsigned long conEngine;
unsigned long conChange;
unsigned long camTime;
unsigned long conDrop;
unsigned long cont_res;

int blink;
byte blinkCount = 0;
byte actual_Hour_int[3];
char *p_tr;
int i;
String command = "";
int tpulsed;
int zone;
int posi;
String pro_intro = "";
bool comp;
float cambEuro;
int eedirec = 0;
bool aux_error = false;

void setup()
{
  pinMode(b_reset, INPUT_PULLUP);
  pinMode(ventilator1, OUTPUT);
  pinMode(ventilator2, OUTPUT);
  pinMode(engine1, OUTPUT);
  pinMode(engine2, OUTPUT);
  pinMode(solenoid1, OUTPUT);
  pinMode(solenoid2, OUTPUT);
  pinMode(p_compressor, OUTPUT);
  pinMode(led_error, OUTPUT);
  pinMode(selec_coins_led, OUTPUT);
  pinMode(caiProduct, INPUT_PULLUP);
  pinMode(selec_coins_led, OUTPUT);
  pinMode(selec_coins_button, INPUT_PULLUP);
  Serial.begin(9600);

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  Timer1.initialize(100000);
  Timer1.attachInterrupt(millisClock);

  char actual_hour_char[9] PROGMEM = __TIME__;
  p_tr = strtok(actual_hour_char, ":");
  while (p_tr != NULL)
  {
    actual_Hour_int[i++] = atoi(p_tr);
    p_tr = strtok(NULL, ":");
  }
  if (i = 3)
    i = 0;

  reloj1 = {actual_Hour_int[0], actual_Hour_int[1], actual_Hour_int[2], 0};

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /*
    IMPORTATE: QUITAR COMENTARIOS CUANDO SE QUIERA INICIAR MAQUINA POR PRIMERA VEZ
    Se cargara la informacion de la maquina en la eeprom

    //Datos maquina

    machine.change = 0.0;
    machine.credit = 0.0;

    //Clock = {h, m, s, ms};
    machine.on = {6, 30, 0, 0};
    machine.of = {23, 59, 0, 0};
    machine.actual_hour = {0, 0, 0, 0};
    machine.power_mode = true;

    //sensor = {pin, histe, obj, peli, min, max, ms_er, ofs}
    machine.ups = {pinUps, 0, 0, 10.0, 0.0, 12.0, 3000, 0.0};
    machine.zone[0].temperature = {temp_1, 0.75, 16.0, 25, -10.0, 40.0, 2000, 0.0};
    machine.zone[1].temperature = {temp_2, 0.75, 22.0, 25, -10.0, 40.0, 2000, 0.0};
    machine.zone[0].humidity = {hum_1, 5.0, 20.0, 30.0, 0.0, 100.0, 6000, 1};
    machine.zone[1].humidity = {hum_2, 5.0, 20.0, 30.0, 0.0, 100.0, 6000, 1};

    //product = {desc, cod, preci, stock, pin, ton}
    strcpy(machine.zone[0].products[0].description, "Coca Cola");
    strcpy(machine.zone[0].products[1].description, "Fanta Naranja");
    strcpy(machine.zone[0].products[2].description, "Zumo de Limon");
    strcpy(machine.zone[0].products[3].description, "Zumo de Piña");
    strcpy(machine.zone[0].products[4].description, "Agua Mineral");
    strcpy(machine.zone[1].products[0].description, "Patatas fritas");
    strcpy(machine.zone[1].products[1].description, "Cacahuetes");
    strcpy(machine.zone[1].products[2].description, "Pistachos");
    strcpy(machine.zone[1].products[3].description, "Pipas");
    strcpy(machine.zone[1].products[4].description, "Maices");

    strcpy(machine.zone[0].products[0].code, "A35");
    strcpy(machine.zone[0].products[1].code, "A36");
    strcpy(machine.zone[0].products[2].code, "A37");
    strcpy(machine.zone[0].products[3].code, "A38");
    strcpy(machine.zone[0].products[4].code, "A39");
    strcpy(machine.zone[1].products[0].code, "A40");
    strcpy(machine.zone[1].products[1].code, "A41");
    strcpy(machine.zone[1].products[2].code, "A42");
    strcpy(machine.zone[1].products[3].code, "A43");
    strcpy(machine.zone[1].products[4].code, "A44");

    machine.zone[0].products[0].price = 1.50;
    machine.zone[0].products[1].price = 1.50;
    machine.zone[0].products[2].price = 1.50;
    machine.zone[0].products[3].price = 1.50;
    machine.zone[0].products[4].price = 2.50;
    machine.zone[1].products[0].price = 1.25;
    machine.zone[1].products[1].price = 1.00;
    machine.zone[1].products[2].price = 2.50;
    machine.zone[1].products[3].price = 1.00;
    machine.zone[1].products[4].price = 3.00;

    machine.zone[0].products[0].stock = 100;
    machine.zone[0].products[1].stock = 100;
    machine.zone[0].products[2].stock = 100;
    machine.zone[0].products[3].stock = 100;
    machine.zone[0].products[4].stock = 100;
    machine.zone[1].products[0].stock = 100;
    machine.zone[1].products[1].stock = 100;
    machine.zone[1].products[2].stock = 100;
    machine.zone[1].products[3].stock = 100;
    machine.zone[1].products[4].stock = 100;

    machine.zone[0].products[0].pin_engine = engine1;
    machine.zone[0].products[1].pin_engine = engine1;
    machine.zone[0].products[2].pin_engine = engine1;
    machine.zone[0].products[3].pin_engine = engine1;
    machine.zone[0].products[4].pin_engine = engine1;
    machine.zone[1].products[0].pin_engine = engine2;
    machine.zone[1].products[1].pin_engine = engine2;
    machine.zone[1].products[2].pin_engine = engine2;
    machine.zone[1].products[3].pin_engine = engine2;
    machine.zone[1].products[4].pin_engine = engine2;

    machine.zone[0].products[0].ton = 5000;
    machine.zone[0].products[1].ton = 5000;
    machine.zone[0].products[2].ton = 5000;
    machine.zone[0].products[3].ton = 5000;
    machine.zone[0].products[4].ton = 5000;
    machine.zone[1].products[0].ton = 5000;
    machine.zone[1].products[1].ton = 5000;
    machine.zone[1].products[2].ton = 5000;
    machine.zone[1].products[3].ton = 5000;
    machine.zone[1].products[4].ton = 5000;

    //Carga de datos
    eedirec = 0;
    EEPROM.put(eedirec, machine);
    Serial.println("La eeprom se ha cargado");
    Serial.println("Ocupa: ");
    Serial.println(sizeof(machine));

    //Datos para el reset
    eedirec += sizeof(machine);
    EEPROM.put(eedirec, machine);
    Serial.println("Se ha cargado la maquina de fabrica");
    eedirec = 0;
  */
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  eedirec = 0;
  EEPROM.get(eedirec, machine);
  Serial.println("Se ha cargado la maquina de la eeprom");

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  Serial.println(F("----------------------------------------------------------------------------------------------------"));
  Serial.println(F("----------------------------------------------------------------------------------------------------"));
  Serial.println(F("                           SISTEMAS ELECTRONICOS PROGRAMABLES"));
  Serial.println(F("                                     PRACTICA 2"));
  Serial.println();
  Serial.println(F("                                 MAQUINA VENDING 2.0"));
  Serial.println(F("                                                                                 autor: Gets Magaña"));
  Serial.println(F("                                                                                 fecha: 18/05/2021"));
  Serial.println(F("----------------------------------------------------------------------------------------------------"));
  Serial.println(F("----------------------------------------------------------------------------------------------------"));
}



void loop()
{
  //Serial.println(sizeof(machine));
  l_various.ups = measurement(&machine.ups);
  l_various.temperature[0] = measurement(&machine.zone[0].temperature);
  l_various.temperature[1] = measurement(&machine.zone[1].temperature);
  l_various.humidity[0] = measurement(&machine.zone[0].humidity);
  l_various.humidity[1] = measurement(&machine.zone[1].humidity);

  e_various.ups = error_ups(&machine.ups, &l_various.ups);
  if (e_various.ups)
    machine_state = LOW_VOLTAGE;
  else
    machine_state = BATTERY_OK;

  e_various.temperature[0] = error(&machine.zone[0].temperature, &l_various.temperature[0], &con_temp_z1);
  if (e_various.temperature[0])
    machine_state = ERROR_T_Z1;

  e_various.temperature[1] = error(&machine.zone[1].temperature, &l_various.temperature[1], &con_temp_z2);
  if (e_various.temperature[1])
    machine_state = ERROR_T_Z2;

  e_various.humidity[0] = error(&machine.zone[0].humidity, &l_various.humidity[0], &con_hum_z1);
  if (e_various.humidity[0])
    machine_state = ERROR_H_Z1;

  e_various.humidity[1] = error(&machine.zone[1].humidity, &l_various.humidity[1], &con_hum_z2);
  if (e_various.humidity[1])
    machine_state = ERROR_H_Z2;

  switch (machine_state)
  {
    case LOW_VOLTAGE:
      //ledBlink(ton, tof);
      blink = ledBlink(1000, 5000);
      digitalWrite(led_error, blink);
      digitalWrite(solenoid1, LOW);
      digitalWrite(solenoid2, LOW);
      digitalWrite(ventilator1, LOW);
      digitalWrite(ventilator2, LOW);
      digitalWrite(p_compressor, LOW);
      command = "Error de alimentacion";
      serial_delay(&command, 2000UL);
      command = "";
      break;

    case ERROR_T_Z1:
      blink = ledBlink(1000, 1000);
      digitalWrite(led_error, blink);
      command = "Error refrigeration Zona 1";
      serial_delay(&command, 2000UL);
      command = "";
      break;

    case ERROR_T_Z2:
      blink = ledBlink(1000, 1000);
      digitalWrite(led_error, blink);
      command = "Error refrigeration Zona 2";
      serial_delay(&command, 2000UL);
      command = "";
      break;

    case ERROR_H_Z1:
      blink = ledBlink(5000, 1000);
      digitalWrite(led_error, blink);
      command = "Error humedad Zona 1";
      serial_delay(&command, 2000UL);
      command = "";
      break;

    case ERROR_H_Z2:
      blink = ledBlink(5000, 1000);
      digitalWrite(led_error, blink);
      command = "Error humedad Zona 2";
      serial_delay(&command, 2000UL);
      command = "";
      break;

    case BATTERY_OK:
      machine.zone[0].refrigeration = estate(&machine.zone[0].temperature, &l_various.temperature[0]);
      machine.zone[1].refrigeration = estate(&machine.zone[1].temperature, &l_various.temperature[1]);
      machine.zone[0].ventilator = estate(&machine.zone[0].humidity, &l_various.humidity[0]);
      machine.zone[1].ventilator = estate(&machine.zone[1].humidity, &l_various.humidity[1]);

      digitalWrite(solenoid1, machine.zone[0].refrigeration);
      digitalWrite(solenoid2, machine.zone[1].refrigeration);
      digitalWrite(ventilator1, machine.zone[0].ventilator);
      digitalWrite(ventilator2, machine.zone[1].ventilator);

      if ((machine.zone[0].refrigeration == true) || (machine.zone[1].refrigeration == true))
        digitalWrite(p_compressor, HIGH);
      else
        digitalWrite(p_compressor, LOW);

      if (sale != SELECTION) {
        if (serialEvent(&command) == 1)
        {
          Serial.println("ECO: " + command);
          if (command == "HELP")
          {
            Serial.println(F("----------------------------------------------------------------------------------------------------"));
            Serial.println(F("----------------------------------------------------------------------------------------------------"));
            Serial.println(F("COMANDOS:"));
            Serial.println();
            Serial.println(F("M_ON                                 -> Activa el modo bajo consumo"));
            Serial.println(F("M_OFF                                -> Desactiva el modo bajo consumo"));
            Serial.println(F("SET_TIME_ON  XX:XX:XX                -> Cambia la hora de inicio"));
            Serial.println(F("SET_TIME_OF  XX:XX:XX                -> Cambia la hora de inicio de bajo consumo"));
            Serial.println();
            Serial.println(F("SET_TEM      Z:OB                    -> Cambia la temperatura objetivo de la zona indicada"));
            Serial.println(F("SET_HUM      Z:OB                    -> Cambia la humedad objetivo de la zona indicada"));
            Serial.println(F("Siendo  Z:                           -> La zona"));
            Serial.println(F("Siendo  OB:                          -> El objetivo"));
            Serial.println();
            Serial.println(F("CONFIG_HUM   Z:O                     -> Cambia la configuracion del sensor de humedad indicado"));
            Serial.println(F("Siendo  Z:                           -> La zona"));
            Serial.println(F("Siendo  O:                           -> El ofset"));
            Serial.println();
            Serial.println(F("CONFIG_TEM   Z:MIN:MAX:H.XX          -> Cambia la configuracion del sensor de temperatura indicado"));
            Serial.println(F("Siendo Z:                            -> La zona"));
            Serial.println(F("Siendo MIN:                          -> La temperatura minima deseada"));
            Serial.println(F("Siendo MAX:                          -> La temperatura maxima deseada"));
            Serial.println(F("Siendo H.XX:                         -> La hysteresis"));
            Serial.println();
            Serial.println(F("STOCK        Z:XXX-COD               -> Repone el stock del producto indicado"));
            Serial.println(F("Siendo Z:                            -> La zona, refrescos zona 1, comida zona 2"));
            Serial.println(F("Siendo XXX:                          -> La cantidad de stock"));
            Serial.println(F("Zona 1 COD:  A35                     -> Coca cola"));
            Serial.println(F("             A36                     -> Fanta de naranja"));
            Serial.println(F("             A37                     -> Zumo de limon"));
            Serial.println(F("             A38                     -> Zumo de piña"));
            Serial.println(F("             A39                     -> Agua mineral"));
            Serial.println(F("Zona 2 COD:  A40                     -> Patatas fritas"));
            Serial.println(F("             A41                     -> Cacahuetes"));
            Serial.println(F("             A42                     -> Pistachos"));
            Serial.println(F("             A43                     -> Pipas"));
            Serial.println(F("             A44                     -> Maices"));
            Serial.println();
            Serial.println(F("HORA                                 -> Muestra la hora actual"));
            Serial.println(F("VER_STOCK                            -> Muestra el stock restante te todos los articulos"));
            Serial.println(F("VER_CONFIG                           -> Muestra la configuracion de la machine actual"));
            Serial.println(F("RESET                                -> Vuelve la machine a la configuracion de serie"));
            Serial.println(F("----------------------------------------------------------------------------------------------------"));
            command = "";
          }
          else if (command.substring(0, (command.indexOf(" "))) == "M_ON")
          {
            machine.power_mode = true;
            EEPROM.put(0, machine);
            Serial.println(F("Se han actualizado los datos en la eeprom"));
            command = "";
          }
          else if (command.substring(0, (command.indexOf(" "))) == "M_OFF")
          {
            machine.power_mode = false;
            EEPROM.put(0, machine);
            Serial.println(F("Se han actualizado los datos en la eeprom"));
            command = "";
          }
          else if (command.substring(0, (command.indexOf(" "))) == "SET_TIME_ON")
          {
            machine.on = config_schedule(&command);
            Serial.print(F("La hora introducida de inicio es: "));
            Serial.print(machine.on.hora);
            Serial.print(F(":"));
            Serial.print(machine.on.minuto);
            Serial.print(F(":"));
            Serial.println(machine.on.segundo);
            EEPROM.put(0, machine);
            Serial.println(F("Se han actualizado los datos en la eeprom"));
            command = "";
          }
          else if (command.substring(0, (command.indexOf(" "))) == "SET_TIME_OF")
          {
            machine.of = config_schedule(&command);
            Serial.print(F("La hora introducida de bajo consumo es: "));
            Serial.print(machine.of.hora);
            Serial.print(":");
            Serial.print(machine.of.minuto);
            Serial.print(":");
            Serial.println(machine.of.segundo);
            EEPROM.put(0, machine);
            Serial.println(F("Se han actualizado los datos en la eeprom"));
            command = "";
          }
          else if (command.substring(0, (command.indexOf(" "))) == "SET_TEM")
          {
            zone = trace_zone(&command);
            machine.zone[zone].temperature.objective = parameter_introduced(&command, ":");
            Serial.print(F("Se ha modificado la temperatura objetivo en la zona: "));
            Serial.println(zone + 1);
            Serial.print(F("La temperatura objetivo introducida es: "));
            Serial.println(machine.zone[zone].temperature.objective);
            EEPROM.put(0, machine);
            Serial.println(F("Se han actualizado los datos en la eeprom"));
            command = "";
          }
          else if (command.substring(0, (command.indexOf(" "))) == "SET_HUM")
          {
            zone = trace_zone(&command);
            machine.zone[zone].humidity.objective = parameter_introduced(&command, ":");
            Serial.print(F("Se ha modificado la humedad objetivo en la zona: "));
            Serial.println(zone + 1);
            Serial.print(F("La humedad objetivo introducida es: "));
            Serial.println(machine.zone[zone].humidity.objective);
            EEPROM.put(0, machine);
            Serial.println(F("Se han actualizado los datos en la eeprom"));
            command = "";
          }
          else if (command.substring(0, (command.indexOf(" "))) == "CONFIG_HUM")
          {
            zone = trace_zone(&command);
            machine.zone[zone].humidity.ofset = parameter_introduced(&command, ":");
            Serial.print(F("Se ha modificado la ofset de la humedad en la zona: "));
            Serial.println(zone + 1);
            Serial.print(F("El ofset introducido es: "));
            Serial.println(machine.zone[zone].humidity.ofset);
            EEPROM.put(0, machine);
            Serial.println(F("Se han actualizado los datos en la eeprom"));
            command = "";
          }
          else if (command.substring(0, (command.indexOf(" "))) == "CONFIG_TEM")
          {
            zone = trace_zone(&command);
            config_parameters_temp(&command, &machine.zone[zone].temperature);
            Serial.print(F("Se ha modificado la configuracion de la zona: "));
            Serial.println(zone + 1);
            Serial.print(F("La temperatura minima introducida es: "));
            Serial.println(machine.zone[zone].temperature.min);
            Serial.print(F("La temperatura maxima introducida es: "));
            Serial.println(machine.zone[zone].temperature.max);
            Serial.print(F("La histerisis introducida es: "));
            Serial.println(machine.zone[zone].temperature.hysteresis);
            EEPROM.put(0, machine);
            Serial.println(F("Se han actualizado los datos en la eeprom"));
            command = "";
          }
          else if (command.substring(0, (command.indexOf(" "))) == "STOCK")
          {
            zone = trace_zone(&command);
            String str = command.substring((command.indexOf("-")) + 1);
            //str.trim();
            int posicion = trace_product(machine.zone[zone].products, &str);
            int stock = parameter_introduced(&command, ":");
            machine.zone[zone].products[posicion].stock = stock;
            Serial.print(F("Se ha modificado el stock del producto: "));
            Serial.println(machine.zone[zone].products[posicion].description);
            Serial.print(F("El stock actual es: "));
            Serial.println(machine.zone[zone].products[posicion].stock);
            EEPROM.put(0, machine);
            Serial.println(F("Se han actualizado los datos en la eeprom"));
            command = "";
          }
          else if (command.substring(0, (command.indexOf(" "))) == "HORA")
          {
            Serial.print(F("La hora atual es:"));
            Serial.print(machine.actual_hour.hora);
            Serial.print(F(":"));
            Serial.print(machine.actual_hour.minuto);
            Serial.print(F(":"));
            Serial.println(machine.actual_hour.segundo);
            command = "";
          }
          else if (command.substring(0, (command.indexOf(" "))) == "VER_STOCK")
          {
            for (int r = 0; r < 2; r++) {
              for (int w = 0; w < 5; w++) {
                Serial.println(machine.zone[r].products[w].description);
                Serial.println(machine.zone[r].products[w].stock);
              }
            }
            command = "";
          }
          else if (command.substring(0, (command.indexOf(" "))) == "VER_CONFIG")
          {
            Serial.print(F("La hora de inicio es:"));
            Serial.print(machine.on.hora);
            Serial.print(F(":"));
            Serial.print(machine.on.minuto);
            Serial.print(F(":"));
            Serial.println(machine.on.segundo);
            Serial.println();
            Serial.print(F("La hora de bajo consumo es:"));
            Serial.print(machine.of.hora);
            Serial.print(F(":"));
            Serial.print(machine.of.minuto);
            Serial.print(F(":"));
            Serial.println(machine.of.segundo);
            Serial.println();
            Serial.println(F("La parametros de la zona 1 son:"));
            Serial.println(F("Temperatura:"));
            Serial.print(F("Objetivo: "));
            Serial.println(machine.zone[0].temperature.objective);
            Serial.print(F("Minima: "));
            Serial.println(machine.zone[0].temperature.min);
            Serial.print(F("Maxima: "));
            Serial.println(machine.zone[0].temperature.max);
            Serial.print(F("Histeresis: "));
            Serial.println(machine.zone[0].temperature.hysteresis);
            Serial.println(F("Humedad:"));
            Serial.print(F("Objetivo: "));
            Serial.println(machine.zone[0].humidity.objective);
            Serial.print(F("Ofset: "));
            Serial.println(machine.zone[0].humidity.ofset);
            Serial.print(F("Histeresis: "));
            Serial.println(machine.zone[0].humidity.hysteresis);
            Serial.println();
            Serial.println(F("La parametros de la zona 2 son:"));
            Serial.println(F("Temperatura:"));
            Serial.print(F("Objetivo: "));
            Serial.println(machine.zone[1].temperature.objective);
            Serial.print(F("Minima: "));
            Serial.println(machine.zone[1].temperature.min);
            Serial.print(F("Maxima: "));
            Serial.println(machine.zone[1].temperature.max);
            Serial.print(F("Histeresis: "));
            Serial.println(machine.zone[1].temperature.hysteresis);
            Serial.println(F("Humedad:"));
            Serial.print(F("Objetivo: "));
            Serial.println(machine.zone[1].humidity.objective);
            Serial.print(F("Ofset: "));
            Serial.println(machine.zone[1].humidity.ofset);
            Serial.print(F("Histeresis: "));
            Serial.println(machine.zone[1].humidity.hysteresis);
            Serial.println();
            if (machine.power_mode == true)Serial.println(F("El control de horario esta activado."));
            else if (machine.power_mode == false)Serial.println(F("El control de horario esta desactivado."));
            command = "";
          }
          else if (command.substring(0, (command.indexOf(" "))) == "RESET")
          {
            aux_error = true;
            cont_res = millis();
            Serial.println(F("Pulsa el boton RESET durante al menos 1s:"));
            command = "";
          }
          else
          {
            Serial.println(F("No has introducido un codigo valido"));
            command = "";
          }
          //command = "";
          Serial.println(F("----------------------------------------------------------------------------------------------------"));
        }

        if (aux_error == true) {
          if ((millis() - cont_res) <= 20000)
          {
            //tpulsado_res = readButton(b_reset);
            if (readButton(b_reset) > 1000)
            {
              eedirec = sizeof(machine);
              EEPROM.get(eedirec, machine);
              EEPROM.put(0, machine);
              Serial.println("Se ha reseteado la machine");
              Serial.println(F("----------------------------------------------------------------------------------------------------"));
              eedirec = 0;
              aux_error = false;
            }

          }
          else {
            Serial.println(F("Ha pasado demasiado tiempo....vuelve a intentarlo"));
            Serial.println(F("----------------------------------------------------------------------------------------------------"));
            aux_error = false;
          }
        }
      }

      if (machine.power_mode == true) contr_h = schedule(&machine.on, &machine.of, &machine.actual_hour);
      else if (machine.power_mode == false) contr_h = MODE_ON;

      switch (contr_h)
      {
        case MODE_ON:
          switch (sale)
          {
            case AMOUNT:
              tpulsed = readButton(selec_coins_button);
              if (tpulsed > 0)
              {
                machine.credit += amount(tpulsed);
                if (machine.credit > 0)
                {
                  Serial.println(F("El importe introducidos es: "));
                  Serial.println(machine.credit);
                  sale = M_PROD;
                }
              }
              break;

            case M_PROD:
              Serial.println(F("****************************************************************"));
              Serial.println(F("Introduce el producto deseado: "));
              Serial.println(F("      A35       ->Precio:1.50€            -> Refresco de cola"));
              Serial.println(F("      A36       ->Precio:1.50€            -> Refresco de naranja"));
              Serial.println(F("      A37       ->Precio:1.50€            -> Refresco de limon"));
              Serial.println(F("      A38       ->Precio:1.50€            -> Refresco de zumo"));
              Serial.println(F("      A39       ->Precio:2.50€            -> Aguan mineral"));
              Serial.println(F("      A40       ->Precio:1.25€            -> Refresco de cola"));
              Serial.println(F("      A41       ->Precio:1.00€            -> Cacahuetes"));
              Serial.println(F("      A42       ->Precio:2.50€            -> Pistachos"));
              Serial.println(F("      A43       ->Precio:1.00€            -> Pipas"));
              Serial.println(F("      A44       ->Precio:3.00€            -> Nachos de queso"));
              Serial.println(F("****************************************************************"));
              sale = SELECTION;
              break;

            case SELECTION:
              if (serialEvent(&pro_intro) == 1)
              {
                zone = 0;
                Serial.println("ECO: " + pro_intro);
                posi = trace_product(machine.zone[zone].products, &pro_intro);
                if (posi < 0) {
                  zone ++;
                  posi = trace_product(machine.zone[zone].products, &pro_intro);
                  pro_intro = "";
                }
                else {
                  pro_intro = "";
                }

                if (posi >= 0)
                {
                  if (machine.zone[zone].products[posi].stock >= 1) {
                    Serial.println(F("Has seleccionado el producto: "));
                    Serial.println(machine.zone[zone].products[posi].description);
                    Serial.println(F("Precio: "));
                    Serial.println(machine.zone[zone].products[posi].price);
                    Serial.println(F("****************************************************************"));
                    sale = COMPARISON;
                  }
                  else {
                    Serial.println(F("No hay stock del producto solicitado"));
                    Serial.println(F("...............introduce otro codigo"));
                    Serial.println(F("****************************************************************"));
                  }
                }
                else
                {
                  Serial.println(F("No has seleccionado ningun producto"));
                  Serial.println(F("................vuelve a intentarlo"));
                  Serial.println(F("****************************************************************"));
                }
              }
              break;

            case COMPARISON:
              comp = comparison(machine.zone[zone].products[posi].price, machine.credit);
              if (comp == true)
              {
                Serial.println(F("Has introducido suficiente dinero"));
                Serial.println(F("****************************************************************"));
                conEngine = millis();
                sale = ENGINE;
              }
              else if (comp == false)
              {
                Serial.println(F("No has introducido suficiente dinero"));
                Serial.println(F("****************************************************************"));
                sale = AMOUNT;
              }
              break;

            case ENGINE:
              if (millis() - conEngine < machine.zone[zone].products[posi].ton)
              {
                digitalWrite(machine.zone[zone].products[posi].pin_engine, ledBlink(500, 500));
              }
              else
              {
                digitalWrite(machine.zone[zone].products[posi].pin_engine, LOW);
                conDrop = millis();
                sale = DROP;
              }
              break;

            case DROP:
              if (millis() - conDrop <= tDrop)
              {
                if (readButton(caiProduct) >= securityDrop)
                {
                  machine.zone[zone].products[posi].stock -= 1;
                  conChange = millis();
                  EEPROM.put(0, machine);
                  Serial.println(F("Se han actualizado los datos en la eeprom"));
                  sale = RETURN_EXCHAN;
                }
              }
              else
              {
                conChange = millis();
                sale = REFUND_AMOUNT;
              }
              break;

            case REFUND_AMOUNT:
              camTime = changeSeg(machine.credit);
              if (millis() - conChange <= camTime)
              {
                digitalWrite(selec_coins_led, HIGH);
              }
              else
              {
                Serial.println(F("El producto no ha sido dispensado."));
                Serial.println(F("El cambio es: "));
                Serial.println(machine.credit);
                Serial.println(F("****************************************************************"));
                digitalWrite(selec_coins_led, LOW);
                machine.credit = 0;
                sale = AMOUNT;
              }
              break;

            case RETURN_EXCHAN:
              cambEuro = change(machine.zone[zone].products[posi].price, machine.credit);
              camTime = changeSeg(cambEuro);
              if (millis() - conChange <= camTime)
              {
                digitalWrite(selec_coins_led, HIGH);
              }
              else
              {
                Serial.println(F("El producto ha sido dispensado."));
                Serial.println(F("El cambio es: "));
                Serial.println(cambEuro);
                Serial.print(F("Quedan:"));
                Serial.print(machine.zone[zone].products[posi].stock);
                Serial.println(" unidades");
                Serial.println(F("****************************************************************"));
                digitalWrite(selec_coins_led, LOW);
                machine.credit = 0;
                sale = AMOUNT;
              }
              break;
          }
          break;

        case L_COSUM:

          break;
      }
      break;
  }
}

/*
Funcion que mantiene el reloj actualizado 

Function that keeps the clock updated
*/
void millisClock(void)
{
  blinkCount++;

  noInterrupts();
  if (blinkCount == 1)
  {
    blinkCount = 0;
    reloj1.milisegundo += 10;
  }
  if (reloj1.milisegundo == 100)
  {
    reloj1.milisegundo = 0;
    reloj1.segundo++;
  }
  if (reloj1.segundo == 60)
  {
    reloj1.minuto++;
    reloj1.segundo = 0;
  }
  if (reloj1.minuto == 60)
  {
    reloj1.hora++;
    reloj1.minuto = 0;
  }
  if (reloj1.hora == 24)
  {
    reloj1.hora = 0;
  }

  machine.actual_hour.milisegundo = reloj1.milisegundo;
  machine.actual_hour.segundo = reloj1.segundo;
  machine.actual_hour.minuto = reloj1.minuto;
  machine.actual_hour.hora = reloj1.hora;

  interrupts();
}

/*
Funcion para realizar las medidas de los sensores
Argumentos:
struct sensor

Function to perform sensor measurements
Arguments:
struct sensor
*/
float measurement(sensor *temp)
{
  static float measurement;
  static float o_min = (1023.0 / 5.0) * temp->ofset;
  measurement = analogRead(temp->pin);
  measurement = mapFloat(measurement, o_min, 1023, temp->min, temp->max);
  return measurement;
}

/*
Funcion que nos devuele el estado segun lecturas
Argumentos:
struct sensor
measurement -- medida leida de los sensores

Function that returns the state according to readings
Arguments:
structure sensor
measurement -- measurement read from sensors
*/
bool estate(sensor *temp, float *measurement)
{
  static bool estate;
  if (*measurement < (temp->objective - temp->hysteresis) && estate == true)
    estate = false;
  else if (*measurement > (temp->objective - temp->hysteresis) && estate == false)
    estate = true;

  return estate;
}

/*
Funcion que nos devuele el posible error del ups
Argumentos:
struct sensor
measurement -- medida leida de los sensores

Function that returns the possible error of the ups
Arguments:
structure sensor
measurement -- measurement read from sensors
*/
bool error_ups(sensor *temp, float *measurement)
{
  static unsigned long cont;
  static bool error;
  if (*measurement > temp->danger)
  {
    cont = millis();
    error = false;
  }
  else if (*measurement < temp->danger)
  {
    if (millis() - cont >= temp->ms_error && *measurement < temp->danger)
      error = true;
  }
  return error;
}

/*
Funcion que nos devuele el posible error de los sensores
Argumentos:
struct sensor
measurement -- medida leida de los sensores
cont -- contador para diferentes sensores

Function that returns the possible error of the sensors
Arguments:
structure sensor
measurement -- measurement read from sensors
cont -- counter for diferent sensors
*/
bool error(sensor *temp, float *measurement, unsigned long *cont)
{
  static bool error;
  if (*measurement < temp->danger)
  {
    *cont = millis();
    error = false;
  }
  else if (*measurement > temp->danger)
  {
    if (millis() - *cont >= temp->ms_error && *measurement > temp->danger)
      error = true;
  }
  return error;
}

/*
Funcion para hacer un parpadeo en los leds
Argumentos:
ton -- tiempo on
tof -- tiempo of

Function to make a blink in the leds
Arguments:
ton -- time on
tof -- time of
*/
int ledBlink(unsigned long ton, unsigned long tof)
{
  static unsigned long tbefore;
  static int ledstate;

  if (ledstate == HIGH)
  {
    if (millis() - tbefore >= ton)
    {
      ledstate = LOW;
      tbefore = millis();
    }
  }
  else if (ledstate == LOW)
  {
    if (millis() - tbefore >= tof)
    {
      ledstate = HIGH;
      tbefore = millis();
    }
  }
  return ledstate;
}

/*
Funcion que nos devuele el tiempo trascurrido pulsando un boton
Argumentos:
input -- entrada

Function that returns the elapsed time by pressing a button
Arguments:
input -- input
*/
unsigned long readButton(int input)
{
  static int previusState = HIGH;
  static int actualState;
  static unsigned long cont;
  unsigned long timePressed = 0;
  actualState = digitalRead(input);

  if (actualState != previusState)
  {
    if (previusState == HIGH && actualState == LOW)
    {
      cont = millis();
    }
    else if (previusState == LOW && actualState == HIGH)
    {
      timePressed = millis() - cont;
    }
  }
  previusState = actualState;
  return timePressed;
}

/*
Funcion que nos devuelve "true" si el importe es mayor al precio
Argumentos:
price -- precio
amount -- importe

Function that returns "true" if the amount is greater than the price
Arguments:
price -- price
amount -- amount
*/
bool comparison(float price, float amount)
{
  static bool comp;
  if (amount < price)
    comp = false;
  if (amount > price)
    comp = true;
  return comp;
}

/*
Funcion que convierte el timpo en euros
Argumentos:
time - tiempo transcurrido

Function that converts time into euros
Arguments:
time - time
*/
float amount(unsigned long time)
{
  static int cent;
  static float euro;
  if (time >= 100)
  { //los tiempos estan modificados porque eran muy largos
    cent = 1 * (time / 100);
    euro = 5 * cent * 0.01;
  }
  else
  {
    euro = 0;
  }
  return euro;
}

/*
Funcion que calcula el cambio
Argumentos:
price -- precio
euro -- importe introducido

Function that calculates the change
Arguments:
price -- price
euro -- entered amount
*/
float change(float price, float euro)
{
  static float change;
  if (euro >= price)
  {
    change = (euro - price);
  }
  else
  {
    change = euro;
  }
  return change;
}

/*
Funcion que calcula el cambio en segundos
Argumentos:
cam -- cambio en euros

Function that calculates the change in seconds
Arguments:
cam -- change in euros
*/
float changeSeg(float cam)
{
  static float camseg;
  camseg = (cam * 100 * 500) / 5;
  return camseg;
}

/*
Funcion para leer el teclado
Si se ha escrito algo devolvera un 1, de no ser asi un 0
Argumentos:
com -- string donde se guardaran los datos introducidos

Function to read the keyboard
If something has been written it will return a 1, otherwise a 0
Arguments:
com -- string where the entered data will be saved
*/
byte serialEvent(String *com)
{
  static char inChar;
  while (Serial.available() > 0)
  {
    inChar = Serial.read();
    *com += inChar;
    if (inChar == '\n')
    {
      (*com).trim();
      return 1;
    }
  }
  return 0;
}

/*
Funcion para configurar el horario de la maquina
Argumentos:
com -- comando

Function to set the time of the machine
Arguments:
com -- command
*/
Clock config_schedule(String *cmd)
{
  Clock temp;
  static byte p;
  p = (cmd->indexOf(" ")) + 1;
  temp.hora = cmd->substring(p, ":").toInt();
  temp.minuto = cmd->substring(p + 3, ":").toInt();
  temp.segundo = cmd->substring(p + 3 + 3, ":").toInt();
  temp.milisegundo = 0;

  return temp;
}

/*
Funcion para configurar los parametros de la maquina
Argumentos:
com -- comando

Function to configure the parameters of the machine
Arguments:
com -- command
*/
void config_parameters_temp(String *cmd, sensor *temp)
{
  static byte p;
  p = (cmd->indexOf(":")) + 1;
  temp->min = cmd->substring(p, ":").toInt();
  p = (cmd->indexOf(":", p)) + 1;
  temp->max = cmd->substring(p, ":").toInt();
  p = (cmd->indexOf(":", p)) + 1;
  temp->hysteresis = cmd->substring(p).toFloat();
}

/*
Funcion para busca el producto introducido por teclado segun codigo
Argumentos:
product -- struct product
str -- codigo producto

Function to search for the product entered by keyboard according to code
Arguments:
product -- struct product
str -- product code
*/
int trace_product(product *temp, String *str)
{
  static int i;
  for (i = 0; i < 5; i++)
  {
    if (str->equals(temp[i].code))
    {
      return i;
    }
  }
  return -1;
}

/*
Funcion que nos devolvera el estado de la maquina segun la hora actual
Argumentos:
on -- hora de encendido
of -- hora de apagado
actual -- hora actual

Function that will return the state of the machine according to the current time
Arguments:
on -- power on time
of -- power off time
current -- current time
*/
Control_Hours schedule(Clock *on, Clock *of, Clock *actual)
{
  Control_Hours c_h;
  if ((actual->hora >= on->hora) && (actual->hora <= of->hora))
  {
    if ((actual->hora == on->hora))
    {
      if ((actual->minuto >= on->minuto))
      {
        if ((actual->segundo >= on->segundo))
        {
          c_h = MODE_ON;
        }
      }
    }
    if ((actual->hora > on->hora))
    {

      if ((actual->hora < of->hora))
      {
        c_h = MODE_ON;
      }
      else if ((actual->hora == of->hora))
      {
        if ((actual->minuto <= of->minuto))
        {
          if ((actual->segundo <= of->segundo))
          {
            c_h = MODE_ON;
          }
        }
      }
    }
  }
  else c_h = L_COSUM;
  return c_h;
}

/*
Funcion para busca el parametro introducido por teclado
Argumentos:
cmd -- comando
delimiter -- delimitador a partir donde buscara el parametro

Function to search for the parameter entered by keyboard
Arguments:
cmd -- command
delimiter -- delimiter from where to look for the parameter
*/
float parameter_introduced(String *cmd, String delimiter)
{
  static byte p;
  static float temp;
  p = (cmd->indexOf(delimiter)) + 1;
  temp = (cmd->substring(p).toInt());
  return temp;
}

/*
Funcion para busca la zona introducida por teclado
Argumentos:
cmd -- comando

Function to search the zone entered by keyboard
Arguments:
cmd -- command
*/
int trace_zone(String *cmd)
{
  static byte p;
  static int temp;
  p = (cmd->indexOf(" ")) + 1;
  temp = (cmd->substring(p, p + 1).toInt());
  return temp - 1;
}

/*
Funcion para imprimir por pantalla cada cierto tiempo
Argumentos:
str -- comando
time -- tiempo de delay

Function to print on screen from time to time
Arguments:
str -- command
time -- delay time
*/
void serial_delay(String *str, unsigned long time)
{
  static unsigned long cont;
  if (millis() - cont > time) {
    Serial.println(*str);
    Serial.print(F("La hora atual es:"));
    Serial.print(machine.actual_hour.hora);
    Serial.print(F(":"));
    Serial.print(machine.actual_hour.minuto);
    Serial.print(F(":"));
    Serial.println(machine.actual_hour.segundo);
    cont = millis();
    *str = "";
  }
}
