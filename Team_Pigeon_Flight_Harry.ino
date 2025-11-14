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
// servoPins[i] = pin for servo of scene i
// tapPins[i]   = pin for copper tap of scene i
const int servoPins[6] = {8, 9, 10, 11, 12, 13};
const int tapPins[6]   = {2, 3, 4, 5, 6, 7};

Servo servos[6];  // all 6 servo motors

// ---- Tap Reading + Debounce ----
// tapState[i]     = stable HIGH/LOW of tap i
// lastRawTap[i]   = most recent raw reading (before debounce)
// lastDebounceTime[i] = timestamp when tap reading last changed
int tapState[6];
int lastRawTap[6];
unsigned long lastDebounceTime[6];
const unsigned long debounceDelay = 12; // time to confirm stable tap

// ---- Active Scene Tracking ----
// Only these scenes run continuously: 1, 2, 3, and 6
// (0,1,2,5 in array index form)
int currentHoldScene = -1; // -1 = nothing currently running

// ---- Scene 3 (Ping-Pong Servo) ----
int pp_position = 0;          // current angle
int pp_step = 1;              // +1 or -1 (moving forward/back)
unsigned long pp_lastMove = 0;
const unsigned long pp_interval = 10; // time between small angle movements

// ---- Scene 6 (Timed Spin) ----
int stopPulse = 1500;         // pulse width that stops a 360° servo
int spinPulse = 1600;         // slow spin (closer to 1500 = slower)
bool isScene6Spinning = false;
unsigned long scene6StartTime = 0;
unsigned long scene6SpinDuration = 900; // how long to spin for full turn

void setup() {
  Serial.begin(9600);
  Serial.println("Controller starting up...");

  // Setup all tap pins and servo pins
  for (int i = 0; i < 6; i++) {
    pinMode(tapPins[i], INPUT); // copper tap uses external pulldown

    // initialize debounce readings
    lastRawTap[i] = digitalRead(tapPins[i]);
    tapState[i] = lastRawTap[i];
    lastDebounceTime[i] = millis();

    // attach servos
    servos[i].attach(servoPins[i]);
  }

  // Start servos at safe/neutral positions
  servos[0].writeMicroseconds(stopPulse); // stop continuous servo
  servos[1].writeMicroseconds(stopPulse);
  servos[2].write(0);   // 180° servo start at zero
  servos[3].write(0);
  servos[4].write(0);
  servos[5].writeMicroseconds(stopPulse);

  Serial.println("Ready. Touch taps 2–7 to activate scenes.");
}

void loop() {
  unsigned long now = millis();

  // ---- Read all taps and detect presses/releases ----
  for (int i = 0; i < 6; i++) {
    int rawTapValue = digitalRead(tapPins[i]);

    // If reading changed, start debounce timer
    if (rawTapValue != lastRawTap[i]) {
      lastDebounceTime[i] = now;
      lastRawTap[i] = rawTapValue;
    }

    // Reading becomes stable after debounceDelay ms
    if ((now - lastDebounceTime[i]) > debounceDelay) {
      if (rawTapValue != tapState[i]) {
        int previous = tapState[i];
        tapState[i] = rawTapValue;

        if (tapState[i] == HIGH && previous == LOW) {
          onTapPressed(i);
        } else if (tapState[i] == LOW && previous == HIGH) {
          onTapReleased(i);
        }
      }
    }
  }

  // ---- Scene 3 ongoing ping-pong motion ----
  if (currentHoldScene == 2) scene3_pingpong(now);

  // ---- Scene 6 timed spin ongoing ----
  if (isScene6Spinning) {
    if (now - scene6StartTime >= scene6SpinDuration) {
      servos[5].writeMicroseconds(stopPulse);
      isScene6Spinning = false;
      Serial.println("Scene 6: spin finished (auto stop).");

      if (currentHoldScene == 5) currentHoldScene = -1;
    }
  }
}

// ----------------- On Tap Pressed -----------------
void onTapPressed(int sceneIndex) {
  Serial.print("Tap ");
  Serial.print(sceneIndex + 1);
  Serial.println(" PRESSED");

  // Stop previously running hold-type scene
  if (currentHoldScene != -1 && currentHoldScene != sceneIndex) {
    stopHoldScene(currentHoldScene);
    currentHoldScene = -1;
  }

  switch (sceneIndex) {

    case 0: // Scene 1
      servos[0].writeMicroseconds(2000);
      Serial.println("Scene 1: spinning (hold to keep spinning)");
      currentHoldScene = 0;
      break;

    case 1: // Scene 2
      servos[1].writeMicroseconds(2000);
      Serial.println("Scene 2: spinning (hold to keep spinning)");
      currentHoldScene = 1;
      break;

    case 2: // Scene 3
      pp_position = 0;
      pp_step = 1;
      pp_lastMove = millis();
      Serial.println("Scene 3: ping-pong motion started");
      currentHoldScene = 2;
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

  // Only stop if the scene is the currently running hold scene
  if ((sceneIndex == 0 || sceneIndex == 1 || sceneIndex == 2) &&
      currentHoldScene == sceneIndex) {

    stopHoldScene(sceneIndex);
    currentHoldScene = -1;
  }
}

// ----------------- Stop a Hold Scene -----------------
void stopHoldScene(int sceneIndex) {
  Serial.print("Stopping Scene ");
  Serial.println(sceneIndex + 1);

  switch (sceneIndex) {
    case 0:
      servos[0].writeMicroseconds(stopPulse);
      break;
    case 1:
      servos[1].writeMicroseconds(stopPulse);
      break;
    case 2:
      servos[2].write(90); // middle position (neutral)
      break;
    case 5:
      servos[5].writeMicroseconds(stopPulse);
      isScene6Spinning = false;
      break;
  }
}

// ----------------- Scene 3 Ping-Pong Motion -----------------
void scene3_pingpong(unsigned long now) {
  if (now - pp_lastMove >= pp_interval) {
    pp_position += pp_step;

    if (pp_position >= 180) {
      pp_position = 180;
      pp_step = -1;
    } else if (pp_position <= 0) {
      pp_position = 0;
      pp_step = 1;
    }

    servos[2].write(pp_position);
    pp_lastMove = now;
  }
}
