#include <arduino.h>
#include "MapFloat.h"
#include <stdio.h>
#include <string.h>
#include <TimerOne.h>
#include <EEPROM.h>

const int b_reset = 2;
const int ventilador1 = 3;
const int ventilador2 = 4;
const int motor1 = 5;
const int motor2 = 6;
const int electrov1 = 7;
const int electrov2 = 8;
const int p_compresor = 9;
const int led_error = 10;
const int caiProducto = 11;
const int selec_monedas_led = 12;
const int selec_monedas_boton = 13;

const int pinUps = A0;
const int temp_1 = A1;
const int temp_2 = A2;
const int hum_1 = A3;
const int hum_2 = A4;

const unsigned long tCaida = 20000;
const unsigned long seguridadCaida = 1000;

enum CONTROL_HORAS
{
  B_COSUMO,
  MODO_ON
};
CONTROL_HORAS contr_h;

enum Estados_maqui
{
  BATERIA_OK,
  BAJA_TENSION,
  ERROR_T_Z1,
  ERROR_T_Z2,
  ERROR_H_Z1,
  ERROR_H_Z2
};

Estados_maqui estado_maquina;

enum Dispensador
{
  IMPORTE,
  M_PROD,
  SELECCION,
  COMPARACION,
  MOTOR,
  CAIDA,
  DEVO_IMPO,
  DEVO_CAM
};
Dispensador venta = IMPORTE;

typedef struct reloj
{
  byte hora;
  byte minuto;
  byte segundo;
  byte milisegundo;
};
reloj reloj1;

typedef struct producto
{
  char descripcion[15];
  char codigo[4];
  float precio;
  int stock;
  int pin_motor;
  unsigned long int ton;
};

typedef struct sensor
{
  int pin;
  float histeresis;
  float objetivo;
  float peligro;
  float min;
  float max;
  unsigned long ms_error;
  float ofset;
};

typedef struct lectura_varios
{
  float ups;
  float temperatura[2];
  float humedad[2];
};
lectura_varios l_varios;

typedef struct errores_varios
{
  float ups;
  float temperatura[2];
  float humedad[2];
};
errores_varios e_varios;

typedef struct control_zonas
{
  sensor humedad;
  sensor temperatura;
  producto productos[5];
  bool refrigeracion;
  bool ventilador;
};

typedef struct vending
{
  reloj on;
  reloj of;
  reloj reloj_actual;
  control_zonas zonas[2];
  sensor ups;
  float credito;
  float cambio;
  bool power_mode;
};
vending maquina;


void serial_delay(String *str, unsigned long tiempo);
void millisReloj(void);
float lectura(sensor *temp);
bool estado(sensor *temp, float *lectura);
bool error_ups(sensor *temp, float *lectura);
bool error(sensor *temp, float *lectura, unsigned long *cont);
int parpadeoLed(unsigned long ton, unsigned long tof);
unsigned long  leerBoton(int input);
bool comparacion(float precio, float importe);
float importe(unsigned long tiempo);
float cambio(float precio, float euro);
float cambioSeg(float cam);
byte serialEvent(String *com);
reloj config_horario(String *cmd);
void config_parametros_temp(String *cmd, sensor *temp);
int localicar_producto(producto *temp, String *str);
CONTROL_HORAS horario(reloj *on, reloj *of, reloj *actual);
float parametro_introducido(String *cmd, String delimitador);
int localizador_zona(String *cmd);

unsigned long con_temp_z1;
unsigned long con_temp_z2;
unsigned long con_hum_z1;
unsigned long con_hum_z2;
unsigned long conMotor;
unsigned long conCambio;
unsigned long camTiempo;
unsigned long conCaida;
unsigned long cont_res;

int parpadeo;
byte blinkCount = 0;
byte horaActual_int[3];
char *p_tr;
int i;
String comando = "";
int tpulsado;
int zona;
int posi;
String pro_intro = "";
bool comp;
float cambEuro;
int eedirec = 0;
bool aux_error = false;

