#include <Servo.h> // Library for servo motors --- NOTE: Including this library disables analogWrite() on pins 9+10
#include <math.h> // Library for math functions. Required for trigonometry to calculate rotations.

#define joystickX A0 // Defining the pins assigned to each joystick.
#define joystickY A1
#define servoPin 9
#define generalOnOffPin 2
#define waterOnOffPin 3

#define generalOnOffLEDPin 4
#define waterOnOffLEDPin 5
#define waterPumpControlPin 6

int joyStickPositionX; // Tracks the X position of the joystick.
int joyStickPositionY; // Tracks the Y position of the joystick.
int servoPosition;
bool isDeviceOn = false; // Tracks whether or not the device should be on or off.
bool isDeviceWaterOn = false;
String cmdStr;

bool isGeneralOnOffButtonDepressed;
bool isWaterOnOffButtonDepressed;

Servo rotationServo; // Defines a servo object.

void checkGeneralOnOffBtn()
{
  if (digitalRead(generalOnOffPin) == HIGH && isGeneralOnOffButtonDepressed == false)
  {
    isGeneralOnOffButtonDepressed = true;
    if (isDeviceOn == false)
    {
      isDeviceOn = true;
    }
    else
    {
      isDeviceOn = false;
      isDeviceWaterOn = false; // We want to turn the water off if the device is off!
    }
  }
  else if (digitalRead(generalOnOffPin) == LOW && isGeneralOnOffButtonDepressed == true)
  {
    if (isGeneralOnOffButtonDepressed == true)
    {
      isGeneralOnOffButtonDepressed = false;
    }
  }
}

void checkWaterOnOffBtn()
{
if (digitalRead(waterOnOffPin) == HIGH && isWaterOnOffButtonDepressed == false)
  {
    isWaterOnOffButtonDepressed = true;
    if (isDeviceWaterOn == false && isDeviceOn == true)
    {
      isDeviceWaterOn = true;
    }
    else
    {
      isDeviceWaterOn = false;
    }
  }
  else if (digitalRead(waterOnOffPin) == LOW && isWaterOnOffButtonDepressed == true)
  {
    isWaterOnOffButtonDepressed = false;
  }
}

bool doesJoystickPassThreshold(int joystickX, int joystickY) // This function checks if the joystick passes the position threshold required to cause an activation. The joystick must be moved at 255 units away from its default position before it will register an activation.
{
  const int defaultPosX = 511; // The default "straight up" positions for a joystick.
  const int defaultPosY = 511;
  if ((joystickX >= (defaultPosX + 255)) || (joystickX <= (defaultPosX - 255)) || (joystickY >= (defaultPosY + 255)) || (joystickX <= (defaultPosX - 255))) {return true;}; // If it's more than 255 units away from the default position, return true. Otherwise, return false.
  return false;
}

bool checkIfAngleInBackwardsDeadzone(float angle) // This checks if the joystick is in the 90 degree dead zone surrounding the furthest back of the joystick. This prevents the servo motor from flipping back and forth over and over because of human imprecision on the joystick.
{
  if (angle > 225.0f && angle < 315.0f) {return true;}
  return false;
}

float setAngleInRange(float angle) // Takes in an angle in degrees and makes sure it is within the range of 0 and 360 degrees by adding or subtracting in units of 360 degrees until it's in the required range.
{
  bool angleIsValid = false;
  while (angleIsValid == false)
  {
    if (angle >= 0.0f && angle <= 360.0f)
    {
      angleIsValid == true;
      break;
    }
    if (angle < 0.0f)
    {
      angle += 360.0f;
    }
    else if (angle > 360.0f)
    {
      angle -= 360.0f;
    }
  }
  return angle;
}

float calculateAngleFromJoystick(int joystickX, int joystickY) 
// This basically determines the angle that a servo motor should be at given a value between 0 and 180 degrees, with 0 being far left and 180 being far right. 
// Due to limitations of servo motors, values between 181 and 360 degrees should instead just be set to their maximums. (181 - 225 = 180, 315 - 360 = 0).
// A 45 degree dead zone is used to prevent the servo from rapidly flipping back and forth if the joystick is held backwards due to imprecision in how these devices are held by humans.
{
  // X: 0, Y: 511 = 0 degrees. X: 511, Y: 1023 = 90 degrees. X: 1023, Y: 511 = 180 degrees.

  const int defaultPosX = 511; // The default "straight up" positions for a joystick.
  const int defaultPosY = 511;

  joystickX -= defaultPosX; // By setting these to equal 0 at default position, this calculation is a lot easier to perform.
  joystickY -= defaultPosY;

  float angle = (atan2(float(joystickX), float(joystickY))) * (180 / M_PI); // This calculates the the raw angle of the joystick in radians and then converts it to degrees.
  angle += 90.0f; // We want to add 90 to the result of angle. This is because the default position of 0 is actually left instead of up due to how the servo motor works, so we need to account for that. By doing this, we get an expected result when the joystick is tilted to the left.
  angle = setAngleInRange(angle); // This makes sure the angle is in the range of 0 and 360 degrees.
  return angle;
}

int calculateServoPositionFromAngle(float angle)
{
  if (angle >= 180.0f && angle <= 270.0f) // Sets the servo angle to 180 if the angle is in this range. Since the deadzone check is already performed earlier, we don't need to worry about the backwards dead zone here.
  {
    return 0;
  }
  else if (angle >= 270.0f && angle <= 360.0f) // Does the same as above, but for the left side.
  {
    return 180;
  }
  return int(angle); // If it isn't in those zones, we can just set it as such.
}

