/**********************************************
   CROWNFALL
   Created by Rob Canciello, Nicole Polidore, Jacob Surovsky, and Connor Wolf
 *********************************************/
#define RED_COLOR makeColorHSB(15, 255, 255)
#define BLUE_COLOR makeColorHSB(115, 255, 255)
#define CROWN_YELLOW makeColorHSB(50, 200, 255)
#define ALCHEMISTGREEN (makeColorHSB(70, 255, 255))

#define SPIN_DURATION 120 //Wizard Animation
#define KNIGHTTIME 1000 //Knight Animation
#define QUEENTIME 750 //Queen Animation
#define ALCHEMISTTIME 150 //Alchemist Animation
#define SPINLENGTH 300 //Jester Animation
Timer animationTimer;

byte bombDirection = 1; // Alchemist Animation

byte bombFace = 1; // Alchemist Animation
byte spinFace; // Alchemist Animation

enum GameState
{
  SETUP, //Team selection and King declaration
  PLAY, //Gameplay
};

enum Roles
{
  RESET, //Role for properly resetting the game.
  PAWN, //Role for Blinks in setup mode.
  KING, //3 Knock-offs and you're out.
  KNIGHT, //Defend a buddy on your back.
  WIZARD, //Teleport around the arena.
  JESTER, //BANZAI!
  ALCHEMIST, //Passes bombs.
  QUEEN, //An immovable object.
};

enum Teams
{
  RED_TEAM,
  BLUE_TEAM,
  BLUE_TRANSITION //Team for switching to blue safely.
};

byte health = 3;
byte role = PAWN;
byte team = RED_TEAM;
byte gameState =  SETUP;
Color teamColor = RED_COLOR;

byte EncodeSignal()
{
  return (role << 3) + (team << 1);
}

byte GetRole(byte input)
{
  return ((input >> 3) & 7);
}

byte GetTeam(byte input)
{
  return ((input >> 1) & 3);
}

void Reset()
{
  health = 3;

  role = RESET;
  gameState = SETUP;
}

void setup()
{
  Reset();
}

void loop()
{
  setValueSentOnAllFaces(EncodeSignal());

  switch (gameState)
  {
    case SETUP:
      SetupLoop();
      break;

    case PLAY:
      PlayLoop();
      break;
  }

  DisplayLoop();

  buttonSingleClicked();
  buttonDoubleClicked();
  buttonMultiClicked();
  buttonLongPressed();


}

void SetupLoop()
{
  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    RESETIING
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
  if (role == RESET)
  {
    role = PAWN;
    FOREACH_FACE(face)
    {
      if (!isValueReceivedOnFaceExpired(face))
      {
        if (GetRole(getLastValueReceivedOnFace(face)) >= 2) //If something around me isn't in reset or setup.
        {
          role = RESET;
        }
      }
    }
    return;
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
          if (GetTeam(getLastValueReceivedOnFace(face)) == BLUE_TRANSITION)
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
          if (GetTeam(getLastValueReceivedOnFace(face)) == RED_TEAM)
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
          if (GetTeam(getLastValueReceivedOnFace(face)) == RED_TEAM)
          {
            team = BLUE_TRANSITION;
          }
        }
      }
      break;
  }

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    ROLE ASSIGNMENT
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
  FOREACH_FACE(face)
  {
    if (!isValueReceivedOnFaceExpired(face))
    {
      if (GetRole(getLastValueReceivedOnFace(face)) >= 2) // NOT RESET OR PAWN. RESET is 0, PAWN is 1.
      {
        role = GetRole(getLastValueReceivedOnFace(face)) + 1;
        gameState = PLAY;
        return;
      }
    }
  }

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    PLAYER INPUT
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
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

  if (buttonMultiClicked() && buttonClickCount() == 3)
  {
    role = KING;
    gameState = PLAY;
    return;
  }
}

void PlayLoop()
{
  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    RESET CHECKING
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
  FOREACH_FACE(face)
  {
    if (!isValueReceivedOnFaceExpired(face))
    {
      if (GetRole(getLastValueReceivedOnFace(face)) == RESET) //Neighbor is in reset mode.
      {
        Reset();
        return;
      }
    }
  }

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    PLAYER INPUT
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
  if (buttonMultiClicked() && buttonClickCount() == 3)
  {
    Reset();
  }

  
  if (buttonSingleClicked()) {
    health = max(health - 1, 0);
  }

  if(buttonDoubleClicked()){
    health = 3;
    }
}

void DisplayLoop()
{
  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    TEAM COLOR ASSIGNMENT
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
  switch (team)
  {
    case RED_TEAM:
      teamColor = RED_COLOR;
      break;

    case BLUE_TEAM:
      teamColor = BLUE_COLOR;
      break;

    case BLUE_TRANSITION:
      teamColor = CROWN_YELLOW;
      break;
  }

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    ROLE ANIMATIONS
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
  switch (role)
  {
    case RESET:
      setColor(MAGENTA);
      break;

    case PAWN:
      setColor(teamColor);
      break;

    case KING:
      KingAnimation();
      break;

    case KNIGHT:
      KnightAnimation();
      break;

    case WIZARD:
      WizardAnimation();
      break;

    case JESTER:
      JesterAnimation();
      break;

    case ALCHEMIST:
      AlchemistAnimation();
      break;

    case QUEEN:
      QueenAnimation();
      break;
  }
}

