/**********************************************
   CROWNFALL
   Created by Rob Canciello, Nicole Polidore,
   Jacob Surovsky, and Connor Wolf
 *********************************************/
enum gameStates {SETUP, KING, SOLDIER, WIZARD, CLERIC, JESTER, QUEEN, PLAY};
byte gameState = SETUP;
byte playRole = SETUP;
byte kingHealth = 3;

enum Teams {RED_TEAM, BLUE_TEAM, BLUE_TRANSITION};
byte team = RED_TEAM;

bool faceConnections[6];
Timer connectedTimer;

enum shieldStates {EXHAUSTED, PROTECTING, NOT_PROTECTED, PROTECTED};
byte shieldState = NOT_PROTECTED;
byte divineIntervention  = 2;

Color teamColor = RED;

void setup() {
}

void loop() {
  // put your main code here, to run repeatedly:
  switch (gameState) {
    case SETUP:
      setupLoop();
      break;
    case PLAY:
      playLoop();
      break;
    default:
      assignLoop();
      break;
  }

  CheckNeighbors();
  displayLoop();

  setValueSentOnAllFaces(EncodeSignal());
}

byte EncodeSignal() {
  if (gameState != PLAY)
    return (gameState << 3) + (team << 1);
  else
    return (gameState << 3) + (team << 2) + shieldState;
}

byte GetGameState(byte data) {
  return (data >> 3);
}

byte GetTeamState(byte data) {
  if (gameState != PLAY)
    return ((data >> 1) & 3);
  else
    return ((data >> 2) & 1);
}

byte GetShieldState(byte data) {
  return (data & 3);
}

void setupLoop() {
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

  //get triple clicked, become king
  if (buttonMultiClicked() && buttonClickCount() == 3) {
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

  //find a neighbor in assignment, go to that
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//neighbor!
      byte neighborGameState = GetGameState(getLastValueReceivedOnFace(f));
      if (neighborGameState != SETUP && neighborGameState != PLAY) {//this neighbor is telling me to go into assignment
        //I get a temporary gamestate here that is roughly accurate, but it's refined later
        if (neighborGameState == QUEEN) {
          gameState = QUEEN;
        } else {
          gameState = neighborGameState + 1;
        }
      }
    }
  }

  shieldState = NOT_PROTECTED;
}

void assignLoop() {
  //this is a few frames where I can refine my position

  bool neighborsInSetup = false;
  byte lowestNeighbor = gameState - 1;//this assumes that our triggering neighbor was the lowest possible

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//neighbor!
      byte neighborGameState = GetGameState(getLastValueReceivedOnFace(f));

      if (neighborGameState == SETUP) {//is this neighbor in setup?
        neighborsInSetup = true;
      } else if (neighborGameState != PLAY) {//is this neighbor in assignment?

        //is this neighbor lower than my lowest neighbor?
        if (neighborGameState < lowestNeighbor) {
          lowestNeighbor = neighborGameState;
        }
      }
    }
  }//end face loop

  if (lowestNeighbor != 10) {//so I did have a low neighbor
    gameState = min(lowestNeighbor + 1, QUEEN);
  }

  if (neighborsInSetup == false) {
    playRole = gameState;
    gameState = PLAY;
  }

  if (playRole == CLERIC) shieldState = NOT_PROTECTED;
}

void playLoop() {
  if (buttonSingleClicked())
  {
    if (shieldState == PROTECTED) shieldState = NOT_PROTECTED;
    else if (playRole == KING) kingHealth--;
  }

  if (buttonDoubleClicked())
  {
    if (playRole == KING) kingHealth = 3;
  }

  //get triple clicked, go back to setup
  if (buttonMultiClicked() && buttonClickCount() == 3) {
    gameState = SETUP;
  }

  //find a neighbor in setup, go to that
  FOREACH_FACE(f) {

    if (!isValueReceivedOnFaceExpired(f)) {//neighbor!
      byte neighborGameState = GetGameState(getLastValueReceivedOnFace(f));
      if (neighborGameState == SETUP) {//this neighbor is telling me to go into setup
        gameState = SETUP;
      }

    }
  }

  if (isAlone()) {
    if (playRole == CLERIC) {
      shieldState = (divineIntervention > 0) ? PROTECTING : EXHAUSTED;
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

          if (GetGameState(getLastValueReceivedOnFace(face)) == PLAY) {
            if (playRole == CLERIC) {
              if (shieldState == PROTECTING) {
                if (GetShieldState(getLastValueReceivedOnFace(face)) == NOT_PROTECTED) {
                  divineIntervention--;
                  shieldState = EXHAUSTED;
                }
              }
            } else
            {
              if (GetShieldState(getLastValueReceivedOnFace(face)) == PROTECTING) {
                if (shieldState == NOT_PROTECTED) {
                  shieldState = PROTECTED;
                }
              }
            }
          }

          connectedTimer.set(500);
        }
      } else
        faceConnections[face] = false;
    }
  }
}

void displayLoop() {
  teamColor = (team) ? RED : BLUE;

  switch (gameState) {
    case SETUP:
      setColor(dim(teamColor, 128));
      break;
    case PLAY:
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
    case KING:
      setColorOnFace(teamColor, 0);
      break;
    case SOLDIER:
      setColorOnFace(teamColor, 0);
      setColorOnFace(teamColor, 1);
      break;
    case WIZARD:
      setColorOnFace(teamColor, 0);
      setColorOnFace(teamColor, 1);
      setColorOnFace(teamColor, 2);
      break;
    case CLERIC:
      setColor(teamColor);
      setColorOnFace(OFF, 4);
      setColorOnFace(OFF, 5);
      if (divineIntervention > 0) divineShield();
      break;
    case JESTER:
      setColor(teamColor);
      setColorOnFace(OFF, 5);
      break;
    case QUEEN:
      setColor(teamColor);
      break;
  }

  if (shieldState == PROTECTED) divineShield();
}

void divineShield() {
  FOREACH_FACE(face) {
    byte faceBrightness = sin8_C((millis() + ((face + 1) * 1000)) / 4);
    if (faceBrightness > 200) {
      setColorOnFace(dim(YELLOW, faceBrightness / 2), face);
    }
  }
}