void setup()
{
  pinMode(b_reset, INPUT_PULLUP);
  pinMode(ventilador1, OUTPUT);
  pinMode(ventilador2, OUTPUT);
  pinMode(motor1, OUTPUT);
  pinMode(motor2, OUTPUT);
  pinMode(electrov1, OUTPUT);
  pinMode(electrov2, OUTPUT);
  pinMode(p_compresor, OUTPUT);
  pinMode(led_error, OUTPUT);
  pinMode(selec_monedas_led, OUTPUT);
  pinMode(caiProducto, INPUT_PULLUP);
  pinMode(selec_monedas_led, OUTPUT);
  pinMode(selec_monedas_boton, INPUT_PULLUP);
  Serial.begin(9600);

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  Timer1.initialize(100000);
  Timer1.attachInterrupt(millisReloj);

  char horaActual_char[9] PROGMEM = __TIME__;
  p_tr = strtok(horaActual_char, ":");
  while (p_tr != NULL)
  {
    horaActual_int[i++] = atoi(p_tr);
    p_tr = strtok(NULL, ":");
  }
  if (i = 3)
    i = 0;

  reloj1 = {horaActual_int[0], horaActual_int[1], horaActual_int[2], 0};

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /*
    maquina.cambio = 0.0;
    maquina.credito = 0.0;

    //reloj = {h, m, s, ms};
    maquina.on = {6, 30, 0, 0};
    maquina.of = {23, 59, 0, 0};
    maquina.reloj_actual = {0, 0, 0, 0};
    maquina.power_mode = true;

    //sensor = {pin, histe, obj, peli, min, max, ms_er, ofs}
    maquina.ups = {pinUps, 0, 0, 10.0, 0.0, 12.0, 3000, 0.0};
    maquina.zonas[0].temperatura = {temp_1, 0.75, 16.0, 25, -10.0, 40.0, 2000, 0.0};
    maquina.zonas[1].temperatura = {temp_2, 0.75, 22.0, 25, -10.0, 40.0, 2000, 0.0};
    maquina.zonas[0].humedad = {hum_1, 5.0, 20.0, 30.0, 0.0, 100.0, 6000, 1};
    maquina.zonas[1].humedad = {hum_2, 5.0, 20.0, 30.0, 0.0, 100.0, 6000, 1};

    //producto = {desc, cod, preci, stock, pin, ton}
    strcpy(maquina.zonas[0].productos[0].descripcion, "Coca Cola");
    strcpy(maquina.zonas[0].productos[1].descripcion, "Fanta Naranja");
    strcpy(maquina.zonas[0].productos[2].descripcion, "Zumo de Limon");
    strcpy(maquina.zonas[0].productos[3].descripcion, "Zumo de Piña");
    strcpy(maquina.zonas[0].productos[4].descripcion, "Agua Mineral");
    strcpy(maquina.zonas[1].productos[0].descripcion, "Patatas fritas");
    strcpy(maquina.zonas[1].productos[1].descripcion, "Cacahuetes");
    strcpy(maquina.zonas[1].productos[2].descripcion, "Pistachos");
    strcpy(maquina.zonas[1].productos[3].descripcion, "Pipas");
    strcpy(maquina.zonas[1].productos[4].descripcion, "Maices");

    strcpy(maquina.zonas[0].productos[0].codigo, "A35");
    strcpy(maquina.zonas[0].productos[1].codigo, "A36");
    strcpy(maquina.zonas[0].productos[2].codigo, "A37");
    strcpy(maquina.zonas[0].productos[3].codigo, "A38");
    strcpy(maquina.zonas[0].productos[4].codigo, "A39");
    strcpy(maquina.zonas[1].productos[0].codigo, "A40");
    strcpy(maquina.zonas[1].productos[1].codigo, "A41");
    strcpy(maquina.zonas[1].productos[2].codigo, "A42");
    strcpy(maquina.zonas[1].productos[3].codigo, "A43");
    strcpy(maquina.zonas[1].productos[4].codigo, "A44");

    maquina.zonas[0].productos[0].precio = 1.50;
    maquina.zonas[0].productos[1].precio = 1.50;
    maquina.zonas[0].productos[2].precio = 1.50;
    maquina.zonas[0].productos[3].precio = 1.50;
    maquina.zonas[0].productos[4].precio = 2.50;
    maquina.zonas[1].productos[0].precio = 1.25;
    maquina.zonas[1].productos[1].precio = 1.00;
    maquina.zonas[1].productos[2].precio = 2.50;
    maquina.zonas[1].productos[3].precio = 1.00;
    maquina.zonas[1].productos[4].precio = 3.00;

    maquina.zonas[0].productos[0].stock = 100;
    maquina.zonas[0].productos[1].stock = 100;
    maquina.zonas[0].productos[2].stock = 100;
    maquina.zonas[0].productos[3].stock = 100;
    maquina.zonas[0].productos[4].stock = 100;
    maquina.zonas[1].productos[0].stock = 100;
    maquina.zonas[1].productos[1].stock = 100;
    maquina.zonas[1].productos[2].stock = 100;
    maquina.zonas[1].productos[3].stock = 100;
    maquina.zonas[1].productos[4].stock = 100;

    maquina.zonas[0].productos[0].pin_motor = motor1;
    maquina.zonas[0].productos[1].pin_motor = motor1;
    maquina.zonas[0].productos[2].pin_motor = motor1;
    maquina.zonas[0].productos[3].pin_motor = motor1;
    maquina.zonas[0].productos[4].pin_motor = motor1;
    maquina.zonas[1].productos[0].pin_motor = motor2;
    maquina.zonas[1].productos[1].pin_motor = motor2;
    maquina.zonas[1].productos[2].pin_motor = motor2;
    maquina.zonas[1].productos[3].pin_motor = motor2;
    maquina.zonas[1].productos[4].pin_motor = motor2;

    maquina.zonas[0].productos[0].ton = 5000;
    maquina.zonas[0].productos[1].ton = 5000;
    maquina.zonas[0].productos[2].ton = 5000;
    maquina.zonas[0].productos[3].ton = 5000;
    maquina.zonas[0].productos[4].ton = 5000;
    maquina.zonas[1].productos[0].ton = 5000;
    maquina.zonas[1].productos[1].ton = 5000;
    maquina.zonas[1].productos[2].ton = 5000;
    maquina.zonas[1].productos[3].ton = 5000;
    maquina.zonas[1].productos[4].ton = 5000;
  */
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /*
    eedirec = 0;
    EEPROM.put(eedirec, maquina);
    Serial.println("La eeprom se ha cargado");
    Serial.println("Ocupa: ");
    Serial.println(sizeof(maquina));

    eedirec += sizeof(maquina);
    EEPROM.put(eedirec, maquina);
    Serial.println("Se ha cargado la maquina de fabrica");
    eedirec = 0;
  */
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  eedirec = 0;
  EEPROM.get(eedirec, maquina);
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
  //Serial.println(sizeof(maquina));
  l_varios.ups = lectura(&maquina.ups);
  l_varios.temperatura[0] = lectura(&maquina.zonas[0].temperatura);
  l_varios.temperatura[1] = lectura(&maquina.zonas[1].temperatura);
  l_varios.humedad[0] = lectura(&maquina.zonas[0].humedad);
  l_varios.humedad[1] = lectura(&maquina.zonas[1].humedad);

  e_varios.ups = error_ups(&maquina.ups, &l_varios.ups);
  if (e_varios.ups)
    estado_maquina = BAJA_TENSION;
  else
    estado_maquina = BATERIA_OK;

  e_varios.temperatura[0] = error(&maquina.zonas[0].temperatura, &l_varios.temperatura[0], &con_temp_z1);
  if (e_varios.temperatura[0])
    estado_maquina = ERROR_T_Z1;

  e_varios.temperatura[1] = error(&maquina.zonas[1].temperatura, &l_varios.temperatura[1], &con_temp_z2);
  if (e_varios.temperatura[1])
    estado_maquina = ERROR_T_Z2;

  e_varios.humedad[0] = error(&maquina.zonas[0].humedad, &l_varios.humedad[0], &con_hum_z1);
  if (e_varios.humedad[0])
    estado_maquina = ERROR_H_Z1;

  e_varios.humedad[1] = error(&maquina.zonas[1].humedad, &l_varios.humedad[1], &con_hum_z2);
  if (e_varios.humedad[1])
    estado_maquina = ERROR_H_Z2;

  switch (estado_maquina)
  {
    case BAJA_TENSION:
      //parpadeoLed(ton, tof);
      parpadeo = parpadeoLed(1000, 5000);
      digitalWrite(led_error, parpadeo);
      digitalWrite(electrov1, LOW);
      digitalWrite(electrov2, LOW);
      digitalWrite(ventilador1, LOW);
      digitalWrite(ventilador2, LOW);
      digitalWrite(p_compresor, LOW);
      comando = "Error de alimentacion";
      serial_delay(&comando, 2000UL);
      comando = "";
      break;

    case ERROR_T_Z1:
      parpadeo = parpadeoLed(1000, 1000);
      digitalWrite(led_error, parpadeo);
      comando = "Error refrigeracion Zona 1";
      serial_delay(&comando, 2000UL);
      comando = "";
      break;

    case ERROR_T_Z2:
      parpadeo = parpadeoLed(1000, 1000);
      digitalWrite(led_error, parpadeo);
      comando = "Error refrigeracion Zona 2";
      serial_delay(&comando, 2000UL);
      comando = "";
      break;

    case ERROR_H_Z1:
      parpadeo = parpadeoLed(5000, 1000);
      digitalWrite(led_error, parpadeo);
      comando = "Error humedad Zona 1";
      serial_delay(&comando, 2000UL);
      comando = "";
      break;

    case ERROR_H_Z2:
      parpadeo = parpadeoLed(5000, 1000);
      digitalWrite(led_error, parpadeo);
      comando = "Error humedad Zona 2";
      serial_delay(&comando, 2000UL);
      comando = "";
      break;

    case BATERIA_OK:
      maquina.zonas[0].refrigeracion = estado(&maquina.zonas[0].temperatura, &l_varios.temperatura[0]);
      maquina.zonas[1].refrigeracion = estado(&maquina.zonas[1].temperatura, &l_varios.temperatura[1]);
      maquina.zonas[0].ventilador = estado(&maquina.zonas[0].humedad, &l_varios.humedad[0]);
      maquina.zonas[1].ventilador = estado(&maquina.zonas[1].humedad, &l_varios.humedad[1]);

      digitalWrite(electrov1, maquina.zonas[0].refrigeracion);
      digitalWrite(electrov2, maquina.zonas[1].refrigeracion);
      digitalWrite(ventilador1, maquina.zonas[0].ventilador);
      digitalWrite(ventilador2, maquina.zonas[1].ventilador);

      if ((maquina.zonas[0].refrigeracion == true) || (maquina.zonas[1].refrigeracion == true))
        digitalWrite(p_compresor, HIGH);
      else
        digitalWrite(p_compresor, LOW);

      if (venta != SELECCION) {
        if (serialEvent(&comando) == 1)
        {
          Serial.println("ECO: " + comando);
          if (comando == "HELP")
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
            Serial.println(F("Siendo H.XX:                         -> La histeresis"));
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
            Serial.println(F("VER_CONFIG                           -> Muestra la configuracion de la maquina actual"));
            Serial.println(F("RESET                                -> Vuelve la maquina a la configuracion de serie"));
            Serial.println(F("----------------------------------------------------------------------------------------------------"));
            comando = "";
          }
          else if (comando.substring(0, (comando.indexOf(" "))) == "M_ON")
          {
            maquina.power_mode = true;
            EEPROM.put(0, maquina);
            Serial.println(F("Se han actualizado los datos en la eeprom"));
            comando = "";
          }
          else if (comando.substring(0, (comando.indexOf(" "))) == "M_OFF")
          {
            maquina.power_mode = false;
            EEPROM.put(0, maquina);
            Serial.println(F("Se han actualizado los datos en la eeprom"));
            comando = "";
          }
          else if (comando.substring(0, (comando.indexOf(" "))) == "SET_TIME_ON")
          {
            maquina.on = config_horario(&comando);
            Serial.print(F("La hora introducida de inicio es: "));
            Serial.print(maquina.on.hora);
            Serial.print(F(":"));
            Serial.print(maquina.on.minuto);
            Serial.print(F(":"));
            Serial.println(maquina.on.segundo);
            EEPROM.put(0, maquina);
            Serial.println(F("Se han actualizado los datos en la eeprom"));
            comando = "";
          }
          else if (comando.substring(0, (comando.indexOf(" "))) == "SET_TIME_OF")
          {
            maquina.of = config_horario(&comando);
            Serial.print(F("La hora introducida de bajo consumo es: "));
            Serial.print(maquina.of.hora);
            Serial.print(":");
            Serial.print(maquina.of.minuto);
            Serial.print(":");
            Serial.println(maquina.of.segundo);
            EEPROM.put(0, maquina);
            Serial.println(F("Se han actualizado los datos en la eeprom"));
            comando = "";
          }
          else if (comando.substring(0, (comando.indexOf(" "))) == "SET_TEM")
          {
            zona = localizador_zona(&comando);
            maquina.zonas[zona].temperatura.objetivo = parametro_introducido(&comando, ":");
            Serial.print(F("Se ha modificado la temperatura objetivo en la zona: "));
            Serial.println(zona + 1);
            Serial.print(F("La temperatura objetivo introducida es: "));
            Serial.println(maquina.zonas[zona].temperatura.objetivo);
            EEPROM.put(0, maquina);
            Serial.println(F("Se han actualizado los datos en la eeprom"));
            comando = "";
          }
          else if (comando.substring(0, (comando.indexOf(" "))) == "SET_HUM")
          {
            zona = localizador_zona(&comando);
            maquina.zonas[zona].humedad.objetivo = parametro_introducido(&comando, ":");
            Serial.print(F("Se ha modificado la humedad objetivo en la zona: "));
            Serial.println(zona + 1);
            Serial.print(F("La humead objetivo introducida es: "));
            Serial.println(maquina.zonas[zona].humedad.objetivo);
            EEPROM.put(0, maquina);
            Serial.println(F("Se han actualizado los datos en la eeprom"));
            comando = "";
          }
          else if (comando.substring(0, (comando.indexOf(" "))) == "CONFIG_HUM")
          {
            zona = localizador_zona(&comando);
            maquina.zonas[zona].humedad.ofset = parametro_introducido(&comando, ":");
            Serial.print(F("Se ha modificado la ofset de la humedad en la zona: "));
            Serial.println(zona + 1);
            Serial.print(F("El ofset introducido es: "));
            Serial.println(maquina.zonas[zona].humedad.ofset);
            EEPROM.put(0, maquina);
            Serial.println(F("Se han actualizado los datos en la eeprom"));
            comando = "";
          }
          else if (comando.substring(0, (comando.indexOf(" "))) == "CONFIG_TEM")
          {
            zona = localizador_zona(&comando);
            config_parametros_temp(&comando, &maquina.zonas[zona].temperatura);
            Serial.print(F("Se ha modificado la configuracion de la zona: "));
            Serial.println(zona + 1);
            Serial.print(F("La temperatura minima introducida es: "));
            Serial.println(maquina.zonas[zona].temperatura.min);
            Serial.print(F("La temperatura maxima introducida es: "));
            Serial.println(maquina.zonas[zona].temperatura.max);
            Serial.print(F("La histerisis introducida es: "));
            Serial.println(maquina.zonas[zona].temperatura.histeresis);
            EEPROM.put(0, maquina);
            Serial.println(F("Se han actualizado los datos en la eeprom"));
            comando = "";
          }
          else if (comando.substring(0, (comando.indexOf(" "))) == "STOCK")
          {
            zona = localizador_zona(&comando);
            String str = comando.substring((comando.indexOf("-")) + 1);
            //str.trim();
            int posicion = localicar_producto(maquina.zonas[zona].productos, &str);
            int stock = parametro_introducido(&comando, ":");
            maquina.zonas[zona].productos[posicion].stock = stock;
            Serial.print(F("Se ha modificado el stock del producto: "));
            Serial.println(maquina.zonas[zona].productos[posicion].descripcion);
            Serial.print(F("El stock actual es: "));
            Serial.println(maquina.zonas[zona].productos[posicion].stock);
            EEPROM.put(0, maquina);
            Serial.println(F("Se han actualizado los datos en la eeprom"));
            comando = "";
          }
          else if (comando.substring(0, (comando.indexOf(" "))) == "HORA")
          {
            Serial.print(F("La hora atual es:"));
            Serial.print(maquina.reloj_actual.hora);
            Serial.print(F(":"));
            Serial.print(maquina.reloj_actual.minuto);
            Serial.print(F(":"));
            Serial.println(maquina.reloj_actual.segundo);
            comando = "";
          }
          else if (comando.substring(0, (comando.indexOf(" "))) == "VER_STOCK")
          {
            for (int r = 0; r < 2; r++) {
              for (int w = 0; w < 5; w++) {
                Serial.println(maquina.zonas[r].productos[w].descripcion);
                Serial.println(maquina.zonas[r].productos[w].stock);
              }
            }
            comando = "";
          }
          else if (comando.substring(0, (comando.indexOf(" "))) == "VER_CONFIG")
          {
            Serial.print(F("La hora de inicio es:"));
            Serial.print(maquina.on.hora);
            Serial.print(F(":"));
            Serial.print(maquina.on.minuto);
            Serial.print(F(":"));
            Serial.println(maquina.on.segundo);
            Serial.println();
            Serial.print(F("La hora de bajo consumo es:"));
            Serial.print(maquina.of.hora);
            Serial.print(F(":"));
            Serial.print(maquina.of.minuto);
            Serial.print(F(":"));
            Serial.println(maquina.of.segundo);
            Serial.println();
            Serial.println(F("La parametros de la zona 1 son:"));
            Serial.println(F("Temperatura:"));
            Serial.print(F("Objetivo: "));
            Serial.println(maquina.zonas[0].temperatura.objetivo);
            Serial.print(F("Minima: "));
            Serial.println(maquina.zonas[0].temperatura.min);
            Serial.print(F("Maxima: "));
            Serial.println(maquina.zonas[0].temperatura.max);
            Serial.print(F("Histeresis: "));
            Serial.println(maquina.zonas[0].temperatura.histeresis);
            Serial.println(F("Humedad:"));
            Serial.print(F("Objetivo: "));
            Serial.println(maquina.zonas[0].humedad.objetivo);
            Serial.print(F("Ofset: "));
            Serial.println(maquina.zonas[0].humedad.ofset);
            Serial.print(F("Histeresis: "));
            Serial.println(maquina.zonas[0].humedad.histeresis);
            Serial.println();
            Serial.println(F("La parametros de la zona 2 son:"));
            Serial.println(F("Temperatura:"));
            Serial.print(F("Objetivo: "));
            Serial.println(maquina.zonas[1].temperatura.objetivo);
            Serial.print(F("Minima: "));
            Serial.println(maquina.zonas[1].temperatura.min);
            Serial.print(F("Maxima: "));
            Serial.println(maquina.zonas[1].temperatura.max);
            Serial.print(F("Histeresis: "));
            Serial.println(maquina.zonas[1].temperatura.histeresis);
            Serial.println(F("Humedad:"));
            Serial.print(F("Objetivo: "));
            Serial.println(maquina.zonas[1].humedad.objetivo);
            Serial.print(F("Ofset: "));
            Serial.println(maquina.zonas[1].humedad.ofset);
            Serial.print(F("Histeresis: "));
            Serial.println(maquina.zonas[1].humedad.histeresis);
            Serial.println();
            if (maquina.power_mode == true)Serial.println(F("El control de horario esta activado."));
            else if (maquina.power_mode == false)Serial.println(F("El control de horario esta desactivado."));
            comando = "";
          }
          else if (comando.substring(0, (comando.indexOf(" "))) == "RESET")
          {
            aux_error = true;
            cont_res = millis();
            Serial.println(F("Pulsa el boton RESET durante al menos 1s:"));
            comando = "";
          }
          else
          {
            Serial.println(F("No has introducido un codigo valido"));
            comando = "";
          }
          //comando = "";
          Serial.println(F("----------------------------------------------------------------------------------------------------"));
        }

        if (aux_error == true) {
          if ((millis() - cont_res) <= 20000)
          {
            //tpulsado_res = leerBoton(b_reset);
            if (leerBoton(b_reset) > 1000)
            {
              eedirec = sizeof(maquina);
              EEPROM.get(eedirec, maquina);
              EEPROM.put(0, maquina);
              Serial.println("Se ha reseteado la maquina");
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

      if (maquina.power_mode == true) contr_h = horario(&maquina.on, &maquina.of, &maquina.reloj_actual);
      else if (maquina.power_mode == false) contr_h = MODO_ON;

      switch (contr_h)
      {
        case MODO_ON:
          switch (venta)
          {
            case IMPORTE:
              tpulsado = leerBoton(selec_monedas_boton);
              if (tpulsado > 0)
              {
                maquina.credito += importe(tpulsado);
                if (maquina.credito > 0)
                {
                  Serial.println(F("El importe introducidos es: "));
                  Serial.println(maquina.credito);
                  venta = M_PROD;
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
              venta = SELECCION;
              break;

            case SELECCION:
              if (serialEvent(&pro_intro) == 1)
              {
                zona = 0;
                Serial.println("ECO: " + pro_intro);
                posi = localicar_producto(maquina.zonas[zona].productos, &pro_intro);
                if (posi < 0) {
                  zona ++;
                  posi = localicar_producto(maquina.zonas[zona].productos, &pro_intro);
                  pro_intro = "";
                }
                else {
                  pro_intro = "";
                }

                if (posi >= 0)
                {
                  if (maquina.zonas[zona].productos[posi].stock >= 1) {
                    Serial.println(F("Has seleccionado el producto: "));
                    Serial.println(maquina.zonas[zona].productos[posi].descripcion);
                    Serial.println(F("Precio: "));
                    Serial.println(maquina.zonas[zona].productos[posi].precio);
                    Serial.println(F("****************************************************************"));
                    venta = COMPARACION;
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

            case COMPARACION:
              comp = comparacion(maquina.zonas[zona].productos[posi].precio, maquina.credito);
              if (comp == true)
              {
                Serial.println(F("Has introducido suficiente dinero"));
                Serial.println(F("****************************************************************"));
                conMotor = millis();
                venta = MOTOR;
              }
              else if (comp == false)
              {
                Serial.println(F("No has introducido suficiente dinero"));
                Serial.println(F("****************************************************************"));
                venta = IMPORTE;
              }
              break;

            case MOTOR:
              if (millis() - conMotor < maquina.zonas[zona].productos[posi].ton)
              {
                digitalWrite(maquina.zonas[zona].productos[posi].pin_motor, parpadeoLed(500, 500));
              }
              else
              {
                digitalWrite(maquina.zonas[zona].productos[posi].pin_motor, LOW);
                conCaida = millis();
                venta = CAIDA;
              }
              break;

            case CAIDA:
              if (millis() - conCaida <= tCaida)
              {
                if (leerBoton(caiProducto) >= seguridadCaida)
                {
                  maquina.zonas[zona].productos[posi].stock -= 1;
                  conCambio = millis();
                  EEPROM.put(0, maquina);
                  Serial.println(F("Se han actualizado los datos en la eeprom"));
                  venta = DEVO_CAM;
                }
              }
              else
              {
                conCambio = millis();
                venta = DEVO_IMPO;
              }
              break;

            case DEVO_IMPO:
              camTiempo = cambioSeg(maquina.credito);
              if (millis() - conCambio <= camTiempo)
              {
                digitalWrite(selec_monedas_led, HIGH);
              }
              else
              {
                Serial.println(F("El producto no ha sido dispensado."));
                Serial.println(F("El cambio es: "));
                Serial.println(maquina.credito);
                Serial.println(F("****************************************************************"));
                digitalWrite(selec_monedas_led, LOW);
                maquina.credito = 0;
                venta = IMPORTE;
              }
              break;

            case DEVO_CAM:
              cambEuro = cambio(maquina.zonas[zona].productos[posi].precio, maquina.credito);
              camTiempo = cambioSeg(cambEuro);
              if (millis() - conCambio <= camTiempo)
              {
                digitalWrite(selec_monedas_led, HIGH);
              }
              else
              {
                Serial.println(F("El producto ha sido dispensado."));
                Serial.println(F("El cambio es: "));
                Serial.println(cambEuro);
                Serial.print(F("Quedan:"));
                Serial.print(maquina.zonas[zona].productos[posi].stock);
                Serial.println(" unidades");
                Serial.println(F("****************************************************************"));
                digitalWrite(selec_monedas_led, LOW);
                maquina.credito = 0;
                venta = IMPORTE;
              }
              break;
          }
          break;

        case B_COSUMO:

          break;
      }
      break;
  }
}

void millisReloj(void)
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

  maquina.reloj_actual.milisegundo = reloj1.milisegundo;
  maquina.reloj_actual.segundo = reloj1.segundo;
  maquina.reloj_actual.minuto = reloj1.minuto;
  maquina.reloj_actual.hora = reloj1.hora;

  interrupts();
}

float lectura(sensor *temp)
{
  static float lectura;
  static float o_min = (1023.0 / 5.0) * temp->ofset;
  lectura = analogRead(temp->pin);
  lectura = mapFloat(lectura, o_min, 1023, temp->min, temp->max);
  return lectura;
}

bool estado(sensor *temp, float *lectura)
{
  static bool estado;
  if (*lectura < (temp->objetivo - temp->histeresis) && estado == true)
    estado = false;
  else if (*lectura > (temp->objetivo - temp->histeresis) && estado == false)
    estado = true;

  return estado;
}

bool error_ups(sensor *temp, float *lectura)
{
  static unsigned long cont;
  static bool error;
  if (*lectura > temp->peligro)
  {
    cont = millis();
    error = false;
  }
  else if (*lectura < temp->peligro)
  {
    if (millis() - cont >= temp->ms_error && *lectura < temp->peligro)
      error = true;
  }
  return error;
}

bool error(sensor *temp, float *lectura, unsigned long *cont)
{
  static bool error;
  if (*lectura < temp->peligro)
  {
    *cont = millis();
    error = false;
  }
  else if (*lectura > temp->peligro)
  {
    if (millis() - *cont >= temp->ms_error && *lectura > temp->peligro)
      error = true;
  }
  return error;
}

int parpadeoLed(unsigned long ton, unsigned long tof)
{
  static unsigned long tantes;
  static int estadoled;

  if (estadoled == HIGH)
  {
    if (millis() - tantes >= ton)
    {
      estadoled = LOW;
      tantes = millis();
    }
  }
  else if (estadoled == LOW)
  {
    if (millis() - tantes >= tof)
    {
      estadoled = HIGH;
      tantes = millis();
    }
  }
  return estadoled;
}

unsigned long leerBoton(int input)
{
  static int estadoAnterior = HIGH;
  static int estadoActual;
  static unsigned long cont;
  unsigned long tiempoPulsado = 0;
  estadoActual = digitalRead(input);

  if (estadoActual != estadoAnterior)
  {
    if (estadoAnterior == HIGH && estadoActual == LOW)
    {
      cont = millis();
    }
    else if (estadoAnterior == LOW && estadoActual == HIGH)
    {
      tiempoPulsado = millis() - cont;
    }
  }
  estadoAnterior = estadoActual;
  return tiempoPulsado;
}

bool comparacion(float precio, float importe)
{
  static bool comp;
  if (importe < precio)
    comp = false;
  if (importe > precio)
    comp = true;
  return comp;
}

float importe(unsigned long tiempo)
{
  static int cent;
  static float euro;
  if (tiempo >= 100)
  { //los tiempos estan modificados porque eran muy largos
    cent = 1 * (tiempo / 100);
    euro = 5 * cent * 0.01;
  }
  else
  {
    euro = 0;
  }
  return euro;
}

float cambio(float precio, float euro)
{
  static float cambio;
  if (euro >= precio)
  {
    cambio = (euro - precio);
  }
  else
  {
    cambio = euro;
  }
  return cambio;
}

float cambioSeg(float cam)
{
  static float camseg;
  camseg = (cam * 100 * 500) / 5;
  return camseg;
}

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

reloj config_horario(String *cmd)
{
  reloj temp;
  static byte p;
  p = (cmd->indexOf(" ")) + 1;
  temp.hora = cmd->substring(p, ":").toInt();
  temp.minuto = cmd->substring(p + 3, ":").toInt();
  temp.segundo = cmd->substring(p + 3 + 3, ":").toInt();
  temp.milisegundo = 0;

  return temp;
}

void config_parametros_temp(String *cmd, sensor *temp)
{
  static byte p;
  p = (cmd->indexOf(":")) + 1;
  temp->min = cmd->substring(p, ":").toInt();
  p = (cmd->indexOf(":", p)) + 1;
  temp->max = cmd->substring(p, ":").toInt();
  p = (cmd->indexOf(":", p)) + 1;
  temp->histeresis = cmd->substring(p).toFloat();
}

int localicar_producto(producto *temp, String *str)
{
  static int i;
  for (i = 0; i < 5; i++)
  {
    if (str->equals(temp[i].codigo))
    {
      return i;
    }
  }
  return -1;
}

CONTROL_HORAS horario(reloj *on, reloj *of, reloj *actual)
{
  CONTROL_HORAS c_h;
  //static bool h;
  if ((actual->hora >= on->hora) && (actual->hora <= of->hora))
  {
    if ((actual->hora == on->hora))
    {
      if ((actual->minuto >= on->minuto))
      {
        if ((actual->segundo >= on->segundo))
        {
          c_h = MODO_ON;
          //return true;
        }
      }
    }
    if ((actual->hora > on->hora))
    {

      if ((actual->hora < of->hora))
      {
        c_h = MODO_ON;
        //return true;
      }
      else if ((actual->hora == of->hora))
      {
        if ((actual->minuto <= of->minuto))
        {
          if ((actual->segundo <= of->segundo))
          {
            c_h = MODO_ON;
            //return true;
          }
        }
      }
    }
  }
  else c_h = B_COSUMO;
  return c_h;
}

float parametro_introducido(String *cmd, String delimitador)
{
  static byte p;
  static float temp;
  p = (cmd->indexOf(delimitador)) + 1;
  temp = (cmd->substring(p).toInt());
  return temp;
}

int localizador_zona(String *cmd)
{
  static byte p;
  static int temp;
  p = (cmd->indexOf(" ")) + 1;
  temp = (cmd->substring(p, p + 1).toInt());
  return temp - 1;
}

void serial_delay(String *str, unsigned long tiempo)
{
  static unsigned long cont;
  if (millis() - cont > tiempo) {
    Serial.println(*str);
    Serial.print(F("La hora atual es:"));
    Serial.print(maquina.reloj_actual.hora);
    Serial.print(F(":"));
    Serial.print(maquina.reloj_actual.minuto);
    Serial.print(F(":"));
    Serial.println(maquina.reloj_actual.segundo);
    cont = millis();
    *str = "";
  }
}