// DEBUG FUNCTIONS //

bool compareStrings(String str1, String str2) // This function was made after having multiple issues due to line endings to simplify string comparisons.
{
  if (str1.equals(str2) || str1.equals(str2 + "\n") || str1.equals(str2 + "\r") || str1.equals(str2 + "\n\r") || str1.equals(str2 + "\r\n"))
  {
    return true;
  }
  return false;
}

void clearTerminal() // Clears the terminal by adding a ton of whitespace because this is how this works apparently.
{
  for (int i = 0; i < 50; ++i)
  {
    Serial.println("\n");
  }
  return;
}

void printTerminalItem(String printStr)
{
  clearTerminal();
  for (int i = 0; i < 20; ++i)
  {
    Serial.print('-');
  }
  Serial.print("\n\n");
  Serial.println(printStr);
  Serial.print("\n");
  for (int i = 0; i < 20; ++i)
  {
    Serial.print('-');
  }
}

void printJoystickPositions()
{
  int joyX = analogRead(joystickX);
  int joyY = analogRead(joystickY);
  printTerminalItem("### Joystick Position ###\n\nJoystick X: " + String(joyX) + "\nJoystick Y: " + String(joyY));
  return;
}

void printDeviceOnStatus()
{
  if (isDeviceOn)
  {
    printTerminalItem("### Device Status ###\n\nDevice is on.");
  }
  else
  {
    printTerminalItem("### Device Status ###\n\nDevice is off.");
  }
  return;
}

void printDeviceWaterStatus()
{
  if (isDeviceWaterOn)
  {
    printTerminalItem("### Device Water Status ###\n\nDevice Water is on.");
  }
  else
  {
    printTerminalItem("### Device Watyh67yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyer Status ###\n\nDevice Water is off.");
  }
  return;
}

void printServoPosition()
{
  printTerminalItem("### Servo Position ###\n\n Servo position is: " + String(servoPosition));
}

void testServo()
{
  printTerminalItem("### Servo Test (1/3) ###\n\nMoving servo to position 0.");
  rotationServo.write(0);
  delay(1000);
  printTerminalItem("### Servo Test (2/3) ###\n\nMoving servo to position 90.");
  rotationServo.write(90);
  delay(1000);
  printTerminalItem("### Servo Test (3/3) ###\n\nMoving servo to position 180.");
  rotationServo.write(180);
  delay(1000);
  printTerminalItem("### Servo Test (Complete) ###\n\nServo test complete!");
  return;
}

void handleSerialCommands()
{
  if (Serial.available())
  {
    cmdStr = Serial.readStringUntil('\n');
    if (compareStrings(cmdStr, "joystick"))
    {
      printJoystickPositions();
    }
    else if (compareStrings(cmdStr, "device status"))
    {
      printDeviceOnStatus();
    }
    else if (compareStrings(cmdStr, "device water status"))
    {
      printDeviceWaterStatus();
    }
    else if (compareStrings(cmdStr, "servo position"))
    {
      printServoPosition();
    }
    else if (compareStrings(cmdStr, "test servo"))
    {
      testServo();
    }
  }
  return;
}

// SETUP AND LOOP //

void setup()
{
  rotationServo.attach(9); // Connects the servo motor to pin 9.
  pinMode(waterPumpControlPin, OUTPUT); // Set pin 6 to output as it controls the water pump.
  pinMode(generalOnOffLEDPin, OUTPUT);
  pinMode(waterOnOffLEDPin, OUTPUT);
  servoPosition = 90;
  rotationServo.write(servoPosition);
  Serial.begin(9600);
  clearTerminal();
  isGeneralOnOffButtonDepressed = false;
  isWaterOnOffButtonDepressed = false;
  Serial.println("--------------------\n");
  Serial.println("Started!\n");
  Serial.println("--------------------");
}

void loop()
{
  handleSerialCommands();
  joyStickPositionX = analogRead(joystickX);
  joyStickPositionY = analogRead(joystickY);
  checkGeneralOnOffBtn();
  checkWaterOnOffBtn();
  if (isDeviceOn) // Don't do anything if the device is turned off. 
  {
    digitalWrite(generalOnOffLEDPin, HIGH);
    if (doesJoystickPassThreshold(joyStickPositionX, joyStickPositionY)) // Checks if the joystick position passes the required thresholds. If it doesn't, then do not do anything. This creates a "dead zone" which prevents the motor from responding to movements too small to matter as well as general analog noise.
    {
      float angle = 180.0f - calculateAngleFromJoystick(joyStickPositionX, joyStickPositionY); // Because of how the servo got mounted, it was controlling backwards. By setting it up this way, we effectively invert the controls of the servo, getting the desired result.
      if (checkIfAngleInBackwardsDeadzone(angle) == false)
      {
        servoPosition = calculateServoPositionFromAngle(angle);
      }
    }
    if (isDeviceWaterOn) // Turns the water control on or off.
    {
      digitalWrite(waterPumpControlPin, HIGH);
      digitalWrite(waterOnOffLEDPin, HIGH);
    }
    else
    {
      digitalWrite(waterPumpControlPin, LOW);
      digitalWrite(waterOnOffLEDPin, LOW);
    }
    rotationServo.write(servoPosition);
  }
  else
  {
    digitalWrite(generalOnOffLEDPin, LOW);
  }
}
