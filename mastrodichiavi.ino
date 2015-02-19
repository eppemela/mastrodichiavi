#include <Adafruit_NeoPixel.h>
#include <Stepper.h>



//Arduino PIN definition
#define lockswitch 4 // the digital pin with the microswitch connected to the lock
#define nfcswitch 5 // the digital pin where arrives the signal from the Came RBM21
#define neopixel_pin 6  // the digital pin the neopixel ring is connected to
#define closingswitch 7 // the digital pin with the closing switch attached to

//Defining some colors
#define all_off strip.Color(0, 0, 0)
#define low_red strip.Color(3, 0, 0)
#define med_red strip.Color(20, 0, 0)
#define hi_red strip.Color(40, 0, 0)
#define low_green strip.Color(0, 3, 0)
#define med_green strip.Color(0, 20, 0)
#define hi_green strip.Color(0, 40,0)
#define orange strip.Color(139, 69, 0)


int timer_before_closing_duration = 2000; // 2 seconds * 24 pixels = 48 secs before closing

int door_open = 0; // pseudo digital
int auto_lock_switch_time = 2; // in seconds

unsigned long time_door = millis();
unsigned long time_switch = millis();

long debounce = 500;
boolean locked = false;
boolean auto_lock = false;
boolean prev_status = false;


//Defining the motor shield pins
const int pwmA = 3;
const int brakeB = 8;
const int brakeA = 9;
const int pwmB = 11;
const int dirA = 12;
const int dirB = 13;

// Steps 200 is a complete round
const int STEPS = 208;
const int TURN_FIXTURE = 108;

// Initialize the Stepper
Stepper stepperMotor(STEPS, dirA, dirB);

//Initialize the NeoPixel Led Ring
Adafruit_NeoPixel strip = Adafruit_NeoPixel(24, neopixel_pin, NEO_GRB + NEO_KHZ800);






void setup() {

  // Open serial communications
  Serial.begin(9600);


  // send an intro:
  Serial.println("\n\n Mastro di chiavi [Booting...]\n");
  Serial.println();

  //Initialize the led ring
  strip.begin();
  strip.show();

  // Initialize the stepper motor
  pinMode(pwmA, OUTPUT);
  pinMode(pwmB, OUTPUT);
  pinMode(brakeA, OUTPUT);
  pinMode(brakeB, OUTPUT);

  pinMode(lockswitch, INPUT);
  pinMode(closingswitch, INPUT);
  pinMode(nfcswitch, INPUT);


  // Set the stepper rotation speed a good value found with a potentiometer is ~ 65/75 rpm
  stepperMotor.setSpeed(65);

  //Booting routine
  theaterChase(strip.Color(  0,   0, 127), 50, 50); // Blue
  color_wipe(strip.Color(0, 0, 0), 50);    // Black/off

  //Randomize seed
  randomSeed(analogRead(0));


  // send an intro:
  Serial.println("\n\n Mastro di chiavi [Ready]\n");
  Serial.println();
}



void loop() {

  openSwitch();
  doorSensor();
  closeSwitch();

}


/*Controlla lo status della porta (aperto\chiuso)*/
void doorSensor(){
  Serial.println("\n doorSensor() \n");
  // pseudo digital
  door_open = analogRead(lockswitch);
  Serial.println("door_open:");
  Serial.print(door_open, DEC);
  Serial.println("\ntime_door:");
  Serial.print(time_door, DEC);
  Serial.println("\ndebounce:");
  Serial.print(debounce, DEC);

  //Serial.println("millis() =" + millis());

  if(millis() - time_door > debounce){
    if (door_open <= 500 && prev_status == false){
      Serial.println("Door: opened");
      prev_status = true;
      locked = false;
      fade_up(200, 10, 0, 40, 0); //hi green
    }else  if(door_open > 500 && prev_status == true){
      Serial.println("Door: closed");
      prev_status = false;
      fade_up(200, 10, 40, 0, 0); //hi red
    }
    time_door = millis();
  }
}


void closeSwitch(){
  Serial.println("closeSwitch");

  int dstimer = 0;
  int door_switch = analogRead(closingswitch); // pseudo digital
  if(millis() - time_switch > debounce && door_switch >= 300){
    while (analogRead(closingswitch) >= 300) {
      delay(100);
      dstimer++;
    }
    Serial.println(door_switch,DEC);
    Serial.println(dstimer,DEC);
    if (dstimer < auto_lock_switch_time*10) { //button has been pressed less than 2 seconds = 1000/100
        if (locked == false){
          turn_key("close");
          Serial.println("door locked");
          locked = true;

        }else if(locked == true){
          turn_key("open");
          Serial.println("door unlocked");
          locked = false;
      }
    }else {
      // auto_unlock off/on
      if(auto_lock == true){
        Serial.println("auto_lock off");
        auto_lock = false;
      }else{
        Serial.println("auto_lock on");
        auto_lock = true;
      }
      //analogWrite(ledIN, 0); // resetting output

    }
    time_switch = millis();
  }
}