void KingAnimation()
{
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

      break;
  }

  byte bgBrightness = map(sin8_C(map(millis() % PULSE_PERIOD, 0, PULSE_PERIOD, 0, 255)), 0, 255, darkestPoint, 255);
  byte crownBrightness = map(sin8_C(map(PULSE_PERIOD - (millis() % PULSE_PERIOD), 0, PULSE_PERIOD, 0, 255)), 0, 255, darkestPoint, 255);
  setColor(dim(teamColor, bgBrightness));

  setColorOnFace(dim(score1, crownBrightness), 1);
  setColorOnFace(dim(score2, crownBrightness), 3);
  setColorOnFace(dim(score3, crownBrightness), 5);
}

void KnightAnimation()
{
  byte lFootFront = map(sin8_C(map(millis() % KNIGHTTIME, 0, KNIGHTTIME, 0, 255)), 0, 255, 50, 255);
  byte lFootBack = map(sin8_C(map((millis() - (KNIGHTTIME / 6)) % KNIGHTTIME, 0, KNIGHTTIME, 0, 255)), 0, 255, 50, 255);
  byte rFootFront = map(sin8_C(map(KNIGHTTIME - (millis() % KNIGHTTIME), 0, KNIGHTTIME, 0, 255)), 0, 255, 50, 255);
  byte rFootBack = map(sin8_C(map(KNIGHTTIME - ((millis() - (KNIGHTTIME / 6)) % KNIGHTTIME), 0, KNIGHTTIME, 0, 255)), 0, 255, 50, 255);


  //now we can paint all the faces to flash on and off with the appropriate timers

  setColorOnFace(dim(teamColor, 0), 0);
  setColorOnFace(dim(teamColor, 0), 3);

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

  setColorOnFace(dim(teamColor, 50), 0);
  setColorOnFace(dim(teamColor, 50), 3);

  setColorOnFace(dim(teamColor, lFootFront2), 1);
  setColorOnFace(dim(teamColor, lFootBack2), 2);

  setColorOnFace(dim(teamColor, rFootBack2), 4);
  setColorOnFace(dim(teamColor, rFootFront2), 5);

}

void WizardAnimation()
{
  byte teamHue;

  //make sure to alter these if the team colors change

  if (team == RED_TEAM) {
    teamHue = 15;
  }
  if (team == BLUE_TEAM) {
    teamHue = 115;
  }

  byte spiralOffset = (millis() / SPIN_DURATION) % 6;

  FOREACH_FACE(f) {

    setColorOnFace(makeColorHSB(teamHue, (50 * f) + 50, (40 * f) + 50), (f + spiralOffset) % 6);

  }
}

void JesterAnimation()
{
  setColor(teamColor);

  byte hue1 = 220;

  if (animationTimer.isExpired()) {
    spinFace = (spinFace + 5) % 6;
    animationTimer.set(SPINLENGTH);
  }

  setColorOnFace(makeColorHSB(hue1, 255, 255), spinFace);
  setColorOnFace(makeColorHSB(hue1, 255, 255), (spinFace + 3) % 6);
}

void AlchemistAnimation()
{
  if (!animationTimer.isExpired()) {
    //if the timer is NOT expired, paint the stated face like a bomb
    setColor(teamColor);
    setColorOnFace(ALCHEMISTGREEN, bombFace);
    setColorOnFace(dim(teamColor, 180), (bombFace + 1) % 6);
    setColorOnFace(dim(teamColor, 180), (bombFace + 5) % 6);

  } else if (animationTimer.isExpired()) {
    //but if the timer IS expired, then change the bombFace by one in the direction the bomb is currently cycling

    bombFace = bombFace + (1 * bombDirection);

    if (bombFace == 5) {
      //when we hit Face 3, send the bomb back to the left
      bombDirection = -1;
    }

    if (bombFace == 0) {
      //when we hit Face 1, send the bomb back to the right
      bombDirection = 1;
    }

    //whenever the timer expires, reset it
    animationTimer.set(ALCHEMISTTIME);
  }
}

void QueenAnimation()
{
  byte currentBrightness = map(sin8_C(map(millis() % QUEENTIME, 0, QUEENTIME, 0, 255)), 0, 255, 150, 255);
  byte altBrightness = map(sin8_C(map(QUEENTIME - (millis() % QUEENTIME), 0, QUEENTIME, 0, 255)), 0, 255, 150, 255);

  setColorOnFace(makeColorHSB(0, 0, currentBrightness), 2);
  setColorOnFace(makeColorHSB(0, 0, altBrightness), 3);
  setColorOnFace(makeColorHSB(0, 0, currentBrightness), 4);
}
/**/
