/**********************************************************************************************************
    Name    : Plantomatic
    Author  : Lars Almén
    Created : 2017-04-02
    Last Modified: 2017-04-02
    Version : 1.0.0
    Notes   : The software monitors soil moisture levels and controls a pump connected to a relay board
              according to setpoints for wet and dry soil, and maximum pump time.
              It has several other parameters available from the menu to fine-tune control, such as
              interval between measurements, how many samples is taken (and delay between those) each "run"
              and maximum pump runtime when dispensing water.
              To minimize galvanic corrosion, and maximize sensor lifetime,
              the sensor is powered only for the few milliseconds it takes to get a sample.
              It borrows heavily from Paul Siewert´s menu-system, with some tweaks,
              and Sverd Industries pumpless water system.
    License : This software is available under MIT License.
              Copyright 2017 Lars Almén
              Permission is hereby granted, free of charge, to any person obtaining a copy
              of this software and associated documentation files (the "Software"),
              to deal in the Software without restriction, including without limitation the rights to use,
              copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
              and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
              The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
              THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
              INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
              IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
              WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ***********************************************************************************************************/
 
#include <LiquidCrystal.h>

// Pin configs
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
int sensorPin = 1;  // Set pin for the soil sensor
int sensorPowerPin = 2; // Set pin to power the sensor
int pumpPin = 3; // Set pin to control pump-relay

// Global variables
int soilValue;    // Variable to store one sensor reading
double avgSoil; // Variable to store average of 10 sensor readings
// Variable to keep track of loops
unsigned long previousLoopMillis = 0;

// Soil and timings setup
int wetValue = 500; // The default sensor value where the soil is sufficiently wet
int dryValue = 600; // The default value where the soil is dry and needs watering

int samples = 10; // Default no. of samples to average from
int maxSamples = 50; // Upper limit of sample settings

int sampleDelay = 5; // Default delay between samples (seconds)
int maxSampleDelay = 60; // Upper limit of setting delay between samples (seconds)

int runtimeInterval = 2; // Default interval between starts (minutes)
int maxRuntimeInterval = 360; // Upper setpoint interval between starts(minutes)

//int pumpRunCycleTime = 10; // Cycling the pump may allow water to distribute more evenly in the soil
//int pumpDelayCycleTime = 5; // Cycling the pump may allow water to distribute more evenly in the soil
int pumpRunTime = 20; // Maximum runtime, useful for ex. alarm if the pump does not achieve wet soil in reasonable time (seconds)
int maxPumpRunTime = 120; // Upper setpoint for pumpRunTime

// Array of strings to display as menu choices
String menuItems[] = {"Start", "Pump on thresh.", "Pump off thresh.", "Sample delay", "No. samples", "Start interval", "Set pump time"};

// Navigation button variables
int readKey;

// Menu control variables
int menuPage = 0;
int maxMenuPages = round((sizeof(menuItems) / sizeof(String)) - 2);
int cursorPosition = 0;

// Creates 3 custom characters for the menu display
byte downArrow[8] = {
  0b00100, //   *
  0b00100, //   *
  0b00100, //   *
  0b00100, //   *
  0b00100, //   *
  0b10101, // * * *
  0b01110, //  ***
  0b00100  //   *
};

byte upArrow[8] = {
  0b00100, //   *
  0b01110, //  ***
  0b10101, // * * *
  0b00100, //   *
  0b00100, //   *
  0b00100, //   *
  0b00100, //   *
  0b00100  //   *
};

byte menuCursor[8] = {
  B01000, //  *
  B00100, //   *
  B00010, //    *
  B00001, //     *
  B00010, //    *
  B00100, //   *
  B01000, //  *
  B00000  //
};

void setup() {
  // Sets up pinmodes
  pinMode(sensorPin, INPUT);
  pinMode(sensorPowerPin, OUTPUT);
  pinMode(pumpPin, OUTPUT);

  // Initializes serial communication, for debugging purposes
  //Serial.begin(9600);

  // Initializes and clears the LCD screen
  lcd.begin(16, 2);

  // Creates the byte for the 3 custom characters
  lcd.createChar(0, menuCursor);
  lcd.createChar(1, upArrow);
  lcd.createChar(2, downArrow);
}

