/*
Rejurhf 
21.04.2018
*/
#include <LedControl.h>

//JoyStick conection
const int SW_pin = 2; // digital pin connected to switch output
const int X_pin = 0; // analog pin connected to X output    podłączyć do    !!! A0 !!!
const int Y_pin = 1; // analog pin connected to Y output    podłączyć do    !!! A1 !!!

//8x8 Led conection
int DIN = 5;   //DIN port 5
int CS =  4;   //CS port 4
int CLK = 3;   //CLK port 3
LedControl lc = LedControl(DIN,CLK,CS,0); //LED setup


enum Direction {NORTH_EAST, EAST, SOUTH_EAST, SOUTH_WEST, WEST, NORTH_WEST, NONE};  //All possible ball directions
   
//ball struct
struct ballStruct{
  int x;            //position on X axis
  int y;            //position on Y axis
  int velocity;      //speed (number of iterations before changing the position)
  enum Direction dir; //direction
 } ball = {.x = 6, .y = 4, .velocity = 5, .dir = NORTH_WEST};

//global variables
byte paddlePos[8] = {0,0,0,0,0,0,0,0};    //paddle array to display
int paddDotPos = 2;         //Position of paddle main dot (-5 - 8)
//********************************************************************
int rSize = 3;              //length of the paddle (number 1-5)
int points = 5;             //number of points you can lost
//********************************************************************
int xCount = 0;             //Counter of X switched in order to commit acction
int yCount = 0;             //Counter of Y switched in order to commit acction
int switchCount = 0;        //Counter of SW switched in order to commit acction
int ballCount = 0;          //Counter of iterations in order to change slow ball
int bounces = 0;            //ball bounces counter

int tmpPrntCount = 0;       //ONLY FOR TESTING, counter for printing array in monitor

long randNumber;            //Random number to choose position and direction of ball
bool roundOver = false;     //After you lost one of POINTS roundOver is set true (reset ball position)
bool waiting = true;        //after you lose one of POINTS start round only after paddle is moved (up or down)

enum Direction tmpDirection = NONE;        //Variable used to save direction of the ball
bool switchReleased = false; //Is set true if switch is released
bool switchPressed = false;  //Is set true if switch is pressed
bool longPress = false;      //Is set true if switch was pressed for a longer time
bool showingBall = true;     //Is set false if ball shouldn'd be displayed on led
bool gameOverScore = false;  //Is set true if all points are lost and score is displayed on led


//*********************************************************************************
//*******************************INITIALIZATION************************************
//*********************************************************************************

void setup() {
  lc.shutdown(0,false);     //The MAX72XX is in power-saving mode on startup
  lc.setIntensity(0,1);     // Set the brightness
  lc.clearDisplay(0);       // and clear the display
  pinMode(SW_pin, INPUT);
  digitalWrite(SW_pin, HIGH);
  Serial.begin(115200);
  convPaddleToSize();   //initial fill paddle array, cause at start is empty
  randomSeed(analogRead(5));    //initialization for random numbers generator (analog input pin 5 is unconnected)
  resetBall(&ball);           //ball on starting position
}

//*********************************************************************************
//*******************************GAME LOOP*****************************************
//*********************************************************************************

void loop() {
  ++ballCount;            //new iteration increse counter
//  ++tmpPrntCount;       //new iteration increase counter
  checkCounters(&ball);      //check if something is switched, and if there is action to do  
 /* 
  if(tmpPrntCount >= 5){ //ONLY FOR TESTING, display tings in monitor
    tmpPrntCount = 0;     //prints array in monitor every 5 iterations
    Serial.println(rSize);
    printArray(paddlePos); 
  }   //after 5 iterations print paddle array and make counter = 0
*/

if(points <= 0){     //After you lost all POINTS (GAME OVER)
  points = 5;         //reset points
  gameOverScore = true;  //game is over and score will be displayed on led
  Serial.println("GAME OVER");    //for tests
  while(true){
    checkCounters(&ball);         //checking if switch was pressed
    if(switchPressed == false){
      showScore(bounces);         //if switch wasn't pressed show score
    }
    else{
      break;                      //if switch was pressed exit the loop
    }
  }
  bounces = 0;                    //reset score
  gameOverScore = false;          //score is not displayed anymore
}

if(roundOver){        //After you lost one of POINTS
  resetPaddle();
  resetBall(&ball);   //reset ball's position and choose random direction
  
  roundOver = false;
  waiting = true;     //wait for player to move paddle
}

if(!waiting){
  if(ballCount >= ball.velocity){ //ball.velocity - number of iterations to move ball
    ballCount = 0;
    checkCollision(&ball);        //check if ball hit one of edges  
    moveBall(&ball);              //change ball's position
    //Serial.println(bounces);    //for test
  }
}
  
  if(gameOverScore == false){     //not 
    lc.clearDisplay(0);           //clear display
    printByte(paddlePos);         //display paddle on a screen
    if(showingBall == true){
      printBall(&ball);           //display ball on the screen
    }
    delay(1);
  }
}


