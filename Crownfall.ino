/**********************************************
   CROWNFALL
   Created by Rob Canciello, Nicole Polidore,
   Jacob Surovsky, and Connor Wolf
 *********************************************/
enum gameStates {SETUP, KING, SOLDIER, WIZARD, CLERIC, JESTER, QUEEN, RED_PLAY, BLUE_PLAY};
byte gameState = SETUP;
byte playRole = SETUP;
byte kingHealth = 3;

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
      if (neighborGameState != SETUP && neighborGameState != RED_PLAY && neighborGameState != BLUE_PLAY) {//this neighbor is telling me to go into assignment
        //I get a temporary gamestate here that is roughly accurate, but it's refined later
        if (neighborGameState == QUEEN) {
          gameState = QUEEN;
        } else {
          gameState = neighborGameState + 1;
        }
      }
    }
  }

  blessState = NOT_BLESSED;
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
      } else if (neighborGameState != RED_PLAY && neighborGameState != BLUE_PLAY) {//is this neighbor in assignment?

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
    if (team == RED_TEAM) gameState = RED_PLAY;
    else gameState = BLUE_PLAY;
  }

  if (playRole == CLERIC) blessState = EXHAUSTED;
}

void playLoop() {
  if (playRole == CLERIC) {
    ClericPlayLoop();
  }

  //get triple clicked, go back to setup
  if (buttonMultiClicked() && buttonClickCount() == 3) {
    gameState = SETUP;
  }

  if (buttonSingleClicked())
  {
    if (playRole == KING) kingHealth--;
    else if (blessState == BLESSED) blessState = NOT_BLESSED;
  }

  if (buttonDoubleClicked())
  {
    if (playRole == KING) kingHealth = 3;
  }

  //find a neighbor in setup, go to that
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//neighbor!
      byte neighborGameState = GetGameState(getLastValueReceivedOnFace(f));
      if (neighborGameState == SETUP) {//this neighbor is telling me to go into setup
        gameState = SETUP;
      }

      if (GetBlessing(getLastValueReceivedOnFace(f)) == BLESSING) {
        blessState = BLESSED;
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
  teamColor = (team) ? RED : BLUE;

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
      if (blessState == BLESSING) divineShield();
      break;
    case JESTER:
      setColor(teamColor);
      setColorOnFace(OFF, 5);
      break;
    case QUEEN:
      setColor(teamColor);
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
