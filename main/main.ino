#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <SPI.h>
#include "BetItFont10pt7b.h"

#define H A0 //hit pin (pin 23)
#define S A1 //stand pin (pin 24)
#define D A2 //double pin (pin 25)
#define P 2 //play button pin (pin 4)
#define spkRX 3 //speaker RX (pin 5)
#define spkTX 4 //speaker TX (pin 6)
#define dispCS 5 // control select (pin 11)
#define dispDC 6 //data select (pin 12)
#define dispRST 7 //display reset (pin 13)



const int moves[3] = {H,S,D};
const double levels[3] = {2.5*204.8,0.5*204.8,1.2*204.8}; //voltage analog thresholds for hit, stand, double; multiplied by ADC steps/max voltage
const bool activeHigh[3] = {true, false, false}; //does voltage increase when hit, stand, or double are used?
const double startTime = 3000; //start time in milliseconds
const double timeMult = 0.98; //time multiplier each round to slow down time
const char suits[4] = {'/', '-', '+', '*'}; //suits, assuming font works
const char ranks[13] = {'2','3','4','5','6','7','8','9','X','J','Q','K','A'}; //ranks
int lives; //number of lives
int score; //score
double time; //current time between reactions
int index; //pointer to next card
int cards[52]; //deck
Adafruit_ILI9341 tft = Adafruit_ILI9341(dispCS,dispDC,dispRST); //initalize display
SoftwareSerial spkSerial(spkTX,spkRX); //serial for speaker
DFRobotDFPlayerMini myDFPlayer;

class Hand{
  public:
    bool soft = false; //ace in hand
    int total = 0; //total except first ace if any
    String disp = "";
    int cardToNum(int card){
      int val=card%13;
      if(val==12) return 1; //ace
      if(val<=8) return val+2; //2 to 10 in index 0 to 8
      return 10; //J,Q,K
    }
    void deal(){
      Serial.println("Draw!");
      int draw = cards[index++]; //return card
      int val = cardToNum(draw); //get value of draw
      if(val==1 && !soft) soft=true; //set soft total for first ace
      else total += val;

      disp += ranks[draw%13]; //add rank to display
      disp += suits[draw/13]; //add suit to display
      disp += ' '; //add space to display
      Serial.println(disp);
      delay(100);
    }
    void clear(){
      soft = false; //ace in hand
      total = 0; //soft total
      disp = "";
    }
    int bestTotal(){
      if(soft){
        if(total>11) return total+1; //total + 1 if hand would be above 21
        else return total+11; //otherwise ace is 11
      }
      return total; //return hard total without ace
    }
    bool dealerHits(){
      return(bestTotal()<17);
    }
};

void updateDisp(Hand player,Hand dealer){ //show cards on display
  tft.fillScreen(ILI9341_WHITE); //clear screen
  tft.setTextColor(ILI9341_BLACK); //black text
  tft.setCursor(0,20); //move cursor to top
  tft.println("Dealer:");
  tft.println(dealer.disp); //dealer hand at top
  tft.println("\nPlayer:");
  tft.println(player.disp); //player hand at bottom
  Serial.print("Player: ");
  Serial.println(dealer.disp);
  Serial.print("Dealer: ");
  Serial.println(player.disp);
  tft.println("\n");
  tft.print("$");
  tft.print(score); //print score
  tft.print("        "); //print tab between score and lives
  for(int i=0;i<lives;i++) tft.print("-"); //print lives
  Serial.println(score);
  Serial.println(lives);
}

//hard total moves
const int hard[10][10] = { //suggested moves for hard totals, hard[max(0,17-i)][j-2] for total i and upcard j
{S,S,S,S,S,S,S,S,S,S},
{S,S,S,S,S,H,H,H,H,H},
{S,S,S,S,S,H,H,H,H,H},
{S,S,S,S,S,H,H,H,H,H},
{S,S,S,S,S,H,H,H,H,H},
{H,H,S,S,S,H,H,H,H,H},
{D,D,D,D,D,D,D,D,D,D},
{D,D,D,D,D,D,D,D,H,H},
{H,D,D,D,D,H,H,H,H,H},
{H,H,H,H,H,H,H,H,H,H}
};

//soft total moves
const int soft[8][10] = { //suggested moved for soft totals, soft[9-i][j-2] for total A,i and upcard j
{S,S,S,S,S,S,S,S,S,S},
{S,S,S,S,D,S,S,S,S,S},
{D,D,D,D,D,S,S,H,H,H},
{H,D,D,D,D,H,H,H,H,H},
{H,H,D,D,D,H,H,H,H,H},
{H,H,D,D,D,H,H,H,H,H},
{H,H,H,D,D,H,H,H,H,H},
{H,H,H,D,D,H,H,H,H,H}
};

int action(Hand player,Hand dealer){
  int p = player.bestTotal();
  int d = dealer.bestTotal();
  if(player.soft) return soft[20-p,d-2]; //20-, not 9- because A = 11
  else return hard[max(0,17-p)][d-2];
  Serial.println(player.soft + " " + String(p) + " " + String(d));
}

int await(double time){
  double start = millis(); //record current time for start
  while(millis() - start < time){
    for(int i=0;i<3;i++)
    if((analogRead(moves[i])>levels[i])==activeHigh[i]) return moves[i]; //read each pin for a move
  }
  return 5;
}

void shuffle(){
  for(int i=0;i<52;i++){ //choose random index to shuffle for each card
    int j = random(52); //random index
    int temp = cards[i];
    cards[i] = cards[j];
    cards[j] = temp;
    Serial.print(cards[i]);
  }
  Serial.println("");
  index=0;
}

