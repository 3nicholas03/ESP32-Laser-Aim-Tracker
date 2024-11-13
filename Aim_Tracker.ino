/*
* Math.h is only used for the floor function
* LiquidCrystal_I2C.h is the library for the LCD screen
*/
#include <math.h>
#include <LiquidCrystal_I2C.h>


// The lcd object that's used to manipulate the screen
LiquidCrystal_I2C lcd(0x27, 16, 2);


/*
* The button to start the entire sequence is connected to pin 36
* The analog groups array is a way to keep track of 
* photoresistors and their corresponding leds.
* Each row is a photoresistor and its corresponding led.
*/
const int startButton = 34;
const int ledPin1 = 32;
const int ledPin2 = 33;
const int photoResistor1 = 25;
const int photoResistor2 = 26;
const int analogGroups[2][2] = {
  {photoResistor1, ledPin1},
  {photoResistor2, ledPin2},
};


/*
* These are constants for the amount of time the player needs to hit the photoresistor
* and the total amount of time per session
*/
const int TARGET_DURATION = 500;
const int SESSION_TIME = 10000;
const int PHOTO_RESISTOR_THRESHOLD = 200;


/*
* These are global variables that need to be accessed from different portions of the program
*/
int totalAccuracy = 0;
int randomIndex = 0;


/*
* These are here because I like to define the functions near the top of the program
* then implement them below the main loop
*/
void startGame();
bool trackAim();
void continueToTarget();
int randomInt(int lowerBound, int upperBound);
void displayResults(int reactionTime, int accuracy, int targetTotal);
int countDigits(int num);
void clearLCD(int charCount, int row);
bool killSwitchDelayCheck(int delay);
void countDown(int length);


/*
* The setup function that initializes the digital pins, creates a seed for the random function, and initializes the lcd screen
*/
void setup() {
  Serial.begin(115200);

  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);

  pinMode(startButton, INPUT);

  randomSeed(millis());

  lcd.init();
  lcd.backlight();
}


/*
* The main loop that checks if the player has pressed the button
*/
void loop() {
  lcd.setCursor(2, 0);
  lcd.print("Click Button");
  
  lcd.setCursor(4, 1);
  lcd.print("to Begin");

  if (digitalRead(startButton)) {
    lcd.clear();
    randomIndex = randomInt(0, 2);

    countDown(3);
    startGame();
  }
  
  delay(50);
}


/*
* This function's purpose is to control the main game loop, track the stats,
* then print the results at the end.
*/
void startGame() {
  int totalDuration = 0;  // This is the amount of time elapsed since starting the game
  int timeBetweenTargets = 0;  // This is a measurment that's used for calculating the reaction speed
  int targetCounter = 0;  // This is used for the reaction speed and the accuracy, and it counts how many targets were hit
  totalAccuracy = 0; //Reset the accuracy every time the game is started

  Serial.println(analogGroups[randomIndex][0]);
  while (totalDuration < SESSION_TIME) {
    long int t1 = millis();  // I purposefully waited 10ms after taking the first time measurement so that the second time measurement isn't super close
    delay(10);
    
    bool onTarget = trackAim(); // onTarget specifies if the player hit the photoresistor group when trackAim was called

    // if the player hits the target, then increment the targetCounter and give them 500ms to react to the next target
    if (onTarget) {
      targetCounter++;
      delay(500);
    }

    long int t2 = millis();
    timeBetweenTargets += !onTarget * (t2 - t1); // timeBetweenTargets only gets incremented when the player didn't hit the target
    totalDuration += t2 - t1;
  }

  lcd.clear();
  digitalWrite(analogGroups[randomIndex][1], LOW); // This is here to make sure the last led turns off after the game is done

  // Serial Monitor Prints
  Serial.print("Reaction Time: ");
  Serial.print(timeBetweenTargets / ((targetCounter == 0) + targetCounter));  // This averages the time between each target hit (The extra math is just a form of exception handling to ensure we don't divide by 0)
  Serial.println(" milliseconds");

  Serial.print("Accuracy: ");
  Serial.print(totalAccuracy / ((targetCounter == 0) + targetCounter));  // This averages the accuracy of each target hit
  Serial.println("%");

  Serial.print("Targets Hit: ");
  Serial.println(targetCounter);

  // LCD Prints (1st argument is reaction time, 2nd argument is accuracy, 3rd argument is total hits)
  displayResults(timeBetweenTargets / ((targetCounter == 0) + targetCounter), totalAccuracy / ((targetCounter == 0) + targetCounter), targetCounter);
  lcd.clear();
}


