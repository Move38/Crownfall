/**********************************************
   CROWNFALL
   Created by Rob Canciello, Nicole Polidore,
   Jacob Surovsky, and Connor Wolf
 *********************************************/
#define RED_COLOR makeColorHSB(15, 255, 255)
#define BLUE_COLOR makeColorHSB(115, 255, 255)
#define CROWN_YELLOW makeColorHSB(50, 200, 255)
#define ALCHEMISTGREEN (makeColorHSB(65, 255, 255))
enum gameStates {SETUP, KING, SOLDIER, ASSASSIN, JESTER, WIZARD, CLERIC, NECROMANCER, GIANT, CAVALRY, RED_PLAY, BLUE_PLAY};
byte gameState = SETUP;
byte playRole = SETUP;
byte health = 3;
Timer animationTimer;

enum Teams {RED_TEAM, BLUE_TEAM, BLUE_TRANSITION};
byte team = RED_TEAM;

bool faceConnections[6];
Timer connectedTimer;

enum blessStates {EXHAUSTED, BLESSING, NOT_BLESSED, BLESSED};
byte blessState = NOT_BLESSED;
byte blessings = 2;

bool blessedNeighbors[6];

Color teamColor = RED;

void setup() {
}

void loop() {
  // put your main code here, to run repeatedly:
  switch (gameState) {
    case SETUP:
      setupLoop();
      break;
    case RED_PLAY:
    case BLUE_PLAY:
      playLoop();
      break;
    default:
      assignLoop();
      break;
  }

  CheckNeighbors();
  displayLoop();

  buttonSingleClicked();
  buttonDoubleClicked();
  buttonMultiClicked();
  buttonLongPressed();

  setValueSentOnAllFaces(EncodeSignal());
}

byte EncodeSignal() {
  if (gameState != RED_PLAY && gameState != BLUE_PLAY)
    return (gameState << 2) + (team);
  else
    return (gameState << 2) + blessState;
}

byte GetGameState(byte data) {
  return (data >> 2);
}

byte GetTeamState(byte data) {
  if (gameState != RED_PLAY && gameState != BLUE_PLAY)
    return (data & 3);
  else if (gameState == RED_PLAY) return RED_TEAM;
  else return BLUE_TEAM;
}

byte GetBlessing(byte data) {
  if (gameState == RED_PLAY || gameState == BLUE_PLAY)
    return (data & 3);
  else return 0;
}

void setupLoop() {
  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     PLAYER INPUT - SETUP
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
  if (buttonMultiClicked() && buttonClickCount() == 3)
  {
    gameState = KING;
  }

  if (buttonSingleClicked())
  {
    if (team == RED_TEAM)
    {
      team = BLUE_TRANSITION;
    } else
    {
      team = RED_TEAM;
    }
  }

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    TEAM ASSIGNMENT
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
  switch (team)
  {
    case RED_TEAM:
      FOREACH_FACE(face)
      {
        if (!isValueReceivedOnFaceExpired(face))
        {
          if (GetTeamState(getLastValueReceivedOnFace(face)) == BLUE_TRANSITION)
          {
            team = BLUE_TRANSITION;
          }
        }
      }
      break;

    case BLUE_TEAM:
      FOREACH_FACE(face)
      {
        if (!isValueReceivedOnFaceExpired(face))
        {
          if (GetTeamState(getLastValueReceivedOnFace(face)) == RED_TEAM)
          {
            team = RED_TEAM;
          }
        }
      }
      break;

    case BLUE_TRANSITION:
      team = BLUE_TEAM;
      FOREACH_FACE(face)
      {
        if (!isValueReceivedOnFaceExpired(face))
        {
          if (GetTeamState(getLastValueReceivedOnFace(face)) == RED_TEAM)
          {
            team = BLUE_TRANSITION;
          }
        }
      }
      break;
  }


  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     ASSIGNMENT CHECKING
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
  FOREACH_FACE(f)
  {
    if (!isValueReceivedOnFaceExpired(f)) {//neighbor!
      byte neighborGameState = GetGameState(getLastValueReceivedOnFace(f));
      if (neighborGameState != SETUP && neighborGameState != RED_PLAY && neighborGameState != BLUE_PLAY) {//this neighbor is telling me to go into assignment
        //I get a temporary gamestate here that is roughly accurate, but it's refined later
        if (neighborGameState == CAVALRY) {
          gameState = CAVALRY;
        } else {
          gameState = neighborGameState + 1;
        }
      }
    }
  }

  blessState = NOT_BLESSED;
}

