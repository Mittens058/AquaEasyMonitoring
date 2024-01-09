//DEVICE CONFIGURATION IN BLYNK CONSOLE
#define BLYNK_TEMPLATE_ID "TMPL6-kTjpHlS"
#define BLYNK_TEMPLATE_NAME "AquaEasy Monitoring"
#define BLYNK_AUTH_TOKEN "n_nR6fFi1MIEU5bvtcOj1VEqK8qsqaDg"

//INCLUDE ALL NECCESSARY LIBRARIES
#include <OneWire.h>//for temperature
#include <DallasTemperature.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>


//WIFI CREDENTIALS and Blynnk Auth
char auth[] = "YourAuthToken"; // Enter your Blynk authentication token
char ssid[] = "TP-Link_Extender"; // Enter your WiFi network name
char pass[] = ""; // Enter your WiFi password

//include teh blynk eme eme


//something to have the data in a google sheet

//DEFINE ALL PARAMETERS AND SENSORS ASSIGN TO PINS
const int oneWireBus = 33;  //for temperature
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

#define PH_PIN 32 //ph level pin

//ultrasonic sensor
const int trigPin = 31;
const int echoPin = 30;

//DEFINE RELAY PINS ON DIGITAL MODE
#define filterrelay 15
#define aeratorrelay 16
#define solenoidrelay 17
#define uv_relay 18

//define parameter and variables
float voltage,phValue,temperature = 25; //ph level parameter
float acidVoltage = 2021;
float neutralVoltage = 1580;

const float minPH = 4.0;  // Minimum acceptable pH level
const float maxPH = 8.0;  // Maximum acceptable pH level
const float minTEMP = 28.0;  // Minimum acceptable pH level
const float maxTEMP = 32.0; //max temp

float Celsius = 0; //temp initial value

#define SOUND_SPEED 0.034 //parameters for the ultrasonic sensor
#define CM_TO_INCH 0.393701

long duration;
float distanceCm;
float distanceInch;

BlynkTimer timer; 


void myTimer() {
  float tempValue = readTEMPSensor(); // Read temperature sensor
  float phValue = readPHSensor(); // Read pH sensor
  float waterlevelpercent = 100 - ((distanceInch - 4))/100;

  Blynk.virtualWrite(V0, tempValue); // Send temperature data to virtual pin V0
  Blynk.virtualWrite(V3, phValue); // Send pH data to virtual pin V3
  Blynk.virtualWrite(V4, waterlevelpercent); // Send distance data to virtual pin V4
}



//setup functions
void setup()
{
  Serial.begin(115200);
  sensors.begin();

  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input

  pinMode(solenoidrelay, OUTPUT); // Set solenoid relay pin as OUTPUT
  digitalWrite(solenoidrelay, LOW); // Initialize relay as OFF

  pinMode(aeratorrelay, OUTPUT); // Set solenoid relay pin as OUTPUT
  digitalWrite(aeratorrelay, LOW); // Initialize relay as OFF

  pinMode(filterrelay, OUTPUT); // Set solenoid relay pin as OUTPUT
  digitalWrite(filterrelay, LOW); // Initialize relay as OFF

  pinMode(uv_relay, OUTPUT); // Set solenoid relay pin as OUTPUT
  digitalWrite(uv_relay, LOW); // Initialize relay as OFF


  timer.setInterval(1000L, myTimer); 

  Blynk.begin(auth, ssid, pass);
}

//create sensor function

float readTEMPSensor() { //for temperature sensor
  sensors.requestTemperatures(); 
  float temperatureC = sensors.getTempCByIndex(0);
  Serial.print(temperatureC);
  Serial.println("ºC");

  Blynk.virtualWrite(V0,temperatureC);

  return Celsius;
}

float readPHSensor() {
   static unsigned long timepoint = millis();
  if(millis()-timepoint>1000U){
    timepoint = millis();
        //temperature = readTemperature();         // read your temperature sensor to execute temperature compensation
        voltage = analogRead(PH_PIN)/1024.0*5000;

        float slope = (7.0-4.0)/((neutralVoltage-1500)/3.0 - (acidVoltage-1500)/3.0);
        float intercept = 7.0 - slope*(neutralVoltage-1500)/3.0;

        phValue = slope*(voltage-1500)/3.0+intercept;
        Blynk.virtualWrite(V3,phValue);

        Serial.print("Voltage: ");
        Serial.print(voltage,1);
        Serial.print("    pH:");
        Serial.println(phValue,2);  
  // Implement code to read pH sensor value
  // ...
  // Replace this placeholder with the actual code from your DFRobot pH sensor library
  // or manual to read the sensor value correctly.
  return phValue;
  }
}

long getDistance() { // for ultrasonic sensor
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH);
  float distanceCm = duration * SOUND_SPEED / 2;
  float distanceInch = distanceCm * CM_TO_INCH;
  
  Serial.print("Distance (cm): ");
  Serial.println(distanceCm);
  Serial.print("Distance (inch): ");
  Serial.println(distanceInch);
  
  float waterlevelpercent = 100 - ((distanceInch - 4))/100;
  Blynk.virtualWrite(V4,waterlevelpercent);
  
  Serial.print("Water Level Percentage: ");
  Serial.print(waterlevelpercent);

  if (waterlevelpercent < 100) {
    digitalWrite(solenoidrelay, HIGH); // Activate relay if distance is below the threshold
  } else {
    digitalWrite(solenoidrelay, LOW); // Deactivate relay otherwise
  }

  return duration;
}


//create automation function for each control

//loop functions
void loop()
{
  phValue = readPHSensor();

  if (phValue < minPH || phValue > maxPH) {
    digitalWrite(filterrelay, HIGH);  // Turn on relay
    digitalWrite(uv_relay, HIGH);
    Serial.println("pH out of range! Relay activated.");
  } else {
    digitalWrite(filterrelay, LOW);   // Turn off relay
    digitalWrite(uv_relay, LOW);
    Serial.println("pH within range. Relay deactivated.");
  }

  Serial.print("pH Value: ");
  Serial.println(phValue);

  delay(2000);  // Delay between readings

  Celsius = readTEMPSensor();

  if (Celsius < minTEMP || Celsius > maxTEMP) {
    digitalWrite(aeratorrelay, HIGH);
    Serial.println("TEMP out of range! Relay activated.");
  } else {
    digitalWrite(aeratorrelay, LOW);
    Serial.println("TEMP within range. Relay deactivated.");
  }

  Serial.print("Temp Value: ");
  Serial.println(Celsius);

  delay(2000);  // Delay between readings


  //IN THE ULTRASONIC SENSOR
  long duration = getDistance(); // ultrasonic sensor
  delay(1000);

  Blynk.run();
  timer.run();
}