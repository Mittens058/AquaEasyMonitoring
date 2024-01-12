



#define BLYNK_TEMPLATE_ID "TMPL6-kTjpHlS"
#define BLYNK_TEMPLATE_NAME "AquaEasy Monitoring"
#define BLYNK_AUTH_TOKEN "n_nR6fFi1MIEU5bvtcOj1VEqK8qsqaDg"

#include <OneWire.h>
#include <DallasTemperature.h>

#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

char auth[] = BLYNK_AUTH_TOKEN;

char ssid[] = "TP-Link_Extender";  // type your wifi name
char pass[] = "";  // type your wifi password

BlynkTimer timer; //import blynk timer function


// GPIO where the DS18B20 is connected to
const int oneWireBus = 33;     

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

const int trigPin = 34;  // pins for ultrasonic sensor
const int echoPin = 35;

#define TdsSensorPin 32
#define VREF 3.3              // analog reference voltage(Volt) of the ADC
#define SCOUNT  100            // sum of sample point

int analogBuffer[SCOUNT];     // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;
int copyIndex = 0;

#define SOUND_SPEED 0.034   //needed calculations for distnace on ultrasonic sensor
#define CM_TO_INCH 0.393701
long duration;
float distanceCm;
float distanceInch;

float averageVoltage = 0;
float tdsValue = 0;
float temperature = 25;       // current temperature for compensation

int timedelay = 500;


// median filtering algorithm for tds calculation in ppm
int getMedianNum(int bArray[], int iFilterLen){
  int bTab[iFilterLen];
  for (byte i = 0; i<iFilterLen; i++)
  bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0){
    bTemp = bTab[(iFilterLen - 1) / 2];
  }
  else {
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  }
  return bTemp;
}


void sendSensor()
{
  static unsigned long analogSampleTimepoint = millis();
  if(millis()-analogSampleTimepoint > 40U){     //every 40 milliseconds,read the analog value from the ADC
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);    //read the analog value and store into the buffer
    analogBufferIndex++;
    if(analogBufferIndex == SCOUNT){ 
      analogBufferIndex = 0;
    }
  }   
  sensors.requestTemperatures(); 

  float temperatureC = sensors.getTempCByIndex(0);
  float temperatureF = sensors.getTempFByIndex(0);
  Serial.print(temperatureC);
  Serial.println("ºC");

  Blynk.virtualWrite(V0,temperatureC);
  delay(timedelay);

  static unsigned long printTimepoint = millis();
  if(millis()-printTimepoint > 800U){
    printTimepoint = millis();
    for(copyIndex=0; copyIndex<SCOUNT; copyIndex++){
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
      
      // read the analog value more stable by the median filtering algorithm, and convert to voltage value
      averageVoltage = getMedianNum(analogBufferTemp,SCOUNT) * (float)VREF / 4096.0;
      
      //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0)); 
      float compensationCoefficient = 1.0+0.02*(temperature-25.0);
      //temperature compensation
      float compensationVoltage=averageVoltage/compensationCoefficient;
      
      //convert voltage value to tds value
      tdsValue=(133.42*compensationVoltage*compensationVoltage*compensationVoltage - 255.86*compensationVoltage*compensationVoltage + 857.39*compensationVoltage)*0.5;


      int tds_ppt = tdsValue*1000000; // ppt value nokwa etuy 

      //Serial.print("voltage:");
      //Serial.print(averageVoltage,2);
      //Serial.print("V   ");
      Serial.print("TDS Value:");
      Serial.print(tdsValue,0);
      Serial.println("ppm");

      Blynk.virtualWrite(V2, tdsValue ); //can change tds value to ppt value based on needed

      delay(timedelay);

    }
  }
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  
  // Calculate the distance
  distanceCm = duration * SOUND_SPEED/2;
  
  // Convert to inches
  distanceInch = distanceCm * CM_TO_INCH;

  int sonicdistance = 8; // max water level
  // distance of ultrasonic sensor max level = 12 - 8 in = 4

  float percentage_level = 100 - ((distanceInch-4)/(sonicdistance))*100; // water percentage calculations
  
  // Prints the distance in the Serial Monitor
  Serial.print("Distance (cm): ");
  Serial.println(distanceCm);
  Serial.print("Distance (inch): ");
  Serial.println(distanceInch);

  Serial.print("Water Level: ");
  Serial.println(percentage_level);

  Blynk.virtualWrite(V0,temperatureC);
  delay(timedelay);
}

void setup(){
  Serial.begin(115200);
  pinMode(TdsSensorPin,INPUT);
  Blynk.begin(auth, ssid, pass);
  timer.setInterval(100L, sendSensor);//send delay for blynk in miliseconds
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output` 
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input

  // Start the DS18B20 sensor
  sensors.begin();
}

void loop(){
  
  Blynk.run();
  timer.run();

}