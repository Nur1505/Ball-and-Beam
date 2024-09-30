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
float positionCM;
double previousAngle = 0;
double newAngle = 0;

double cumulativeError = 0;
double previousError = 0;
std::vector<float> recordedResponse;
std::vector<unsigned long> timeData; // Vector to store time stamps
int sampleNumber = 0;

double Kp = 20; 
double Ki = 0.3; 
double Kd = 40; 


Servo servo;

void setup() {
  Serial.begin(921600);
  servo.attach(servoPin);
  delay(1000);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void loop() {


  potValue = analogRead(potPin);
  float newPositionCM = (potValue / 4095.0) * potLengthCM;
  //Map the potValue to a position in cm
  sampleNumber++;
    
  if(sampleNumber > 250){
	sendData(recordedResponse, timeData);
	recordedResponse.clear();
	timeData.clear();
	sampleNumber = 0;
  }
 
  positionCM = newPositionCM;
  
  recordedResponse.push_back(positionCM);
  timeData.push_back(millis());

  newAngle = pid(positionCM);
  updateMotor(newAngle);
  previousAngle = newAngle;
  delay(100);
}

void updateMotor(double new_tilt_deg) {
  new_tilt_deg = constrain(new_tilt_deg, 0, 180);
  servo.write(new_tilt_deg); 
}

double pid(double distance_cm) {
 
  double setpoint_cm = 20;
  
  double error = setpoint_cm - distance_cm;

  if(error<0.4 && error>-0.4){
    error = 0;
  }
  
  double p_value = error * Kp;

  double i_value = cumulativeError * Ki;

  double d_value = (error - previousError) * Kd;

  double pid_value = p_value + i_value + d_value;
  
  Serial.print("pid_value:");
  Serial.println(pid_value);

  const int upper_limit = (setpoint_cm - 0)*Kp;
  const int lower_limit = (setpoint_cm - 40)*Kp;
  cumulativeError += error;
  previousError = error; 
  
  double new_servo_angle = map(pid_value, lower_limit, upper_limit, 0, 180);

  return new_servo_angle;
}

void sendData(const std::vector<float>& positionData, const std::vector<unsigned long>& timeData) {
  if (WiFi.status() == WL_CONNECTED) { // Check WiFi connection status
    HTTPClient http;
    http.begin("http://192.168.1.16:5000/updatePosition"); // Your server URL
    
    http.addHeader("Content-Type", "application/json"); // Specify content-type header
    
    // Create JSON document
    StaticJsonDocument<400> jsonDoc;
    JsonArray positions = jsonDoc.createNestedArray("positions");
    JsonArray times = jsonDoc.createNestedArray("times");
    
    // Fill the JSON arrays with the position data and corresponding time stamps
    for (float position : positionData) {
      positions.add(position);
    }
    
    for (unsigned long timeStamp : timeData) {
      times.add(timeStamp);
    }

    // Convert JSON document to string
    String jsonString;
    serializeJson(jsonDoc, jsonString);

    // Send the request
    int httpResponseCode = http.POST(jsonString);

    if (httpResponseCode > 0) { // Check for the returning code
      String response = http.getString();
      Serial.println(httpResponseCode); // Print return code
      Serial.println(response);         // Print request answer
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }

    http.end(); // Free the resources
  }
}
