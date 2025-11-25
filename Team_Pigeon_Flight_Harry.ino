/*
  Scene behaviors:
    Scene 1 (servo 8):   360° motor → spins only while you hold the tap
    Scene 2 (servo 9):   360° motor → spins only while you hold the tap
    Scene 3 (servo 10):  180° servo → moves back and forth while you hold tap
    Scene 4 (servo 11):  180° servo → moves from 0 → 180 once when tapped
    Scene 5 (servo 12):  180° servo → moves from 0 → 180 once when tapped
    Scene 6 (servo 13):  360° motor → spins once for a set amount of time
*/

#include <Servo.h>

// ---- Pin Assignments ----
const int servoPins[6] = {8, 9, 10, 11, 12, 13};
const int tapPins[6]   = {2, 3, 4, 5, 6, 7};

Servo servos[6];

// ---- Tap Reading + Debounce ----
int tapState[6];
int lastRawTap[6];
unsigned long lastDebounceTime[6];
const unsigned long debounceDelay = 12;

// ---- Active Scene Tracking ----
int currentHoldScene = -1; // only hold-type scenes (0,1,2,5)

// ---- Scene 3 Ping-Pong ----
int pp_position = 0;
int pp_step = 2;
unsigned long pp_lastMove = 0;
const unsigned long pp_interval = 30;
const int pp_min = 0;
const int pp_max = 40;

// ---- Scene 6 Timed Spin ----
int stopPulse = 1500;
int spinPulse = 1600;
bool isScene6Spinning = false;
unsigned long scene6StartTime = 0;
unsigned long scene6SpinDuration = 900;

void setup() {
  Serial.begin(9600);
  Serial.println("Controller starting up...");

  for (int i = 0; i < 6; i++) {
    pinMode(tapPins[i], INPUT);
    lastRawTap[i] = digitalRead(tapPins[i]);
    tapState[i] = lastRawTap[i];
    lastDebounceTime[i] = millis();
    servos[i].attach(servoPins[i]);
  }

  // Initialize servos
  servos[0].writeMicroseconds(stopPulse);
  servos[1].writeMicroseconds(stopPulse);
  servos[2].write(pp_min);
  servos[3].write(0);
  servos[4].write(0);
  servos[5].writeMicroseconds(stopPulse);

  Serial.println("Ready. Touch taps 2–7 to activate scenes.");
}

void loop() {
  unsigned long now = millis();

  // ---- Read taps ----
  for (int i = 0; i < 6; i++) {
    int rawTapValue = digitalRead(tapPins[i]);
    if (rawTapValue != lastRawTap[i]) {
      lastDebounceTime[i] = now;
      lastRawTap[i] = rawTapValue;
    }
    if ((now - lastDebounceTime[i]) > debounceDelay) {
      if (rawTapValue != tapState[i]) {
        int previous = tapState[i];
        tapState[i] = rawTapValue;
        if (tapState[i] == HIGH && previous == LOW) onTapPressed(i);
        else if (tapState[i] == LOW && previous == HIGH) onTapReleased(i);
      }
    }
  }

  // ---- Scene 3 ping-pong ----
  if (currentHoldScene == 2) scene3_pingpong(now);

  // ---- Scene 6 timed spin ----
  if (isScene6Spinning && (now - scene6StartTime >= scene6SpinDuration)) {
    servos[5].writeMicroseconds(stopPulse);
    isScene6Spinning = false;
    Serial.println("Scene 6: spin finished (auto stop).");
    if (currentHoldScene == 5) currentHoldScene = -1;
  }
}

// ----------------- On Tap Pressed -----------------
void onTapPressed(int sceneIndex) {
  Serial.print("Tap ");
  Serial.print(sceneIndex + 1);
  Serial.println(" PRESSED");

  if (currentHoldScene != -1 && currentHoldScene != sceneIndex) {
    stopHoldScene(currentHoldScene);
    currentHoldScene = -1;
  }

  switch (sceneIndex) {
    case 0: // Scene 1
      servos[0].writeMicroseconds(2000);
      currentHoldScene = 0;
      Serial.println("Scene 1: spinning (hold)");
      break;

    case 1: // Scene 2
      servos[1].writeMicroseconds(2000);
      currentHoldScene = 1;
      Serial.println("Scene 2: spinning (hold)");
      break;

    case 2: // Scene 3
      pp_position = pp_min;
      pp_step = 2;
      pp_lastMove = millis();
      currentHoldScene = 2;
      Serial.println("Scene 3: ping-pong motion started");
      break;

    case 3: // Scene 4 one-time
      servos[3].write(180);
      Serial.println("Scene 4: moved to 180° (one-time)");
      break;

    case 4: // Scene 5 one-time
      servos[4].write(180);
      Serial.println("Scene 5: moved to 180° (one-time)");
      break;

    case 5: // Scene 6 timed spin
      servos[5].writeMicroseconds(spinPulse);
      scene6StartTime = millis();
      isScene6Spinning = true;
      currentHoldScene = 5;
      Serial.println("Scene 6: spinning for preset duration...");
      break;
  }
}

// ----------------- On Tap Released -----------------
void onTapReleased(int sceneIndex) {
  Serial.print("Tap ");
  Serial.print(sceneIndex + 1);
  Serial.println(" RELEASED");

  // Reset Scene 4 and 5 when their tap is released
  if (sceneIndex == 3) servos[3].write(0); // Scene 4
  if (sceneIndex == 4) servos[4].write(0); // Scene 5

  // Stop hold-type scenes
  if ((sceneIndex == 0 || sceneIndex == 1 || sceneIndex == 2 || sceneIndex == 5) &&
      currentHoldScene == sceneIndex) {
    stopHoldScene(sceneIndex);
    currentHoldScene = -1;
  }
}

// ----------------- Stop Hold Scene -----------------
void stopHoldScene(int sceneIndex) {
  Serial.print("Stopping Scene ");
  Serial.println(sceneIndex + 1);
  switch (sceneIndex) {
    case 0: servos[0].writeMicroseconds(stopPulse); break;
    case 1: servos[1].writeMicroseconds(stopPulse); break;
    case 2: servos[2].write(pp_min); break;
    case 5: servos[5].writeMicroseconds(stopPulse); isScene6Spinning=false; break;
  }
}

// ----------------- Scene 3 Ping-Pong -----------------
void scene3_pingpong(unsigned long now) {
  if (now - pp_lastMove >= pp_interval) {
    pp_position += pp_step;
    
    if (pp_position >= pp_max) { 
      pp_position = pp_max; 
      pp_step = -abs(pp_step); 
    } 
    else if (pp_position <= pp_min) { 
      pp_position = pp_min; 
      pp_step = abs(pp_step); 
    }
    
    servos[2].write(pp_position);
    pp_lastMove = now;
  }
}