void loop() {
  mainMenuDraw();
  drawCursor();
  operateMainMenu();
}

// This function will generate the 2 menu items that can fit on the screen. They will change as you scroll through your menu. Up and down arrows will indicate your current menu position.
void mainMenuDraw() {
  // Serial debugging info
  //Serial.println(menuPage);
  //Serial.println(menuItems[menuPage]);
  //Serial.println(maxMenuPages);
  //Serial.println(cursorPosition);
  
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print(menuItems[menuPage]);
  lcd.setCursor(1, 1);
  lcd.print(menuItems[menuPage + 1]);
  if (menuPage == 0) {
    lcd.setCursor(15, 1);
    lcd.write(byte(2));
  } else if (menuPage > 0 and menuPage < maxMenuPages) {
    lcd.setCursor(15, 1);
    lcd.write(byte(2));
    lcd.setCursor(15, 0);
    lcd.write(byte(1));
  } else if (menuPage == maxMenuPages) {
    lcd.setCursor(15, 0);
    lcd.write(byte(1));
  }
}

// When called, this function will erase the current cursor and redraw it based on the cursorPosition and menuPage variables.
void drawCursor() {
  for (int x = 0; x < 2; x++) {     // Erases current cursor
    lcd.setCursor(0, x);
    lcd.print(" ");
  }

  // The menu is set up to be progressive (menuPage 0 = Item 1 & Item 2, menuPage 1 = Item 2 & Item 3, menuPage 2 = Item 3 & Item 4), so
  // in order to determine where the cursor should be you need to see if you are at an odd or even menu page and an odd or even cursor position.
  if (menuPage % 2 == 0) {
    if (cursorPosition % 2 == 0) {  // If the menu page is even and the cursor position is even that means the cursor should be on line 1
      lcd.setCursor(0, 0);
      lcd.write(byte(0));
    }
    if (cursorPosition % 2 != 0) {  // If the menu page is even and the cursor position is odd that means the cursor should be on line 2
      lcd.setCursor(0, 1);
      lcd.write(byte(0));
    }
  }
  if (menuPage % 2 != 0) {
    if (cursorPosition % 2 == 0) {  // If the menu page is odd and the cursor position is even that means the cursor should be on line 2
      lcd.setCursor(0, 1);
      lcd.write(byte(0));
    }
    if (cursorPosition % 2 != 0) {  // If the menu page is odd and the cursor position is odd that means the cursor should be on line 1
      lcd.setCursor(0, 0);
      lcd.write(byte(0));
    }
  }
}


void operateMainMenu()
{
  int activeButton = 0;
  while (activeButton == 0) {
    int button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 0: // When button returns as 0 there is no action taken
        break;
      case 1:  // This case will execute if the "forward" button is pressed
        button = 0;
        switch (cursorPosition) { // The case that is selected here is dependent on which menu page you are on and where the cursor is.
          case 0:
            Start();
            break;
          case 1:
            SetDryValue();
            break;
          case 2:
            SetWetValue();
            break;
          case 3:
            SetSampleDelay();
            break;
          case 4:
            SetSamples();
            break;
          case 5:
            SetRuntimeInterval();
            break;
          case 6:
            SetMaxPumpRunTime();
            break;
        }
        activeButton = 1;
        mainMenuDraw();
        drawCursor();
        break;
      case 2:
        button = 0;
        if (menuPage == 0) {
          cursorPosition = cursorPosition - 1;
          cursorPosition = constrain(cursorPosition, 0, ((sizeof(menuItems) / sizeof(String)) - 1));
        }
        if (menuPage % 2 == 0 and cursorPosition % 2 == 0) {
          menuPage = menuPage - 1;
          menuPage = constrain(menuPage, 0, maxMenuPages);
        }

        if (menuPage % 2 != 0 and cursorPosition % 2 != 0) {
          menuPage = menuPage - 1;
          menuPage = constrain(menuPage, 0, maxMenuPages);
        }

        cursorPosition = cursorPosition - 1;
        cursorPosition = constrain(cursorPosition, 0, ((sizeof(menuItems) / sizeof(String)) - 1));

        mainMenuDraw();
        drawCursor();
        activeButton = 1;
        break;
      case 3:
        button = 0;
        if (menuPage % 2 == 0 and cursorPosition % 2 != 0) {
          menuPage = menuPage + 1;
          menuPage = constrain(menuPage, 0, maxMenuPages);
        }

        if (menuPage % 2 != 0 and cursorPosition % 2 == 0) {
          menuPage = menuPage + 1;
          menuPage = constrain(menuPage, 0, maxMenuPages);
        }

        cursorPosition = cursorPosition + 1;
        cursorPosition = constrain(cursorPosition, 0, ((sizeof(menuItems) / sizeof(String)) - 1));
        mainMenuDraw();
        drawCursor();
        activeButton = 1;
        break;
    }
  }
}