//*********************************************************************************
//*******************************FUNCTIONS*****************************************
//*********************************************************************************

void printArray(byte arr[]){      
  for(int i = 0; i < 8; ++i){
    Serial.print(arr[i], DEC); 
    Serial.print(", "); 
  }
  Serial.println();
}        //print send array in monitor


void printByte(byte character[]){  
  for(int i = 0; i < 8; ++i){
    lc.setRow(0,i,character[i]);
  }
}       //print send array on screen


void checkCounters(ballStruct *ballPtr){
  int xRead = analogRead(X_pin);              //Read of analog X in
  int yRead = analogRead(Y_pin);              //Read of analog Y in
  int switchRead = digitalRead(SW_pin);       //Read of digital SW in
  
  int xDelay = 20;            //Max counter value
  int yDelay = 5;             //Max counter value
  int sPDelay = 3;            //Max counter value
  int lPDelay = 9;            //Max counter value
  
  if(xRead > 700 && xCount >= 0){             //check if X is switched right, and if there wasnt fast changes between states (>=0)
    ++xCount;                                 //if true add
  }else if(xRead < 300 && xCount <= 0){       //same as above, but for switched left 
    --xCount;                                     
  }else if(xRead >= 300 && xRead <= 700 & xCount != 0){     //if not switched and counter != 0, make 0
    xCount = 0;
  }
  
  if(yRead > 700 && yCount >= 0){             //same as above, but for Y
    ++yCount;
  }else if(yRead < 300 && yCount <= 0){
    --yCount;
  }else if(yRead >= 300 && yRead <= 700 & yCount != 0){
    yCount = 0;
  }
  
  if(switchRead == 0){             //if Switch presed, increase counter
    ++switchCount;
    switchPressed = true;          //Switch is pressed
    switchReleased = false;        //Switch wasn't released yet
  }else if(switchRead == 1 & switchCount != 0){  //if Switch released and counter != 0, make counter = 0
    switchCount = 0;
    switchPressed = false;         //Switch isn't pressed anymore
    switchReleased = true;         //Switch was released
  }
  
  if(xCount >= xDelay){       //if X was switched left for 10 iterations print text and make counter = 0
    xCount = 0;
    if(ball.velocity < 10)
      ball.velocity++;
    Serial.print("Right, v: ");
    Serial.println(ball.velocity);
  }else if(xCount <= -xDelay){      //same as above but for X switched Right
    xCount = 0;
    if(ball.velocity > 0)
      ball.velocity--;
    Serial.print("Left, v: ");
    Serial.println(ball.velocity);
  }
  
  if(yCount >= yDelay){       //if Y was switched down for 10 iterations print text and make counter = 0
    yCount = 0;
    if(paddDotPos < 8){   //if smaller than max value, go down
      ++paddDotPos;
      convPaddleToSize();
    }
    Serial.println("Down");
  }else if(yCount <= -yDelay){      //same as above but for Y switched up
    yCount = 0;
    if(paddDotPos > (-1*rSize)){    //if bigger than min value, go up
      --paddDotPos;
      convPaddleToSize();
    }
    Serial.println("Up");
  }
  
  if(switchCount == sPDelay){   //if Switch was pressed for 3 iterations print "Short"
    Serial.println("Short press");
    waiting = false;
  }else if(switchCount == lPDelay){
    Serial.println("Long press");
    showingBall = false;         //not showing a ball when score is displayed
    tmpDirection = ballPtr->dir; //remembering direction of the ball
    ballPtr->dir = NONE;         //stopping the ball in place
    longPress = true; 
  }else if(switchCount > lPDelay){   //if Switch was pressed for at least 10 iteractions show score
    lc.clearDisplay(0);
    showScore(bounces);          //showing score on led
    if(switchCount != 0 && (switchCount % 10) == 0){
      if(rSize == 5){       //if paddle reach max size make 1 again
        rSize = 1;
        if(paddDotPos < -1)   //if paddle out of screen on the top on index < -1 move the dot 
          paddDotPos = -1;    //to index -1 for easier going out of there
      }else if(rSize < 5 && rSize >= 0){
        ++rSize;
      }   //if Switch was pressed for multiple 10 iterations print "Long" change rSize
      convPaddleToSize();
    }
  }
  if(switchReleased == true && longPress == true){
    Serial.println(switchReleased);
    ballPtr->dir = tmpDirection;//giving the ball direction it had before long press of a switch
    switchReleased = false;
    longPress = false;
    showingBall = true;         //ball can be showed now
  }
}       //check if something is switched, and if there is action to do


