/* 

libraries required: 
  AlmostRandom              <- for rng
  DFRobot_RGBLCD1602        <- for the lcd screen

pins required:
  interact button           -> digital pin 8
  game inputs               -> digital pins 0,1,2
  speaker                   -> digital pin 9
  
*/

#include <AlmostRandom.h>
#include "DFRobot_RGBLCD1602.h"

// Create an lcd instance for the DFRobot module
DFRobot_RGBLCD1602 lcd(/*RGBAddr*/0x60 ,/*lcdCols*/16,/*lcdRows*/2);  //16 characters and 2 lines of show

// Create an instance of AlmostRandom
AlmostRandom ar;

const int8_t spkr    = 9;       // speaker 
const int8_t act_btn = 8;       // interact button

// input pins
const int8_t inp0 = 0;
const int8_t inp1 = 1;
const int8_t inp2 = 2;          // the thrust will be read as a digital high/low w.r.t to the diode in the input pin

// words of encouragement, 16 character max, when the user give a correct input. the function to print these takes the size of the array
// encouragement can be added and removed at ease of will
const char* natures[] = {
  "Good!", "Good job!", "Happy Happy!", "Correct!", "You're so quick!", "Too fast!", "Remember2Breathe", "Happy feet!", "WomboCombo!"
};

// arbitrary variables used in the win/lose functions
int fq,i;

void setup() {

  lcd.init();     // initialize lcd display


// introduction sequence
  lcd.setCursor(0, 0);
  lcd.print("BopIt Test");
  
  lcd.setCursor(0, 1);
  lcd.print("Welcome Player!");
  
  // enable the timers for ESP32-S3 rng
  #if defined(CONFIG_IDF_TARGET_ESP32S3)
    // Start ESP32S3's timer
    uint32_t* G0T0_CTRL = (uint32_t*)0x6001F000;
    uint32_t* G1T0_CTRL = (uint32_t*)0x60020000;

    *G0T0_CTRL |= (1<<31);
    *G1T0_CTRL |= (1<<31);
  #endif

  pinMode(spkr, OUTPUT);            // speaker is output
  pinMode(act_btn, INPUT_PULLUP);   // reset button, normal high

  pinMode(inp0, INPUT_PULLUP);      // toggle
  pinMode(inp1, INPUT_PULLUP);      // keyboard key
  pinMode(inp2, INPUT_PULLUP);      // thrust
}

void loop() {

  while(!digitalRead(act_btn));   // wait while/if the interact button is pressed

  clear_row(1);
  lcd.print("Press to Begin");

  while(digitalRead(act_btn));    // wait until the interact button is pressed
  while(!digitalRead(act_btn));   // wait while/if the interact button is pressed

  lcd.clear();

  bool valid;
  int8_t ix,x,input,
         score = 99;              // score can be modified here, can also implement a debug mode above to modify the starting value without reprogramming the system

  int time,
      wait    = 3000 - (99 - score)*15,          // input period (time period where the user can respond to the beeps)
      between = 500  - (99 - score)*5;           // period between inputs (time before beeps of next iteration)

  print_score(score);                            // print score on console

  // game loop     (valid and score != 0)
  for(valid = true; valid && score; wait -= 15, between -= 5){  // decrement by 15ms and 5ms

    ix = play();            // play beeps and get formatted input, beep quantity, for input validation

    valid = false;          // initialize as false
    input = ((digitalRead(inp2) << 2) | (digitalRead(inp1) << 1) | digitalRead(inp0));      // get current input as a number

    // scan for input for (time)-ms
    for(time = wait, x = input; x == input && --time; delay(1)){
      x = ((digitalRead(inp2) << 2) | (digitalRead(inp1) << 1) | digitalRead(inp0));        // read current input at pins to scan for changes
    }

    // ix corresponds to the bit in input that was expected to be toggled
    // if the input given, x, is equal to the input before the input period with the expected bit toggled, the user gave a correct input
    valid = (x == (input^ix)); 

    // if the user gave a correct input
    if(valid){
      print_score(--score);                                               // print (update) the score on the top line
      clear_row(1);                                                       // clear the bottom line for the encouragement
      lcd.print(natures[ar.getRandomByte()%(sizeof(natures)/8)]);         // print encouragement
      delay(between);                                                     // delay between next beeps
    }
  }

  clear_row(1);            // clear bottom line
  score ? lose() : win();   // if score == 0, the user successfully completed 99 rounds -> they won

  while(digitalRead(act_btn));    // grab a seat and sit there until the user starts a new a game
}

// beep (0-3)-times
int8_t play(){
  int8_t bb,xi = ar.getRandomByte()%3;
  for(bb = xi + 1; bb--; delay(300)){
    tone(spkr, 2250, 200);
  }
  
  return (1 << xi);     // xi corresponds to the bit in the formatted number (line 95) that is expected to be toggled
}

// will print the score on the top row
void print_score(int score){
  lcd.setCursor(0, 0);          // set cursor to top row
  lcd.print("Score:");          // print score header

  lcd.setCursor(14, 0);         // set cursor two cells back from end
  lcd.print(99 - score);        // print score
}

// will clear either the top or bottom row
void clear_row(bool i){
  lcd.setCursor(0, i);                  // set cursor to the row
  lcd.print("                ");        // print 16 empty characters
  lcd.setCursor(0, i);                  // normalize cursor
}

// emit "congratulations" noise sequence
void win(){
  lcd.setCursor(0, 1);                  // set cursor to bottom row
  lcd.print("Winner! Replay?");         // print winning statement

  // arbitrary winner tone sequence
  tone(spkr, 3000, 200);
  delay(250);
  
  for(fq = 2500, i = 4; --i; fq += 500, delay(150)){
    tone(spkr, fq, 125);
  }
}

// emit "game lost" noise sequence
void lose(){
  lcd.setCursor(0, 1);                  // set cursor to bottom row
  lcd.print("Loser! Replay?");          // print losing statement :(

  // arbitrary loser tone sequence
  for(fq = 400, i = 4; --i; fq -= 100, delay(400)){
    tone(spkr, fq, 300);
  }
  tone(spkr, fq, 1000);
}