void assignLoop() {
  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    NEIGHBOR CHECKING -> ROLE ASSIGNMENT
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
  bool neighborsInSetup = false;
  byte lowestNeighbor = gameState - 1;//this assumes that our triggering neighbor was the lowest possible

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//neighbor!
      byte neighborGameState = GetGameState(getLastValueReceivedOnFace(f));

      if (neighborGameState == SETUP) {//is this neighbor in setup?
        neighborsInSetup = true;
      } else if (neighborGameState != RED_PLAY && neighborGameState != BLUE_PLAY) {//is this neighbor in assignment?

        //is this neighbor lower than my lowest neighbor?
        if (neighborGameState < lowestNeighbor) {
          lowestNeighbor = neighborGameState;
        }
      }
    }
  }//end face loop

  if (lowestNeighbor != 10) {//so I did have a low neighbor
    gameState = min(lowestNeighbor + 1, CAVALRY);
  }

  if (neighborsInSetup == false) {
    playRole = gameState;
    if (team == RED_TEAM) gameState = RED_PLAY;
    else gameState = BLUE_PLAY;
  }

  //~~SET CLERIC TO EXHAUSTED BY DEFAULT~~
  if (playRole == CLERIC) blessState = EXHAUSTED;
}

void playLoop() {
  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     PLAYER INPUT - PLAY
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
  //get triple clicked, go back to setup
  if (buttonMultiClicked() && buttonClickCount() == 3) {
    gameState = SETUP;
  }

  if (buttonSingleClicked())
  {
    if (playRole == KING) health--;
    else if (blessState == BLESSED) blessState = NOT_BLESSED;
  }

  if (buttonDoubleClicked())
  {
    if (playRole == KING) health = 3;
  }

  //~~CLERIC SPECIFIC INTERACTIONS~~
  if (playRole == CLERIC) {
    ClericPlayLoop();
  }

  //find a neighbor in setup, go to that
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//neighbor!
      byte neighborGameState = GetGameState(getLastValueReceivedOnFace(f));
      if (neighborGameState == SETUP) {//this neighbor is telling me to go into setup
        gameState = SETUP;
      }

      if (neighborGameState == RED_PLAY || neighborGameState == BLUE_PLAY) {
        if (GetBlessing(getLastValueReceivedOnFace(f)) == BLESSING) {
          blessState = BLESSED;
        }
      }
    }
  }
}

void ClericPlayLoop() {
  if (isAlone())
  {
    blessState = BLESSING;
  }

  for (byte face = 0; face != 6; face++) {
    if (faceConnections[face] == true) { //I am connected to a neighbor on face X.
      if (GetBlessing(getLastValueReceivedOnFace(face)) == BLESSED) //neighbor has been blessed.
      {
        if (blessedNeighbors[face] != true) //we didn't know he was blessed.
        {
          blessings--;
          blessedNeighbors[face] = true;
        }
      } else if (GetBlessing(getLastValueReceivedOnFace(face)) == NOT_BLESSED) { //neighbor at Face X is not blessed.
        blessedNeighbors[face] = false;
      }
    } else
    {
      blessedNeighbors[face] = true; //By default, I assume everyone is blessed!
    }
  }
}

