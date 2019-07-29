#include <Keyboard.h>

const int TriggerThreshold = 50;

void setup() {
  Serial.begin(38400);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(0, INPUT);
  pinMode(1, INPUT);
  pinMode(2, INPUT);
  pinMode(3, INPUT);
  pinMode(4, INPUT);
  pinMode(5, INPUT);
  pinMode(6, INPUT);
  //pinMode(1, INPUT_PULLUP);

  Keyboard.begin();
}

bool lastStatus[6] = {false};

void loop() {
  ///pin mappings for where things got soldered
  int p[6] = {0, 1, 2, 3, 4, 5}; // RIGHT, UP, DOWN, LEFT, SELECT, START
  char c[6] = {'a', 'd', 's', 'w', 'p', 'o'};
  //analog read values
  int a[6] = {0};
 
  //check if any buttons are pressed, so we know whether to light the LED
  bool pressed = false;
 
  //read each pin, and set that Joystick button appropriately
  for(int i = 0; i < 6; ++i)
  {
    a[i] = analogRead(p[i]);
    if(a[i] < TriggerThreshold && !lastStatus[i])
    {
      pressed = true;
      Keyboard.press(c[i]);
      lastStatus[i] = true;
    } else if (a[i] >= TriggerThreshold && lastStatus[i]) {
      lastStatus[i] = false;
      Keyboard.release(c[i]);
    }
  }
 
  //Illuminate the LED if a button is pressed
  if(pressed)
    digitalWrite(LED_BUILTIN, HIGH);
  else
    digitalWrite(LED_BUILTIN, LOW);
  delay(5);
}