int whoWins(Hand player,Hand dealer){//3 for player, 2 for dealer, 1 for push, 0 to continue
  int p = player.bestTotal();
  int d = dealer.bestTotal();
  Serial.println(p);
  Serial.println(d);
  if(p==21 || d>21) return 3; //check for ablackjack or bust no matter what
  if(d==21 || p>21) return 2;
  if(!dealer.dealerHits()){ //check for when dealer stops hitting
    if(p>d) return 3;
    if(d>p) return 2;
    if(d==p) return 1;
  }
  return 0;
}

void setup() {
  delay(1200); //short delay to let power stabilize
  Serial.begin(115200);
  Serial.println("Serial check");
  tft.begin();
  tft.setFont(&BetItFont10pt7b); //match name of original file
  tft.setRotation(1);
  pinMode(8, OUTPUT);
  digitalWrite(8, HIGH);  // turn the LED on (HIGH is the voltage level)
  pinMode(H, INPUT);
  pinMode(S, INPUT);
  pinMode(D, INPUT);
  pinMode(P, INPUT);
  pinMode(dispCS,OUTPUT);
  pinMode(dispDC,OUTPUT);
  pinMode(dispRST,OUTPUT);

  spkSerial.begin(9600);
  delay(1000);

  if (myDFPlayer.begin(spkSerial)) {
    Serial.println("DFPlayer connected");
    myDFPlayer.setTimeOut(500);
    Serial.println("timeout set");
    myDFPlayer.volume(20);
    Serial.println("volume set");    
    myDFPlayer.play(12);
    Serial.println("Playing track 12");
  } else {
    Serial.println("no dfplayer detected, skipping audio setup");
  }
  Serial.println("setup complete");
}

void loop() {
  digitalWrite(8, HIGH);  // turn the LED on (HIGH is the voltage level)
  Serial.println("loop started");
  for(int i=0;i<52;i++) cards[i]=i; //card[i/13] = suit, card[i%13] = rank
  // initialize game and hands
  Hand player;
  Hand dealer;
  tft.fillScreen(ILI9341_RED); //clear screen
  tft.setTextColor(ILI9341_BLACK); //black text
  tft.setCursor(0,20); //move cursor to top
  tft.println("Bet it!\nHit button to play");
  while(digitalRead(P)==LOW);
  Serial.println("Play");
  lives = 3;
  score = 0;
  time = startTime;
  shuffle();
  dealer.deal();//dealer draws
  player.deal(); player.deal(); //player draws twice
  updateDisp(player,dealer);
  delay(1000);
  while((lives>0) && (score<99)){ //game goes until max score or 0 lives
    //TODO: set up actual game environment (cards dealing)
    if(player.bestTotal()==21){ //check for natural blackjack
      myDFPlayer.play(11);
      Serial.println("Natural blackjack");
      delay(1500);
      continue;
    }
    int command = action(player,dealer); //create action
    switch(command){
      case S:
        Serial.println("Stand it");
        myDFPlayer.play(1);
        delay(1500);
        break;
      case H:
        Serial.println("Hit it");
        myDFPlayer.play(2);
        delay(1500);
        break;
      case D:
        Serial.println("Double it");
        myDFPlayer.play(3);
        delay(1500);
        break;
    }
    Serial.println(String(time) + " milliseconds"); //tell action
    int act=await(time); //wait for action
    if(command==act){//if correct
      //TODO
      Serial.println("points");
      updateDisp(player,dealer);
      myDFPlayer.play(4);
      delay(1500);
      score++;
    }
    else{//if incorrect
      //TODO
      Serial.println("life lost");
      updateDisp(player,dealer);
      myDFPlayer.play(5);
      delay(1500);
      lives--;
    }
    switch(act){ //determine how rest of the game goes
      case S:
        Serial.println("Chose stand");
        while (dealer.dealerHits()) dealer.deal(); //keep dealing to dealer
        break;
      case H:
        Serial.println("Chose hit");
        player.deal(); //deal to player
        break;
      case D:
        Serial.println("Chose double");
        player.deal(); //deal once more to player
        while (dealer.dealerHits()) dealer.deal(); //keep dealing to dealer
        break;
      case 5:
        Serial.println("Chose nothing");
        break;
    }
    updateDisp(player,dealer);
    int result = whoWins(player,dealer);
    switch(result){ //check who wins after initial draw and after
      case 3: //player wins
        Serial.println("player wins");
        myDFPlayer.play(6);
        delay(1500);
        break;
      case 2: //dealer wins
        Serial.println("dealer wins");
        myDFPlayer.play(7);
        delay(1500);
        break;
      case 1: //push
        Serial.println("push");
        myDFPlayer.play(8);
        delay(1500);
        break;
      case 0: //nothing
        Serial.println("continue play");
        break;
    }
    time*=timeMult;
    if(score==99){
      Serial.println("WIN");
      Serial.print(String(lives));
      tft.fillScreen(ILI9341_WHITE); //clear screen
      tft.setTextColor(ILI9341_BLACK); //black text
      tft.setCursor(0,20); //move cursor to top
      tft.println("YOU WIN"); //game result
      myDFPlayer.play(9);
      delay(10000);
      break;
    }
    if(lives==0){
      Serial.println("Game over");
      Serial.println(String(score));
      tft.fillScreen(ILI9341_WHITE); //clear screen
      tft.setTextColor(ILI9341_BLACK); //black text
      tft.setCursor(0,20); //move cursor to top
      tft.println("GAME OVER"); //game result
      myDFPlayer.play(10);
      delay(10000);
      break;
    }
    if(result>0){
      Serial.println(dealer.disp);
      Serial.println(player.disp);
      player.clear();
      dealer.clear();
      shuffle();
      dealer.deal();//dealer draws
      player.deal(); player.deal(); //player draws twice
      updateDisp(player,dealer);
    }
  }
}