void CheckNeighbors()
{
  if (connectedTimer.isExpired()) {
    FOREACH_FACE(face) {
      if (!isValueReceivedOnFaceExpired(face)) {//neighbor
        if (faceConnections[face] == false) {
          faceConnections[face] = true;
          connectedTimer.set(500);
        }
      } else
        faceConnections[face] = false;
    }
  }
}

void displayLoop() {
  teamColor = (team) ? RED_COLOR : BLUE_COLOR;

  switch (gameState) {
    case SETUP:
      setColor(dim(teamColor, 128));
      break;
    case RED_PLAY:
    case BLUE_PLAY:
      roleDisplay();
      break;
    default:
      setColor(teamColor);
      break;
  }

  if (!connectedTimer.isExpired())
  {
    //Connected Animation
    for (byte i = 0; i != 6; i++) {
      if (faceConnections[i]) {
        setColorOnFace(dim(WHITE, sin8_C(millis())), i);
      }
    }
  }
}

void roleDisplay() {
  setColor(OFF);
  switch (playRole) {
    case SETUP:
      Pawn();
      break;
    case KING:
      Crown();
      break;
    case SOLDIER:
      Knight();
      break;
    case WIZARD:
      Wizard();
      break;
    case JESTER:
      Jester();
      break;
    case CLERIC:
      setColor(teamColor);
      setColorOnFace(OFF, 5);
      if (blessState == BLESSING) divineShield();
      break;
    case GIANT:
      Giant();
      break;
    case CAVALRY:
      Cavalry();
      break;
    case NECROMANCER:
      Necromancer();
      break;
    case ASSASSIN:
      Assassin();
      break;

  }

  if (blessState == BLESSED) divineShield();
}

void divineShield() {
  FOREACH_FACE(face) {
    byte faceBrightness = sin8_C((millis() + ((face + 1) * 1000)) / 4);
    if (faceBrightness > 200) {
      setColorOnFace(dim(YELLOW, faceBrightness / 2), face);
    }
  }
}

void Pawn() {
  int PULSE_PERIOD = 1500;

  byte bgBrightness = map(sin8_C(map(millis() % PULSE_PERIOD, 0, PULSE_PERIOD, 0, 255)), 0, 255, 155, 255);

  setColor(dim(teamColor, bgBrightness));

  setColorOnFace(dim(teamColor, (255 - bgBrightness) + 100), 1);
  setColorOnFace(dim(teamColor, (255 - bgBrightness) + 100), 3);
  setColorOnFace(dim(teamColor, (255 - bgBrightness) + 100), 5);
}

void Crown() {
  byte darkestPoint;
  Color score1;
  Color score2;
  Color score3;
  int PULSE_PERIOD;

  switch (health)
  {
    case 3:
      PULSE_PERIOD = 1500;
      score1 = CROWN_YELLOW;
      score2 = CROWN_YELLOW;
      score3 = CROWN_YELLOW;
      darkestPoint = 200;
      break;

    case 2:
      PULSE_PERIOD = 1000;
      score1 = OFF;
      score2 = CROWN_YELLOW;
      score3 = CROWN_YELLOW;
      darkestPoint = 165;
      break;

    case 1:
      PULSE_PERIOD = 600;
      score1 = OFF;
      score2 = OFF;
      score3 = CROWN_YELLOW;
      darkestPoint = 125;
      break;

    case 0:
      score1 = OFF;
      score2 = OFF;
      score3 = OFF;
      darkestPoint = 90;
      break;
  }

  byte bgBrightness = map(sin8_C(map(millis() % PULSE_PERIOD, 0, PULSE_PERIOD, 0, 255)), 0, 255, darkestPoint, 255);
  byte crownBrightness = map(sin8_C(map(PULSE_PERIOD - (millis() % PULSE_PERIOD), 0, PULSE_PERIOD, 0, 255)), 0, 255, darkestPoint, 255);

  setColor(dim(teamColor, bgBrightness));

  setColorOnFace(dim(score1, crownBrightness), 1);
  setColorOnFace(dim(score2, crownBrightness), 3);
  setColorOnFace(dim(score3, crownBrightness), 5);
}

