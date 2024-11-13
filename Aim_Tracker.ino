#include <math.h>

const int startButton = 4;
const int Analog_Groups[5][2] = {
  {A0, 2},
  {A0, 2},
  {A0, 2},
  {A1, 3},
  {A1, 3}
};

const int TARGET_DURATION = 500;
const int SESSION_TIME = 10000;

int totalAccuracy = 0;
int randomIndex = 0;


void startGame();
bool trackAim();
int continueToTarget(int analogValue, int currDuration);
int randomInt(int lowerBound, int upperBound);


void setup() {
  Serial.begin(9600);

  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);

  pinMode(4, INPUT);

  randomSeed(millis());
}

void loop() {
  if (digitalRead(startButton)) {
    randomIndex = randomInt(0, 5);
    startGame();
    digitalWrite(Analog_Groups[randomIndex][1], LOW);
  }
  
  delay(250);
}


void startGame() {
  int totalDuration = 0;
  int timeBetweenTargets = 0;
  int ledCounter = 0;

  while (totalDuration < SESSION_TIME) {
    long int t1 = millis();
    delay(20);

    bool onTarget = trackAim();

    if (onTarget) {
      ledCounter++;
    }

    timeBetweenTargets += !onTarget * (millis() - t1);
    totalDuration += millis() - t1;
  }


  Serial.print("Reaction Time: ");
  Serial.print(timeBetweenTargets / ledCounter);
  Serial.println(" milliseconds");

  Serial.print("Total Accuracy: ");
  Serial.print(totalAccuracy / ledCounter);
  Serial.println("%");

  Serial.print("Total Targets Hit: ");
  Serial.println(ledCounter);
}


bool trackAim() {
  digitalWrite(Analog_Groups[randomIndex][1], HIGH);
  int analogValue = analogRead(Analog_Groups[randomIndex][0]);
    
  //I don't want to make useless countDuration function calls
  //because it might eat up the dynamic memory
  if (analogValue <= 10) {
    return false;
  }

  long int t1 = millis();
  continueToTarget(analogValue, 0);
  long int t2 = millis();

  digitalWrite(Analog_Groups[randomIndex][1], LOW);
  int totalDuration = t2 - t1;

  totalAccuracy += (int) (100 * (1.0 * TARGET_DURATION / totalDuration));

  int newIndex = randomInt(0, 5);
  while (newIndex == randomIndex) {
    newIndex = randomInt(0, 5);
  }

  randomIndex = newIndex;

  delay(500);
  return true;
}


int continueToTarget(int analogValue, int currDuration) {
  if (currDuration >= TARGET_DURATION) {
    return 0;
  }

  long int t1 = millis();
  long int t2 = t1;

  while (analogValue > 10 && (t2 - t1 + currDuration) < TARGET_DURATION) {
    analogValue = analogRead(Analog_Groups[randomIndex][0]);
    t2 = millis();
  }

  delay(2);
  return t2 - t1 + continueToTarget(analogRead(Analog_Groups[randomIndex][0]), t2 - t1 + currDuration);
}


int randomInt(int lowerBound, int upperBound) {
  return (int) floor(random(lowerBound, upperBound));
}