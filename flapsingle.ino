/*
Fallblattanzeiger 
Versions chronology:
version 1   - 18.10.2020 Simon Widmer 
*/


#include <Wire.h>
#include <SoftwareSerial.h>

#define RX        3    // Soft Serial RS485 Receive pin
#define TX        6    // Soft Serial RS485 Transmit pin
#define RTS       7    // RS485 Direction control
#define POT       A0    // Potentiometer
#define PB1       2    // Pushbutton
#define PB2       9    // Pushbutton
#define LED1      4    // DIP1
#define LED2      5   // DIP2

#define RS485Transmit    HIGH
#define RS485Receive     LOW
#define RS485_BDRATE     19200

#define TIMEOUT_DELAY    100
#define MAX_ADDR          30

#define TYPE_61         66
#define TYPE_40         65

SoftwareSerial RS485Serial(RX, TX);

byte stat_blank[] = {0,30,32,34,36,41,42,43,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61};

unsigned long lastRandom1 = 0; //last time new station appeared
unsigned long presetRandom1 = 0; //

byte pos = 0;
byte addr_stat = 0;
byte type = 0;
byte maxflap = 30;

//
// Methods
//_____________________________________________________________________________________________


//Initialize Timers for random interventions
void init_timers(){
  lastRandom1 = millis();
  presetRandom1 = 10000L+8000L*analogRead(POT);
  Serial.print("Timerinit: "); 
  Serial.print(presetRandom1/1000); 
  Serial.println(" s"); 
}

// Detect Adress and Type
bool detectmodule(){
  bool module_ok=false;
  while (!module_ok && addr_stat < MAX_ADDR){
    if(getflaptype(addr_stat))
    {
      module_ok=true;
    }
    else
    {
      ++addr_stat;
    }
  }
  if(module_ok)
  {
    Serial.print("Modul mit Adresse ");
    Serial.print(addr_stat);
    Serial.print(" vom Typ ");
    Serial.print(type);
    Serial.println(" gefunden");
    digitalWrite(LED1, HIGH);

  }
  else
  {
    Serial.println("Kein Modul gefunden");    
  }
  
}

// Generate Number 1-maxflap which is not blanked
void sf_setflaprandom(){
  //Generate Number between 1 and maxflap which is not blanked
  bool result=false;
  do {
    result = true;
    pos = random(1,maxflap);
    // Check Blanking Array
    for (byte i = 0; i < (sizeof(stat_blank) / sizeof(stat_blank[0])); i++) {
      if (pos == stat_blank[i]){
        result = false;
      }
    } 
  }while(result == false);
  setflap(addr_stat, pos);
}

//Random Station Generator
void checkrandom1(){
  if ((millis() - lastRandom1) > presetRandom1){
    sf_setflaprandom();
    lastRandom1 = millis();

    //Generate new interval, potentiometer is multiplier
    // Maximum 22.7h, minimum every Second.
    presetRandom1 = 10000L+8000L*analogRead(POT);   
    Serial.print("Timer: "); 
    Serial.print(presetRandom1/1000); 
    Serial.println(" s"); 
  }
}

// Send RS485 Break
void sendBreak(unsigned int duration){
  RS485Serial.flush();
  RS485Serial.end();
  pinMode(TX, OUTPUT);
  digitalWrite(RTS, RS485Transmit);
  digitalWrite(TX, LOW);
  delay(duration);
  digitalWrite(TX, HIGH);
  //digitalWrite(RTS, RS485Receive);
  RS485Serial.begin(RS485_BDRATE);
}

// Set Splitflap
void setflap(byte address, byte pos){
  sendBreak(60);
  RS485Serial.write(0xFF);
  RS485Serial.write(0xC0);
  RS485Serial.write(address);
  RS485Serial.write(pos);
  delay(200); // Necessary??
  Serial.print("Neue Position: ");
  Serial.println(pos);
}

bool getflaptype(byte address){
  sendBreak(60);
  RS485Serial.write(0xFF);
  RS485Serial.write(0xDD);
  RS485Serial.write(address);
  digitalWrite(RTS, RS485Receive);
  
  unsigned long start = millis();
  unsigned long timeout = TIMEOUT_DELAY; 
  bool to = false;
  
  while (RS485Serial.available() == 0 && to == false) {
      unsigned long now = millis();
      unsigned long elapsed = now - start;
      if(elapsed > timeout)
        to = true;
  }
  if (RS485Serial.available()>0)
  {
    type = RS485Serial.read();
    switch(type){
      case TYPE_61:
        maxflap = 60;
        Serial.println("Modul 60 detektiert");
        break;
      case TYPE_40:
        maxflap = 40;
        Serial.println("Modul 40 detektiert");
        break;
      default:
        Serial.println("Unbekanntes Modul");
        break;
    }
    return true;
  }
  else
  {
    Serial.println("Timeout");
    return false;
  }
}

//Show blank Flaps
void sf_noshow(void){
  setflap(addr_stat, 0);
}

// Move Flap forward with Button
void addflap(){
  if(pos>=maxflap){
    pos=0;
  }
  else
    ++pos;
    
  setflap(addr_stat, pos);
}


//
// SETUP
//_____________________________________________________________________________________________

void setup() {
  // Init Random Seed
  randomSeed(analogRead(5));
    
  pinMode(RTS, OUTPUT);  
  pinMode(LED1, OUTPUT);  
  pinMode(LED2, OUTPUT);  
  pinMode(PB1, INPUT);  
  pinMode(PB2, INPUT);  
  attachInterrupt(digitalPinToInterrupt(PB1), addflap, FALLING);

  digitalWrite(LED1, HIGH);
  digitalWrite(LED2, HIGH);
    
  // Start the built-in serial port, for Serial Monitor
  Serial.begin(115200);
  Serial.println("*******************************************************************************"); 
  Serial.println("Fallblattanzeiger"); 

  //Init Timers
  init_timers();
  
  // Serial for RS485
  RS485Serial.begin(RS485_BDRATE);  
  delay(100);

  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);

  detectmodule();
  
  sf_noshow();
  delay(3000);
  sf_setflaprandom();
  
}  // end of setup

void loop() {
   checkrandom1();
}     // end of loop