#define KNIGHT_TIME 75
byte swordFace;
byte swordDirection;


void Knight() {

  if (animationTimer.isExpired()) {
    swordFace = swordFace + (1 * swordDirection);

    if (swordFace > 0 && swordFace < 4) {
      animationTimer.set(KNIGHT_TIME);
    } else {
      animationTimer.set(KNIGHT_TIME * 8);
    }
  }

  if (swordFace == 0) {
    swordDirection = 1;
  }
  if (swordFace == 4) {
    swordDirection = -1;
  }


  setColor(dim(teamColor, 100));
  setColorOnFace(WHITE, swordFace);
  setColorOnFace(teamColor, 5);
  setColorOnFace(teamColor, 0);
  setColorOnFace(teamColor, 4);

}

#define WIZARD_TIME 1000

void Wizard() {

  byte spell = map(sin8_C(map(millis() % WIZARD_TIME, 0, WIZARD_TIME, 0, 255)), 0, 255, 0, 255);

  setColorOnFace(dim(teamColor, 255 - spell), 1);
  setColorOnFace(dim(teamColor, 255 - spell), 5);
  setColorOnFace(dim(teamColor, (90 - spell) % 255), 2);
  setColorOnFace(dim(teamColor, (90 - spell) % 255), 4);
  setColorOnFace(dim(teamColor, spell), 3);
  setColorOnFace(BLUE, 0);
}

byte ballFace = 0;
byte ballDirection = 1;
#define JESTER_TIME 100
Color ballColor;

void Jester() {

  if (animationTimer.isExpired()) {
    ballFace = ballFace + (1 * ballDirection);

    animationTimer.set(JESTER_TIME);
  }

  if (ballFace == 0) {
    ballDirection = 1;
    ballColor = CROWN_YELLOW;
  }
  if (ballFace == 4) {
    ballDirection = -1;
    ballColor = MAGENTA;
  }

  setColorOnFace(ballColor, ballFace);
  setColorOnFace(ALCHEMISTGREEN, 5);
}

#define CLERIC_TIME 150
byte healingFace;

void Cleric() {

  if (animationTimer.isExpired()) {
    healingFace = (healingFace + 1) % 6;

    animationTimer.set(CLERIC_TIME);
  }
  

  setColor(OFF);

  setColorOnFace(dim(teamColor, 200), (healingFace + 1) % 6);
  setColorOnFace(dim(teamColor, 150), (healingFace + 2) % 6);
  setColorOnFace(dim(teamColor, 200), (healingFace + 4) % 6);
  setColorOnFace(dim(teamColor, 150), (healingFace + 5) % 6);

  switch (blessings) {
    case 2:

      setColorOnFace(WHITE, healingFace);
      setColorOnFace(WHITE, (healingFace + 3) % 6);

      break;

    case 1:
      setColorOnFace(WHITE, healingFace);
      break;

    case 0:
      //do nothing!
      break;
  }
}

void Giant() {

#define STOMP_TIME 1500

  byte stomp = (map(millis() % STOMP_TIME, 0, STOMP_TIME, 0, 255));
  stomp = 255 - stomp;

  //drawing a footstep that stomps
  setColorOnFace(dim(teamColor, stomp), 1);
  setColorOnFace(dim(teamColor, stomp), 2);
  setColorOnFace(dim(teamColor, stomp), 3);
  setColorOnFace(dim(teamColor, stomp), 5);

  if (stomp > 240) {
    setColor(WHITE);
    setColorOnFace(teamColor, 4);
    setColorOnFace(teamColor, 0);
  } //might want to put this code below the footstep code
  if (stomp < 240 && stomp > 230) {
    setColor(teamColor);
    setColorOnFace(dim(MAGENTA, stomp), 1);
    setColorOnFace(dim(MAGENTA, stomp), 2);
    setColorOnFace(dim(MAGENTA, stomp), 3);
    setColorOnFace(dim(MAGENTA, stomp), 5);
  }
}