/*
* This function's purpose is to react whenever the player hits the photoresistor group
* and track any corresponding values that it needs. It also returns a boolean value
* for if the player was on target.
*/
bool trackAim() {
  digitalWrite(analogGroups[randomIndex][1], HIGH);  // Turn on the random led
  int analogValue = analogRead(analogGroups[randomIndex][0]);  // Reads the analogValue of the random photoresistor group
    
  //I only want to continue if the user aims at the photoresistor
  if (analogValue <= PHOTO_RESISTOR_THRESHOLD) {
    return false;
  }

  long int t1 = millis();
  continueToTarget();  // The function that continues running until the target duration is reached

  totalAccuracy += (int) (100 * (1.0 * TARGET_DURATION / (millis() - t1)));  // Accuracy is calculated based on the ratio of TARGET_DURATION:(The time spent hitting the target)

  digitalWrite(analogGroups[randomIndex][1], LOW); // Turn off the random led

  randomIndex = randomInt(0, 2);  // The new index for the next random group
  return true;
}


/*
* A function that keeps running until the user keeps the laser on the photoresistor for the target
* amount of time. 
*/
void continueToTarget() {
  int currDuration = 0;

  while (currDuration < TARGET_DURATION) {
    long int t1 = millis();
    long int t2 = t1;

    int analogValue = analogRead(analogGroups[randomIndex][0]);
    while (analogValue > PHOTO_RESISTOR_THRESHOLD && (t2 - t1 + currDuration) < TARGET_DURATION) {
      analogValue = analogRead(analogGroups[randomIndex][0]);
      delay(5);
      t2 = millis();
      Serial.println(analogValue);
    }

    currDuration += t2 - t1;
    delay(5);
  }
}


/*
* A function that selects a random integer where the lowerBound is inclusive
* and the upperBound is exclusive
*/
int randomInt(int lowerBound, int upperBound) {
  return (int) floor(random(lowerBound, upperBound)); // Round down the random float that was returned from the random function
}


/*
* A function made to display results until the player wants to play again
*/
void displayResults(int reactionTime, int accuracy, int targetTotal) {
  while (true) {
    const int MAX_COLUMNS = 16;
    int charLength = countDigits(reactionTime);

    // LCD Screen Prints
    lcd.setCursor(1, 0);
    lcd.print("Reaction Time:");

    lcd.setCursor((MAX_COLUMNS - charLength - 3) / 2, 1);  //Math used to center the text along the row
    lcd.print(reactionTime);
    lcd.print(" ms");

    if (killSwitchDelayCheck(3000)) {return;}  //Check to see if the button was pressed while displaying the results
    clearLCD(14, 0);
    clearLCD(charLength + 3, 1);
    

    charLength = countDigits(accuracy);
    lcd.setCursor(3, 0);
    lcd.print("Accuracy:");

    lcd.setCursor((MAX_COLUMNS - charLength - 1) / 2, 1);
    lcd.print(accuracy);
    lcd.print("%");

    if (killSwitchDelayCheck(3000)) {return;}
    clearLCD(9, 0);
    clearLCD(charLength + 1, 1);


    charLength = countDigits(targetTotal);
    lcd.setCursor(2, 0);
    lcd.print("Targets Hit:");

    lcd.setCursor((MAX_COLUMNS - charLength) / 2, 1);
    lcd.print(targetTotal);

    if (killSwitchDelayCheck(3000)) {return;}
    clearLCD(12, 0);
    clearLCD(charLength, 1);


    lcd.setCursor(2, 0);
    lcd.print("Press Button");
    lcd.setCursor(1, 1);
    lcd.print("to Play Again!");

    if (killSwitchDelayCheck(5000)) {return;}
    clearLCD(12, 0);
    clearLCD(14, 1);
  }
}


/*
* A function that's useful for counting the number of digits within an integer
*/
int countDigits(int num) {
  int count = 0;
  while (num != 0 || count == 0) {
    num /= 10;

    count++;
  }
  return count;
}


/*
* A function that clears the LCD screen without flashing
*/
void clearLCD(int charCount, int row) {
  const int MAX_COLUMNS = 16;

  lcd.setCursor((MAX_COLUMNS - charCount) / 2, row);
  for (int i = 0; i < charCount; i++) {
    lcd.print(" ");
  } 
}


/*
* A function that delays for a specified amount of time while checking if the
* start button was pressed
*/
bool killSwitchDelayCheck(int timeDelay) {
  long int startTime = millis();
  long int currentTime = startTime;
  while ((currentTime - startTime) < timeDelay) {
    if (digitalRead(startButton)) {
      return true;
    }

    delay(5);
    currentTime = millis();
  }
  return false;
}


/*
* A function that counts down on the LCD screen before starting the game sequence
*/
void countDown(int length) {
  for (int i = length; i > 0; i--) {
    lcd.setCursor(7, 0);
    lcd.print(i);
    delay(1000);
  }

  lcd.setCursor(5, 0);
  lcd.print("Start!");
}
