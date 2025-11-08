// GitHub Setup Practice - This is a message proving update committed!
// Lai_Sissi_Sensor_Practice
// Lana was here too
// Josiah was here as well...

int sensorPin = A0; // photoresistor connected to A0
int ledPin = 13;    // LED on pin 13
int sensorValue = 0; 

void setup()
  pinMode(ledPin, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  sensorValue = analogRead(sensorPin); // reads value 0-1023
  Serial.println(sensorValue);

  // Decide threshold for LED
  if(sensorValue < 100){ // dark environment
    digitalWrite(ledPin, HIGH); // turn on LED
  } else { // bright environment
    digitalWrite(ledPin, LOW); // turn off LED
  }

  delay(100); // small delay
}