// This function is called whenever a button press is evaluated. The LCD shield works by observing a voltage drop across the buttons all hooked up to A0.
int evaluateButton(int x)
{
  int result = 0;
  if (x < 50) {
    result = 1; // right
  } else if (x < 195) {
    result = 2; // up
  } else if (x < 380) {
    result = 3; // down
  } else if (x < 790) {
    result = 4; // left
  }
  return result;
}

void SetDryValue()
{
  int activeButton = 0;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pump on thresh: ");

  while (activeButton == 0) {

    lcd.setCursor(0, 1);
    lcd.print(dryValue);
    lcd.setCursor(5, 1);
    lcd.write(byte(1));
    lcd.setCursor(6, 1);
    lcd.print("= drier");

    int button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 2: lcd.setCursor(0, 1);
        lcd.print("    ");
        dryValue = min(dryValue + 1, 1000);
        break;
      case 3: lcd.setCursor(0, 1);
        lcd.print("    ");
        dryValue = max(dryValue - 1, 1);
        break;
      case 4:  // This case will execute if the "back" button is pressed
        button = 0;
        activeButton = 1;
        break;
    }
  }
}

void SetWetValue()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pump off thresh: ");

  int activeButton = 0;

  while (activeButton == 0) {

    lcd.setCursor(0, 1);
    lcd.print(wetValue);
    lcd.setCursor(5, 1);
    lcd.write(byte(1));
    lcd.setCursor(6, 1);
    lcd.print("= drier");

    int button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 2:
        lcd.setCursor(0, 1);
        lcd.print("    ");
        wetValue = min(wetValue + 1, 1000);
        break;
      case 3:
        lcd.setCursor(0, 1);
        lcd.print("    ");
        wetValue = max(wetValue - 1, 1);
        break;
      case 4:  // This case will execute if the "back" button is pressed
        button = 0;
        activeButton = 1;
        break;
    }
  }
}

void SetSampleDelay()
{
  int activeButton = 0;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Delay bt samples");

  while (activeButton == 0) {

    lcd.setCursor(0, 1);
    lcd.print(sampleDelay);
    lcd.setCursor(5, 1);
    lcd.print(" seconds.");

    int button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 2:
        lcd.setCursor(0, 1);
        lcd.print("  ");
        sampleDelay = min(sampleDelay + 1, maxSampleDelay);
        break;
      case 3:
        lcd.setCursor(0, 1);
        lcd.print("  ");
        sampleDelay = max(sampleDelay - 1, 1);
        break;
      case 4:  // This case will execute if the "back" button is pressed
        button = 0;
        activeButton = 1;
        break;
    }
  }
}

void SetSamples()
{
  int activeButton = 0;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Samples taken");

  while (activeButton == 0) {

    lcd.setCursor(0, 1);
    lcd.print(samples);

    int button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 2:
        lcd.setCursor(0, 1);
        lcd.print("  ");
        samples = min(samples + 1, maxSamples);
        break;
      case 3:
        lcd.setCursor(0, 1);
        lcd.print("  ");
        samples = max(samples - 1, 1);
        break;
      case 4:  // This case will execute if the "back" button is pressed
        button = 0;
        activeButton = 1;
        break;
    }
  }
}