void convPaddleToSize(){
  for(int i = 0; i < 8; ++i){     //conversion to display paddle array
    if(i >= paddDotPos && i < (paddDotPos + rSize))
      paddlePos[i] = 1;
    else
      paddlePos[i] = 0;
  }
}       //special function to convert paddle from 1 point to rSize paddle

void printBall(ballStruct *ballPtr){
  lc.setLed(0,ballPtr->y, ballPtr->x, true);  
}       //Display ball on the screen

void moveBall(ballStruct *ballPtr){
  switch(ballPtr->dir)
  {
    case NORTH_EAST:
      ballPtr->x++;
      ballPtr->y--;
      break;
    case EAST:
      ballPtr->x++;
      break;
    case SOUTH_EAST:
      ballPtr->x++;
      ballPtr->y++;
      break;
    case SOUTH_WEST:
      ballPtr->x--;
      ballPtr->y++;
      break;
    case WEST:
      ballPtr->x--;
      break;
    case NORTH_WEST:
      ballPtr->x--;
      ballPtr->y--;
      break;
    case NONE:
      break;
  }
}       //change ball's position

void resetBall(ballStruct *ballPtr)
{
  //reset ball's position:
  ballPtr->x = 0;
  randNumber = random(2);   //each position has 50% chance
  switch(randNumber)  
  {
    case 0:
      ballPtr->y = 3;
      break;
    case 1:
      ballPtr->y = 4;
      break;
  }
  //choose random ball direction:
  randNumber = random(3);   //each direction has 33,3% chance
  switch(randNumber)  
  {
    case 0:
      ballPtr->dir = NORTH_EAST;
      break;
    case 1:
      ballPtr->dir = EAST;
      break;
    case 2:
      ballPtr->dir = SOUTH_EAST;
      break;
  }
}

void resetPaddle(){
  if(rSize <= 2)
    paddDotPos = 3;
  else
    paddDotPos = 2;
  convPaddleToSize();
  //printByte(paddlePos);
  Serial.println("reset");
}

void checkCollision(ballStruct *ballPtr){
  if(ballPtr->x == 0 && ballPtr->y == 0)  //top left corner
  {
    if(ballPtr->dir == WEST)
      ballPtr->dir = EAST;
    else if(ballPtr->dir == NORTH_WEST)
      ballPtr->dir = SOUTH_EAST;
  }
  else if(ballPtr->x == 0 && ballPtr->y == 7) //bottom left corner
  {
    if(ballPtr->dir == WEST)
      ballPtr->dir = EAST;
    else if(ballPtr->dir == SOUTH_WEST)
      ballPtr->dir = NORTH_EAST;
  }
  else if(ballPtr->y == 0)    //top edge
  {
    if(ballPtr->dir == NORTH_EAST)
      ballPtr->dir = SOUTH_EAST;
    else if(ballPtr->dir == NORTH_WEST)
      ballPtr->dir = SOUTH_WEST;
  }
  else if(ballPtr->y == 7)    //bottom edge
  {
    if(ballPtr->dir == SOUTH_EAST)
      ballPtr->dir = NORTH_EAST;
    else if(ballPtr->dir == SOUTH_WEST)
      ballPtr->dir = NORTH_WEST;
  }
  else if(ballPtr->x == 0)    //left edge
  {
    randNumber = random(3);   //each direction has 33,3% chance
    switch(randNumber)  
    {
      case 0:
        ballPtr->dir = NORTH_EAST;
        break;
      case 1:
        ballPtr->dir = EAST;
        break;
      case 2:
        ballPtr->dir = SOUTH_EAST;
        break;
    }
  }
  else if(ballPtr->x == 7)    //right edge
  {
    if(ballPtr->y >= paddDotPos && ballPtr->y < paddDotPos + rSize)    //ball inside the paddle
    { 
      //add bounces:
      bounces++;
      
      //change directory:
      randNumber = random(5);
      switch(randNumber)
      {
        case 0:         //20% chance
          ballPtr->dir = NORTH_WEST;
          break;
        case 1:         //20% chance
          ballPtr->dir = WEST;
          break;
        case 2:         //20% chance
          ballPtr->dir = SOUTH_WEST;
          break;
        default:         //40% chance
          if(ballPtr->dir == NORTH_EAST)
            ballPtr->dir = NORTH_WEST;
          else if(ballPtr->dir == EAST)
            ballPtr->dir = WEST;
          else if(ballPtr->dir == SOUTH_EAST)
            ballPtr->dir = SOUTH_WEST;
      }
    }
    else    //ball outside the paddle
    {
      points--;      //minus one point
      roundOver = true;     //reset ball's position:
    }
  }
}