void openSwitch(){
  Serial.println("openSwitch");

  int dstimer = 0;
  int opener_switch = analogRead(nfcswitch); // pseudo digital
  Serial.println(opener_switch,DEC);

  if(millis() - time_switch > debounce && opener_switch >= 300){
    while (analogRead(nfcswitch) >= 300) {
      delay(100);
      dstimer++;
    }
    Serial.println(opener_switch,DEC);
    Serial.println(dstimer,DEC);
    if (dstimer < auto_lock_switch_time*10) { //button has been pressed less than 2 seconds = 1000/100
      if (locked == false){
        Serial.println("door already open");

        }else if(locked == true){
          turn_key("open");
          Serial.println("door unlocked");
          locked = false;
        }
      }
    }
    time_switch = millis();
}



void turn_key(String direction){

  uint16_t i,j;

  //Two turns...
  for( j=0; j<2; j++){
    for (i=0; i< strip.numPixels();i++){
      switch(j){
        case 0:
          if(direction == "close"){
            strip.setPixelColor(i, low_red);
          }else{
            strip.setPixelColor(i, low_green);
          }

        break;

        case 1:
          if(direction == "close"){
            strip.setPixelColor(i, med_red);
            }else{
              strip.setPixelColor(i, med_green);
          }
        break;

      }

      digitalWrite(pwmA, HIGH);
      digitalWrite(pwmB, HIGH);
      digitalWrite(brakeA, LOW);
      digitalWrite(brakeB, LOW);

      if(direction == "close"){
          stepperMotor.step(-(STEPS / strip.numPixels()));
          locked = true;
        }else{
          stepperMotor.step(STEPS / strip.numPixels());
          locked = false;
      }

      digitalWrite(pwmA, LOW);
      digitalWrite(pwmB, LOW);
      digitalWrite(brakeA, HIGH);
      digitalWrite(brakeB, HIGH);

      strip.show();
    }
  }

  //...and an half
  for (int k=0; k < TURN_FIXTURE; k++){
    if(direction == "close"){
      stepperMotor.step(-1);
    }else{
        stepperMotor.step(1);
    }

  }

  if(direction == "close"){
      fade_up(200, 10, 40, 0, 0); //hi red
    }else{
      fade_up(200, 10, 0, 40, 0); //hi green
  }

}//turn_key


// Function to get some random color, to be used with strip.setPixelColor(i, random_color());
uint32_t random_color(){
  long R,B,G;
  uint32_t rnd_color = strip.Color(random(0,255), random(0,255), random(0,255));
  return rnd_color;
}

// This is the function that is fired before the lock gets closed, for some fancy effects on the ledring
void timer_before_closing(){

  randomSeed(analogRead(0));
  uint32_t trick = random(1,6);

  switch(trick){
    case 1:
      theaterChaseRainbow(50); //A 50ms delay corresponds to ~40 sec loop
    break;

    case 2:
      theaterChase(random_color(),50,250); //A 50ms delay with 250 cycles corresponds to ~40 sec loop
    break;

    case 3:
      rainbow(150); //A 150ms delay corresponds to ~40 sec loop
    break;

    case 4:
      rainbowCycle(50); //A 50ms delay corresponds to ~40 sec loop
    break;

    case 5:
      fade_up(100, 20, 238, 238, 0); //yellow
      color_wipe(orange, timer_before_closing_duration); // orange
    break;

  }



}//timer_before_closing


// fade_up - fade up to the given color
void fade_up(int num_steps, int wait, int R, int G, int B) {
   uint16_t i, j;

   for (i=0; i<num_steps; i++) {
      for(j=0; j<strip.numPixels(); j++) {
         strip.setPixelColor(j, strip.Color(R * i / num_steps, G * i / num_steps, B * i / num_steps));
      }
   strip.show();
   delay(wait);
   }
} // fade_up



// Fill the dots one after the other with a color
void color_wipe(uint32_t c, int wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
} //color_wipe

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*3; j++) { // 3 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait, int numcycles) {
  for (int j=0; j < numcycles; j++) {  //do numcycles cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    } else if(WheelPos < 170) {
      WheelPos -= 85;
      return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
      } else {
        WheelPos -= 170;
        return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
      }
    }
