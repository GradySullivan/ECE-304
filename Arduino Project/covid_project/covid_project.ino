#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(4);

#define YesButton A0
#define NoButton A1
#define tempPin A2

#define GreenLED 8
#define YellowLED 9
#define RedLED 10

#define MOTOR 11

#define TRIG 12
#define ECHO 13

const int numQuestions = 11;

float distance, duration;
float prevDistance = 0;

float occupancy = 0;
const float maxOccupancy = 50;
float perOccupancy; // percent occupancy (used for led light determination)

void setup() 
{
  
  Serial.begin(9600); //start serial connection

  // initialize display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();

  //initialize buttons
  pinMode(YesButton, INPUT_PULLUP); //configure as an input and enable the internal pull-up resistor
  pinMode(NoButton, INPUT_PULLUP);  // HIGH when open, LOW when pressed

  //initialize LEDs
  pinMode(GreenLED, OUTPUT);
  pinMode(YellowLED, OUTPUT);
  pinMode(RedLED, OUTPUT);
  
  //initialize motor
  pinMode(MOTOR, OUTPUT);

  //initialize distance sensor
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  pinMode(7, OUTPUT);
}

void loop() 
{
  boolean passed_questionTest, passed_tempTest; //whether user passes tests

  checkLeaving();
  
  displayMessage("Press 'Yes' to start COVID-19 screening");
  if (digitalRead(YesButton) == LOW)
  {
    passed_tempTest = tempTest();
  if (passed_tempTest == true)
    {
      passed_questionTest = questionTest();
      if (passed_questionTest == true)
        {
          displayMessage("You may enter.");
          occupancy++;
          perOccupancy = occupancy / maxOccupancy; //recalculate percent occupancy
          Serial.println(perOccupancy);
          checkOccupancy(perOccupancy);
          powerMotor();
        }    
    }
  }
}

boolean tempTest()
{
  float voltage, tempC, tempF, prevTemp, beginTemp;
  boolean tempChange = false;
  const float minTemp = 70;
  const float maxTemp = 76; //threshold for temp sensor
  const float roomTemp = 68;

  voltage = analogRead(tempPin) * 0.004882814; // voltage from tmp 36
  tempC = (voltage - 0.5) * 100;
  tempF = tempC * 9/5 + 32;
  prevTemp = tempF;
  
  displayMessage("Put your thumb and index finger on the temperature sensor for 5 seconds.");
  
  delay(5000);

  while(1)
  {
    voltage = analogRead(tempPin) * 0.004882814; // voltage from tmp 36
    tempC = (voltage - 0.5) * 100;
    tempF = tempC * 9/5 + 32;
    Serial.println(tempF);
    if(prevTemp - 2 < tempF or tempF > prevTemp + 2) // make sure temp has changed since start
    {
      if(minTemp < tempF and tempF < maxTemp) // if inside "normal range"
    {
      Serial.println("passed");
      return true;
    }
    else
    {
      displayMessage("You may not enter.");
      delay(3000);
      return false;
    }
    }
  }
    
}

boolean questionTest() // give users questions to answer
{
  
  String covidQuestions[numQuestions] = // COVID-19 questions from UMass
  {
    "Please answer 'Yes'  or 'No'. Press yes to continue.",
    "Loss of smell or taste?",
    "Muscle aches?",
    "Sore throat?",
    "Cough?",
    "Shortness of breath?",
    "Chills?", 
    "Headache?",
    "Gastrointestinal symptoms?",
    "Been exposed to COVID-19?",
    "Been in contact with symptomatic people?",
  };

  int i = 0;
  short answered = false; 
  boolean completed = false;
  boolean failed = false;

  while(i<numQuestions and completed == false and failed == false)
    {
      displayMessage(covidQuestions[i]);
      answered = false; // reset answered variable
      delay(250);
      while(answered == false)
      {
        if (i == numQuestions - 1 and digitalRead(NoButton) == LOW) // all answered correctly
          {
            answered = true;
            completed = true;
          }
          
        else if ((i == 0 and digitalRead(YesButton) == LOW) or (i > 0 and digitalRead(NoButton) == LOW)) // correct
          {
            i++;
            answered = true;
          }
        
        else if ((i == 0 and digitalRead(NoButton) == LOW) or (i > 0 and digitalRead(YesButton) == LOW)) // incorrect
        {
          answered = true;
          failed = true;
          displayMessage("You may not enter.");
        }    
      }
    }
  return !failed;
}

void displayMessage(String message)
{
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println(message);
  display.display();
}

void powerMotor()
{
  digitalWrite(MOTOR, HIGH);
  delay(10000);
  digitalWrite(MOTOR, LOW);
}

void checkLeaving()
{
  int i = 0;
  occupancyUpdate();
  distance = getDistance();
  //Serial.println(distance);
  
  while(distance < prevDistance) // to filter out sensor inaccuracies
  {
    prevDistance = distance;
    distance = getDistance();
    if (i == 3) // person dected to be exiting
      {
        Serial.println("leaving");
        occupancy--;
        occupancyUpdate();
        digitalWrite(7, HIGH); // turn test lED on
        powerMotor();
        digitalWrite(7, LOW); // turn test LED off
      }
    Serial.println(i);
    i++;
    delay(30);
  }

  prevDistance = distance;  
}

long getDistance()
{
  digitalWrite(TRIG, LOW);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  duration = pulseIn(ECHO, HIGH);
  distance = duration * 0.017;
  return distance; // in cm
}

void occupancyUpdate()
{      
  perOccupancy = occupancy / maxOccupancy; //recalculate percent occupancy
  
  checkOccupancy(perOccupancy);
  //Serial.print("occupancy ");
  //Serial.println(occupancy);
}

float checkOccupancy(float perOccupancy)
{

  if(perOccupancy < 0.4) // less than 40% capacity
  {
    digitalWrite(GreenLED, HIGH);
    digitalWrite(YellowLED, LOW);
    digitalWrite(RedLED, LOW);
  }
  else if(perOccupancy > 0.8) // more than 80% capacity
  {
    digitalWrite(RedLED, HIGH);
    digitalWrite(GreenLED, LOW);
    digitalWrite(YellowLED, LOW);   
  }
  else // between 40% and 80% capacity
  {
    digitalWrite(GreenLED, LOW);
    digitalWrite(YellowLED, HIGH);
    digitalWrite(RedLED, LOW);
  }
}
