#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <SPI.h>


#define H A0 //hit pin (pin 23)
#define S A1 //stand pin (pin 24)
#define D A2 //double pin (pin 25)
#define P 2 //play button pin (pin 4)
#define dispCS 3 // control select (pin 5)
#define dispDC 4 //data select (pin 6)
#define dispRST 5 //display reset (pin 11)
#define spkRX 6 //speaker RX (pin 12)
#define spkTX 7 //speaker TX (pin 13)


const int moves[3] = {H,S,D};
const double levels[3] = {0,0,0}; //voltage analogthresholds for hit, stand, double
const double startTime = 3000; //start time in milliseconds
const double timeMult = 0.98; //time multiplier each round to slow down time
const String suits[4] = {"♠ ", "♥ ", "♦ ", "♣ "}; //suits
const String ranks[13] = {"2","3","4","5","6","7","8","9","🔟","J","Q","K","A"}; //ranks
int lives; //number of lives
int score; //score
double time; //current time between reactions
int index; //pointer to next card
int order[52]; //order of cards
int cards[52]; //deck

Adafruit_ILI9341 tft = Adafruit_ILI9341(dispCS,dispDC,dispRST); //initalize display
SoftwareSerial spkSerial(spkRX,spkTX); //serial for speaker
DFRobotDFPlayerMini myDFPlayer;

class Hand{
  public:
    int hand[10] = {0,0,0,0,0,0,0,0,0,0};
    int numCards = 0;
    bool soft = false; //ace in hand
    int total = 0; //total except first ace if any
    String disp = "";
    int cardToNum(int card){
      int val=card%16;
      if(val==12) return 1; //ace
      if(val<=8) return val+2; //2 to 10 in index 0 to 8
      return 10; //J,Q,K
    }
    String dispCard(int card){ //convert card number to display
      return (ranks[card%16] + suits[card/16]); //decode card to display
    }
    int deal(){
      index++; //increment card before return
      int draw = cards[order[index-1]]; //return card
      hand[numCards++] = draw; //add card to hand
      int val = cardToNum(draw); //get value of draw
      if(val==1 && !soft) soft=true; //set soft total for first ace
      else total += val;
      disp += dispCard(draw); //add card to display
    }
    int newestCard(){return hand[numCards];}
    void clear(){
      for(int i=0;i<10;i++) hand[i]=0;
      numCards = 0;
      soft = false; //ace in hand
      total = 0; //soft total
      disp = "";
    }
    int bestTotal(){
      if(soft){
        if(total>11) return total+1; //total + 1 if hand would be above 21
        else return total+11; //otherwise ace is 11
      }
      else return total; //return hard total without ace
      return total;
    }
    bool dealerHits(){
      return(bestTotal()<17);
    }
};

void updateDisp(Hand player,Hand dealer){ //show cards on display
  tft.fillScreen(ILI9341_WHITE); //clear screen
  tft.setTextColor(ILI9341_BLACK); //black text
  tft.setCursor(0,0); //move cursor to top
  tft.println(dealer.disp); //dealer hand at top
  tft.println(player.disp); //player hand at bottom
}

//hard total moves
const int hard[10][10] = { //suggested moves for hard totals, hard[min(0,17-i)][j-2] for total i and upcard j
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
{H,H,H,D,D,H,H,H,H,H},
};

int action(Hand player,Hand dealer){
  //if soft above 10, play as hard (total+1)
  //soft 10 is blackjack lol

  return 0;
}

int await(double time){
  double start = millis(); //record current time for start
  while(millis() - start < time){
    for(int i=0;i<3;i++)
    if(analogRead(moves[i])>levels[i]) return moves[i]; //read each pin for a move
  }
  return 0;
}

void shuffle(){
  for(int i=0;i<52;i++) order[i]=i;
  for(int i=0;i<52;i++){ //choose random index to shuffle for each card
    int j = random(52); //random index
    int temp = order[i];
    order[i] = order[j];
    order[j] = temp;
  }
  index=0;
}

int whoWins(Hand player,Hand dealer){//3 for player, 2 for dealer, 1 for push, 0 to continue
  int p = player.bestTotal();
  int d = dealer.bestTotal();
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
  pinMode(H, INPUT);
  pinMode(S, INPUT);
  pinMode(D, INPUT);
  pinMode(P, INPUT);
  pinMode(dispCS,OUTPUT);
  pinMode(dispDC,OUTPUT);
  pinMode(dispRST,OUTPUT);
  Serial.begin(115200);
  Serial.println("Check check!");

  spkSerial.begin(9600);
  myDFPlayer.setTimeOut(500);
  myDFPlayer.volume(0);
  myDFPlayer.EQ(0);
}

void loop() {
  for(int s=0;s<4;s++) for(int r=0;r<13;r++) cards[13*s+r] = 16*s+r; //card[5] = suit, card[3-0] = rank
  // initialize game and hands
  Hand player;
  Hand dealer;
  while(digitalRead(P)==LOW); //wait for play button to be hit
  lives = 3;
  score = 0;
  time = startTime;
  while((lives>0) && (score<99)){ //game goes until max score or 0 lives
    //TODO: set up actual game environment (cards dealing)
    dealer.deal();//dealer draws
    player.deal(); player.deal(); //player draws twice
    updateDisp(player,dealer);
    if(player.bestTotal()==21){ //check for natural blackjack
      myDFPlayer.play(11);
      Serial.println("Natural blackjack");
      continue;
    }
    int command = action(player,dealer); //create action
    switch(command){
      case S:
        Serial.println("Stand it");
        myDFPlayer.play(1);
        break;
      case H:
        Serial.println("Hit it");
        myDFPlayer.play(2);
        break;
      case D:
        Serial.println("Double it");
        myDFPlayer.play(3);
        break;
    }
    //tell action
    int act=await(time); //wait for action
    if(command==act){//if correct
      //TODO
      Serial.println("points");
      myDFPlayer.play(4);
      score++;
    }
    else{//if incorrect
      //TODO
      Serial.println("life lost");
      myDFPlayer.play(5);
      lives--;
    }
    switch(act){ //determine how rest of the game goes
      case S:
        while (dealer.dealerHits()) dealer.deal(); //keep dealing to dealer
        updateDisp(player,dealer);
        break;
      case H:
        player.deal(); //deal to player
        break;
      case D:
        break;
      case 0:
        break;
    }
    switch(whoWins(player,dealer)){ //check who wins after initial draw and after
      case 3: //player wins
        Serial.println("player wins");
        myDFPlayer.play(6);
        break;
      case 2: //dealer wins
        Serial.println("dealer wins");
        myDFPlayer.play(7);
        break;
      case 1: //push
        Serial.println("push");
        myDFPlayer.play(8);
        break;
      case 0: //nothing
        break;
    }
    time*=timeMult;
    if(score==99){
      Serial.println("WIN");
      myDFPlayer.play(9);
    }
    if(lives==0){
      Serial.println("Game over");
      myDFPlayer.play(10);
    }
  }
}