void writeDecimal(int score){     //showing decimal part of score on the led
  int numberD;
  numberD = score/10;             //taking decimal part of score
  byte zeroDecimal[8] = {0,0,224,160,160,160,224,0};
  byte oneDecimal[8] = {0,0,32,32,32,32,32,0};
  byte twoDecimal[8] = {0,0,224,32,224,128,224,0};
  byte threeDecimal[8] = {0,0,224,32,96,32,224,0};
  byte fourDecimal[8] = {0,0,160,160,224,32,32,0};
  byte fiveDecimal[8] = {0,0,224,128,224,32,224,0};
  byte sixDecimal[8] = {0,0,224,128,224,160,224,0};
  byte sevenDecimal[8] = {0,0,224,32,32,32,32,0};
  byte eightDecimal[8] = {0,0,224,160,224,160,224,0};
  byte nineDecimal[8] = {0,0,224,160,224,32,224,0};   //saving led display (in bytes) of decimal digits to a variables
  switch(numberD)
  {
    case 1:
    printByte(oneDecimal);
    break;
    case 2:
    printByte(twoDecimal);
    break;
    case 3:
    printByte(threeDecimal);
    break;
    case 4:
    printByte(fourDecimal);
    break;
    case 5:
    printByte(fiveDecimal);
    break;
    case 6:
    printByte(sixDecimal);
    break;
    case 7:
    printByte(sevenDecimal);
    break;
    case 8:
    printByte(eightDecimal);
    break;
    case 9:
    printByte(nineDecimal);
    break;
    default:
    printByte(zeroDecimal);
  }                             //showing decimal digit on led
}

void writeUnity(int score){     //showing unity part of score on the led
  int numberU;
  numberU = score%10;           //taking unity part of score
  byte zeroUnity[8] = {0,0,28,20,20,20,28,0};
  byte oneUnity[8] = {0,0,4,4,4,4,4,0};
  byte twoUnity[8] = {0,0,28,4,28,16,28,0};
  byte threeUnity[8] = {0,0,28,4,12,4,28,0};
  byte fourUnity[8] = {0,0,20,20,28,4,4,0};
  byte fiveUnity[8] = {0,0,28,16,28,4,28,0};
  byte sixUnity[8] = {0,0,28,16,28,20,28,0};
  byte sevenUnity[8] = {0,0,28,4,4,4,4,0};
  byte eightUnity[8] = {0,0,28,20,28,20,28,0};
  byte nineUnity[8] = {0,0,28,20,28,4,28,0};   //saving led display (in bytes) of unity digits to a variables
  switch(numberU)
  {
    case 1:
    printByte(oneUnity);
    break;
    case 2:
    printByte(twoUnity);
    break;
    case 3:
    printByte(threeUnity);
    break;
    case 4:
    printByte(fourUnity);
    break;
    case 5:
    printByte(fiveUnity);
    break;
    case 6:
    printByte(sixUnity);
    break;
    case 7:
    printByte(sevenUnity);
    break;
    case 8:
    printByte(eightUnity);
    break;
    case 9:
    printByte(nineUnity);
    break;
    default:
    printByte(zeroUnity);
  }                            //showing unity digit on led
}
  
void showScore(int score){     //showing score on the led
  if(score >= 700){ 
    score = 699;               //points limit
  }
  for(int i = 0; i < score/100; ++i){
    lc.setLed(0,0,i,true);     //displaying hundredth part of score
  }
  writeDecimal(score);         //displaying decimal part of score
  writeUnity(score);           //displaying unity part of score
}