/**
 * Proyecto clima ESP8266
 * Lee datos climáticos de un sensor y los pone en
 * una base de datos.
 * Plataforma ESP8266
 *  Patricio Coronado Collado (PACOCO)
 */
/**************************************************************
  Dirección de la base de datos y datos de la WIFI
 **************************************************************/
#define BASE_DE_DATOS sprintf(web,"http://patriciocoronadocollado.000webhostapp.com/clima.php/?valor1=%f&valor2=%f&ubicacion=%s",temperatura,humedad,lugar);
#define WIFI_PUEBLO WiFiMulti.addAP("FX-991SP","Doniga_93");
#define WIFI_MADRID WiFiMulti.addAP("router_no_encontrado", "RoloTomasi8086");
/**************************************************************
            Tiempo en deepsleep
 **************************************************************/
#define SEGUNDOS_DEEPSLEEP 60*30 //Media Hora
/*************************************************************
					ubicación del sensor
**************************************************************/
#define __Pueblo //Para usar la wifi del pueblo
//#define __Madrid //Para usar la wifi de Madrid
char lugar[]="Pueblo-Desvan";
/************************************************************
    Sensor a utilizar. Comentar el que no sea
 ************************************************************/
#define __DHT22
//#define __SHT11
/************************************************************
 *  Librerias
 ************************************************************/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
/************************************************************
			declaración de funciones
*************************************************************/
void entra_en_modo_deepsleep(int);
void imprime_variables(void);
boolean lee_sensor_dht22(void);
/*************************************************************
//declaracines para el sensor DHT22
**************************************************************/
#ifdef __DHT22
  #include "DHTesp.h" 
  DHTesp dht;
  TempAndHumidity Datos; // estructura donde se leen los datos del sensor
  #define DHTPIN 4 
#endif
/*************************************************************
//declaracines para el sensor SH11
**************************************************************/
#ifdef __SHT11
  #include "SHT1x.h"//Sensor
  SHT1x sht1x(dataPin, clockPin);
#endif
/*************************************************************
 *       Pines
 ************************************************************/
  #ifdef __SHT11
  #define dataPin  4 //Pin datos
  #define clockPin 2 //Pin de clk solo para el SHT11
#endif
/**************************************************************
 *    Variables
 **************************************************************/
float temperatura,humedad;
char web[256]; //para ala URL
boolean lecturaSensor=false;//Para comprobar la lectura del DHT22
bool depuracion=false;//true para depurar por el serial
//bool depuracion=true;//true para depurar por el serial

/*************************************************************
 * Macros (no se usan)
**************************************************************/
#define DEPURACION(X,Y) if(depuracion) Serial.print(X);\
 Serial.println(Y); Serial.flush();
/*************************************************************
					declaracines para el wifi
**************************************************************/
ESP8266WiFiMulti WiFiMulti;
/*************************************************************
						SETUP
*************************************************************/
void setup() 
{
  
  Serial.begin(76800);
  Serial.flush();
  #ifdef __Madrid
      WIFI_MADRID
  #endif
  #ifdef __Pueblo
      WIFI_PUEBLO
  #endif
  #ifdef __DHT22
    dht.setup(DHTPIN, DHTesp::DHT22); //Inicializa el DHT22
  #endif
}
void loop()
 {
   #ifdef __SHT11
    humedad = sht1x.readHumidity();
    temperatura = sht1x.readTemperatureC();
   #endif
   #ifdef __DHT22 //.................................................................
   lecturaSensor=lee_sensor_dht22();
   int contadorDHT22=1;
   while(!lecturaSensor)
   {
     delay(2000);
     lecturaSensor=lee_sensor_dht22();
     if(depuracion) Serial.printf("intentando leer el DHT22 %d veces...",contadorDHT22);
    if(++contadorDHT22>=10) 
    {
      Serial.printf("no pude leer el DHT22...by");
      entra_en_modo_deepsleep(SEGUNDOS_DEEPSLEEP);
    }
   }
   humedad = Datos.humidity;
   temperatura = Datos.temperature;
   #endif //........................................................................
  if(depuracion) imprime_variables();
  //Conecta con la wifi
  int contadorWIFI =1;
  while((WiFiMulti.run() != WL_CONNECTED))
  {
    delay(2000);
    if(depuracion) Serial.printf("intentando conectar con la wifi %d veces...",contadorWIFI);
    if(++contadorWIFI>=10) 
    {
      Serial.printf("no pude conectar con la wifi...");
      entra_en_modo_deepsleep(SEGUNDOS_DEEPSLEEP);
    }
  }
  {//Si conecta con la wifi
      WiFiClient client;
      HTTPClient http;
      if(depuracion) Serial.print("[HTTP] begin...\n");
		  BASE_DE_DATOS //dirección de la base de datos en web
      http.begin(client,web); //HTTP
      if(depuracion) Serial.print("[HTTP] GET...\n");
      // start connection and send HTTP header
      int httpCode = http.GET();
      // httpCode will be negative on error
      if(httpCode > 0) //Si la comunicación tiene éxito.........
      {
        if(depuracion) Serial.printf("HTTP CODE del GET = code: %d\n", httpCode);
        if(httpCode == HTTP_CODE_OK) //..Lee la respuesta
        {
                String payload = http.getString();
                if(1/*depuracion*/) //Comprueba si recibimos el #OK
                {
                    if (payload.indexOf("#OK")!=-1) Serial.println("#0K");
						        else Serial.println("#ERROR");
                    Serial.flush();
                }
        }
       } else //Si la comunicación no ha ido bien.....
       {
            Serial.printf(" ... fallo en con web, error: %s\n", http.errorToString(httpCode).c_str());
            Serial.flush();
        }

      http.end();//Cierra la conexión
  }
    
  entra_en_modo_deepsleep(SEGUNDOS_DEEPSLEEP);//Segundos a deepsleep
}
/****************************************************************
  funciones
 ***************************************************************/
/****************************************************************
 *  Envía por el puerto serie temperatura y humedad
 ***************************************************************/
void imprime_variables(void)
{
      Serial.flush();
      Serial.print("Temperatura: ");
      Serial.print(temperatura);
      Serial.println(" *C");
      Serial.flush();
      Serial.print("Humedad relativa: ");
      Serial.print(humedad);
      Serial.println("%");
      Serial.flush();
} 
/****************************************************************
 *  Entra en modo deepsleep
 ***************************************************************/
void entra_en_modo_deepsleep(int segundos)
{
	#define uS_SEGUNDOS 1000000  // factor de conversion de micro seconds a seconds
  ESP.deepSleep(uS_SEGUNDOS*segundos);
}
/*************************************************************
		funcion que lee el sensor DHT22
**************************************************************/
boolean lee_sensor_dht22(void)
{
	Datos = dht.getTempAndHumidity();
	delay(500);
	// Check if any reads failed and exit early (to try again).
	if (dht.getStatus() != 0) 
	{
		if(depuracion)
		{Serial.println("¡error de status en el sensor!: " + String(dht.getStatusString()));}
		return false;
	}
	else return true;
}
/****************************************************************
 
 ***************************************************************/
/******************  FIN  **************************************/