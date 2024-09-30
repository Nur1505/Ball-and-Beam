#include <ESP32Servo.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <vector>

const char* ssid = "";
const char* password = "";

const int potPin = 34;
const int servoPin = 12;
int potValue = 0;
float potLengthCM = 40.0; 
float positionCM = 0;
float velocityCMperS;
double angle = 0.0;
double Ki = 0;
double cumulativeError = 0;
std::vector<float> recordedResponse;
std::vector<unsigned long> timeData;
int sampleNumber = 0;

const float m = 0.2;
const float R = 0.02;
const float g = -9.8;
const double J = 0.000032;
int d = 3;
int L = 40;

Servo servo;

const float A[2][2] = {
    {0, 1},
    {0, 0}
};

const float B[2][1] = {
    {0},
    {-m * g * 0.075 / (J / (R * R) + m)}
};
const float K[1][2] = {
    {38, 16}
};

void setup() {
  Serial.begin(921600);
  servo.attach(servoPin);
  delay(1000);
  positionCM = (potValue / 4095.0) * potLengthCM;;
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.  printf("Connecting to WiFi...");
  }
  Serial.  printf("Connected to WiFi");
}

void loop() {
  potValue = analogRead(potPin);
  float newPositionCM = (potValue / 4095.0) * potLengthCM;
  if((positionCM ==  newPositionCM) || (positionCM < newPositionCM + 0.5) || (positionCM > newPositionCM - 0.5) ){
    sampleNumber++;
    if(sampleNumber > 250){
     sendData(recordedResponse, timeData);
      recordedResponse = std::vector<float>{};
      timeData = std::vector<unsigned long>{};
      sampleNumber = 0;
    }
  }
  // Simulate velocity measurement (in practice, you would measure or estimate this)
  velocityCMperS = (newPositionCM - positionCM)/0.2;  // Assume loop runs every 200ms

  // State vector [position; velocity]
  float x[2] = { newPositionCM, velocityCMperS };
  recordedResponse.push_back(positionCM);
  timeData.push_back(millis());
  float setpoint_cm = 20;
  float error = setpoint_cm - x[0];
  if(error<0.4 && error >-0.4){error = 0; x[1] = 0;}
  cumulativeError+= error;
  
  // Compute control input
  float u = (error)*K[0][0] - K[0][1] * x[1] + cumulativeError*Ki;
  
  // Update servo
  updateMotor(u, setpoint_cm);

  // Update position
  positionCM = newPositionCM;

  delay(100);  
}

void updateMotor(float controlInput, float setpoint_cm) {
  const int u_max = (setpoint_cm - 0)*K[0][0];
  const int u_min = (setpoint_cm - 40)*K[0][0];
  int servoAngle;

  servoAngle = map(controlInput, u_min, u_max, 0, 180);
  servoAngle = constrain(servoAngle, 30, 150);
  
  servo.write(servoAngle);
  delay(100);
}

void sendData(const std::vector<float>& positionData, const std::vector<unsigned long>& timeData) {
  if (WiFi.status() == WL_CONNECTED) { 
    HTTPClient http;
    http.begin("http://192.168.1.16:5000/updatePosition"); 
    
    http.addHeader("Content-Type", "application/json"); 
    StaticJsonDocument<400> jsonDoc;
    JsonArray positions = jsonDoc.createNestedArray("positions");
    JsonArray times = jsonDoc.createNestedArray("times");
    
    for (float position : positionData) {
      positions.add(position);
    }
    
    for (unsigned long timeStamp : timeData) {
      times.add(timeStamp);
    }

    String jsonString;
    serializeJson(jsonDoc, jsonString);

    int httpResponseCode = http.POST(jsonString);
    http.end(); 
  }
}