void SetRuntimeInterval()
{
  int activeButton = 0;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pause bt checks");

  while (activeButton == 0) {

    lcd.setCursor(0, 1);
    lcd.print(runtimeInterval);
    lcd.setCursor(5, 1);
    lcd.print("minutes.");

    int button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 2:
        lcd.setCursor(0, 1);
        lcd.print("   ");
        runtimeInterval = min(runtimeInterval + 1, maxRuntimeInterval);
        break;
      case 3:
        lcd.setCursor(0, 1);
        lcd.print("   ");
        runtimeInterval = max(runtimeInterval - 1, 1);
        break;
      case 4:  // This case will execute if the "back" button is pressed
        button = 0;
        activeButton = 1;
        break;
    }
  }
}

void SetMaxPumpRunTime()
{
  int activeButton = 0;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Max. pump time");

  while (activeButton == 0) {

    lcd.setCursor(0, 1);
    lcd.print(pumpRunTime);
    lcd.setCursor(5, 1);
    lcd.print("seconds.");

    int button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 2:
        lcd.setCursor(0, 1);
        lcd.print("   ");
        pumpRunTime = min(pumpRunTime + 1, maxPumpRunTime);
        break;
      case 3:
        lcd.setCursor(0, 1);
        lcd.print("   ");
        pumpRunTime = max(pumpRunTime - 1, 1);
        break;
      case 4:  // This case will execute if the "back" button is pressed
        button = 0;
        activeButton = 1;
        break;
    }
  }
}

void Start()
{
  int activeButton = 0;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Prev. sample: ");

  while (activeButton == 0) {
    unsigned long currentLoopMillis = millis();
    if ((currentLoopMillis - previousLoopMillis >= runtimeInterval * 60000) || (previousLoopMillis == 0)) // Check for runtimeinterval or first run.
    {
      avgSoil = SampleSoil(sampleDelay, samples);
      lcd.setCursor(0, 1);
      lcd.print(avgSoil);
      Serial.println(avgSoil);
      Serial.println(dryValue);
      if (avgSoil >= dryValue)  // If the average soil value is dry
      {
        DispenseH2O();
      }
      previousLoopMillis = millis();
    }

    int button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 4:  // This case will execute if the "back" button is pressed
        button = 0;
        activeButton = 1;
        break;
    }
  }
}

// Remove delay and use millis instead
double SampleSoil(int sampleInterval, int samples)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Measuring... ");
  for (int i = 0; i < samples; i++)  // For loop to calculate average soil readings
  {
    digitalWrite(sensorPowerPin, HIGH); // Sensor on
    delay(50); // Allow sensor to stabilize
    soilValue = analogRead(sensorPin);
    digitalWrite(sensorPowerPin, LOW); // Sensor off
    delay(sampleInterval * 1000);
    avgSoil += soilValue;
  }
  return avgSoil / samples;
}

void DispenseH2O()
{
  //Serial.println("Dispensing H2O.");
  digitalWrite(sensorPowerPin, HIGH); // Sensor on
  digitalWrite(pumpPin, HIGH);  // Start pump
  lcd.setCursor(0, 0);
  lcd.print("Dispensing H2O.");

  unsigned long startMillis = millis();
  while ((soilValue >= wetValue) && (millis() <= startMillis + (pumpRunTime * 1000))) // While loop to keep watering until the soil is wet or pumpMaxTime is reached.
  {
    soilValue = analogRead(sensorPin);
    lcd.setCursor(0, 1);
    lcd.print(soilValue);
    //Serial.println(soilValue);
    delay(50);
    lcd.print("    ");
  }
  digitalWrite(pumpPin, LOW); // Stop pump
  digitalWrite(sensorPowerPin, LOW); // Sensor off
  avgSoil = soilValue;  // Resets average soil value
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Prev. sample: ");
  lcd.setCursor(0, 1);
  lcd.print(avgSoil);
}