#define CAVALRY_TIME 600

void Cavalry() {

  byte lFootFront = map(sin8_C(map(millis() % CAVALRY_TIME, 0, CAVALRY_TIME, 0, 255)), 0, 255, 50, 255);
  byte lFootBack = map(sin8_C(map((millis() - (CAVALRY_TIME / 6)) % CAVALRY_TIME, 0, CAVALRY_TIME, 0, 255)), 0, 255, 50, 255);
  //  byte rFootFront = map(sin8_C(map(CAVALRY_TIME - (millis() % CAVALRY_TIME), 0, CAVALRY_TIME, 0, 255)), 0, 255, 50, 255);
  //  byte rFootBack = map(sin8_C(map(CAVALRY_TIME - ((millis() - (CAVALRY_TIME / 6)) % CAVALRY_TIME), 0, CAVALRY_TIME, 0, 255)), 0, 255, 50, 255);

  byte rFootFront = (255 + 50) - lFootFront;
  byte rFootBack = (255 + 50) - lFootBack;

  //now we can paint all the faces to flash on and off with the appropriate timers

  byte stepDelay = 100;

  byte lFootFront2 = lFootFront + stepDelay;
  if (lFootFront2 > 255) {
    lFootFront2 = 255;
  }

  byte lFootBack2 = lFootBack + stepDelay;
  if (lFootBack2 > 255) {
    lFootBack2 = 255;
  }

  byte rFootFront2 = rFootFront + stepDelay;
  if (rFootFront2 > 255) {
    rFootFront2 = 255;
  }

  byte rFootBack2 = rFootBack + stepDelay;
  if (rFootBack2 > 255) {
    rFootBack2 = 255;
  }

  setColor(dim(teamColor, 50));

  //  setColorOnFace(dim(teamColor, 50), 0);
  //  setColorOnFace(dim(teamColor, 50), 3);

  setColorOnFace(dim(teamColor, lFootFront2), 1);
  setColorOnFace(dim(teamColor, lFootBack2), 2);

  setColorOnFace(dim(teamColor, rFootBack2), 4);
  setColorOnFace(dim(teamColor, rFootFront2), 5);
}

#define NECROMANCER_TIME 800
byte greenFace1;
byte greenFace2;

void Necromancer() {

  byte Brightness = map(sin8_C(map((NECROMANCER_TIME - animationTimer.getRemaining()), 0, NECROMANCER_TIME, 0, 255)), 0, 255, 100, 255);

  if (animationTimer.isExpired()) {

    greenFace1 = random(5);
    greenFace2 = (greenFace1 + random(4) + 1) % 6;

    animationTimer.set(NECROMANCER_TIME);
  }

  byte bgBrightness = map(sin8_C(map(millis() % (NECROMANCER_TIME * 2), 0, (NECROMANCER_TIME * 2), 0, 255)), 0, 255, 155, 255);

  setColor(dim(teamColor, bgBrightness));

  setColorOnFace(dim(ALCHEMISTGREEN, Brightness), greenFace1);
  setColorOnFace(dim(ALCHEMISTGREEN, Brightness), greenFace2);
}

#define ASSASSIN_TIME 1000
bool darkness;
byte brightness;

void Assassin() {

  if (animationTimer.isExpired()) {

    if (darkness == true) {
      darkness = false;
    } else if (darkness == false) {
      darkness = true;
    }

    animationTimer.set(ASSASSIN_TIME);
  } else {

    if (darkness) {
      setColor(dim(teamColor, 100));
    } else {
      brightness = map(sin8_C(map(animationTimer.getRemaining(), 0, ASSASSIN_TIME, 0, 255)), 0, 255, 100, 255);
      FOREACH_FACE(f) {
        setColorOnFace(dim(teamColor, (brightness * ((f % 2) + 1)) % 256), f);
      }
    }
  }
}
