

//      ___________ __     ___    ____  ______
//     / ____<  / // /    /   |  / __ \/ ____/
//    / /_   / / // /_   / /| | / /_/ / /     
//   / __/  / /__  __/  / ___ |/ ____/ /___   
//  /_/    /_/  /_/____/_/  |_/_/    \____/   
//               /_____/                      
//
// Code for F-14 Tomcat running on Arduino Pinball Controller
//
// General notes
// - Each major "thing" on the game has a related "handler" which will take care of processing switch hits,
//   lighting lamps, scoring etc etc.
//   This keeps the main game switch handler simple.

#include "F14_Constants.h"

byte F14_TomcatTargets[5][12]; // track status of the 12 Tomcat targets for each player - 0 shot not made, 1 shot made
const byte F14TomcatTargetLampNumbers[12] = {33,34,35,36,37,38,49,50,51,52,53,54};  // The lamp for the corresponding target
const byte F14_1to6LampNumbers[6] = {116,115,114,113,110,112};
const byte F14_1to6SwitchNumbers[6] = {43,42,41,44,45,46};

const byte F14_LockedOnSeq[23] = {25,5,26,5,27,5,28,5,29,5,30,5,29,5,28,5,27,5,26,5,25,5,0};


byte F14_Kills[5];  // How many kills (Alpha -> Golf) has the player made
byte F14_LockStatus[5][3]; // status of locks for each player.  0 not active, 1 is lit, 2 is locked, 3 contains unlocked ball
byte F14_LockOccupied[3]; // does the lock contain a ball.  0 = no, 1 = yes, 2 = no but waiting refill
byte F14_LandingStatus[5][3]; // Track status of multiball progress towards fighter jackpot
byte F14_YagovKills[5];  // tracks how many times we killed Yagov
byte F14_ExtraBallLit[5]; // is the extra ball lit for player?
byte F14_TomcatsCompleted[5]; // how many times has the player completed T-O-M-C-A-T
byte F14_LocksClearing; // flag to indicate if locks are still being cleared out for multiball
byte F14_LaunchBonus;
byte F14_Bonus;
byte F14_Multiplier;
byte F14_GI_IsOn = 0;

// When we are running a lampshow, we will temporarily "repoint" the APC lamp update code
// to our show arrays.  In this way the regular calls to TurnOn/TurnOff/Blink etc will update
// the original Lamp/LED arrays in the background and will catch up when we flip back after show
byte F14_ShowLamps[8];
byte F14_ShowLEDs[8];
const byte *apc_LEDStatus;


// All handlers in the game should generally have these events defined
const byte LAMP_UPDATE=200;   // Update the lamps (between players, for example)
const byte HANDLER_LAMP_RESET=254;  // 
bool lamp_show_running=false; // If we are running a lampshow of some kind, this will be set




const byte F14_SearchCoils[15] = {1,3,5,7,10,13,20,0}; // coils to fire when the ball watchdog timer runs out - has to end with a zero
unsigned int F14_SolTimes[32] = {30,50,50,50,50,50,30,50,50,50,50,50,50,50,50,0,50,50,50,50,50,50,0,0,100,100,100,100,100,100,100,100}; // Activation times for solenoids


#define F14set_OutholeSwitch 0
#define F14set_BallThroughSwitches 1
#define F14set_PlungerLaneSwitch 5
#define F14set_ACselectRelay 6
#define F14set_OutholeKicker 7
#define F14set_ShooterLaneFeeder 8
#define F14set_InstalledBalls 9

const byte F14_defaults[64] = {10,11,12,13,14,16,14,1,        // game default settings
                              2,4,0,0,0,0,0,0,
                              0,0,0,0,0,0,0,0,
                              0,0,0,0,0,0,0,0,
                              0,0,0,0,0,0,0,0,
                              0,0,0,0,0,0,0,0,
                              0,0,0,0,0,0,0,0,
                              0,0,0,0,0,0,0,0};


const struct SettingTopic F14_setList[13] = {{"OUTHOLESWITCH ",HandleNumSetting,0,1,64}, // defines the game specific settings
    {" BALL  THRU 1 ",HandleNumSetting,0,1,64},
    {" BALL  THRU 2 ",HandleNumSetting,0,1,64},
    {" BALL  THRU 3 ",HandleNumSetting,0,1,64},
    {" BALL  THRU 4 ",HandleNumSetting,0,1,64},
    {"PLUNGERLN SW  ",HandleNumSetting,0,1,64},
    {"AC SEL RELAY  ",HandleNumSetting,0,0,22},
    {"OUTHOLEKICKER ",HandleNumSetting,0,1,22},
    {"SHOOTERLN FEED",HandleNumSetting,0,1,22},
    {"INSTALD BALLS ",HandleNumSetting,0,1,3},
   // {" RESET  HIGH  ",F14_ResetHighScores,0,0,0},
    {"RESTOREDEFAULT",RestoreDefaults,0,0,0},
    {"  EXIT SETTNGS",ExitSettings,0,0,0},
    {"",NULL,0,0,0}};


struct GameDef F14_GameDefinition = {
    F14_setList,                                       // GameSettingsList
    (byte*)F14_defaults,                               // GameDefaultsPointer
    "F14_SET.BIN",                                     // GameSettingsFileName
    "F14_SCORE.BIN",                                   // HighScoresFileName
    F14_AttractMode,                                   // AttractMode
    F14_SolTimes};                                     // Default activation times of solenoids

void F14_init() {
  if (APC_settings[DebugMode]) {                      // activate serial interface in debug mode
    Serial.begin(115200);
    Serial.println("Debug on");}
  GameDefinition = F14_GameDefinition;                // read the game specific settings and highscores
}
  

void F14_AttractMode() {                               // Attract Mode
  F14_GIOn(255,255,255);                                        //switch on the gi
  ACselectRelay = game_settings[F14set_ACselectRelay]; // assign the number of the A/C select relay
  if (ACselectRelay) {
    F14_SolTimes[ACselectRelay-1] = 0;}                // allow A/C relay to be turned on permanently
  DispRow1 = DisplayUpper;
  DispRow2 = DisplayLower;
  digitalWrite(VolumePin,HIGH);                       // set volume to zero
  Switch_Pressed = F14_AttractModeSW;
  Switch_Released = DummyProcess;
  AppByte2 = 0;
  ActivateTimer(1000, 0, F14_AttractLampCycle);
  F14_AttractDisplayCycle(1);
  }



//     ___________   ___  __  __               __  __                   _____         __   
//    / __<  / / /  / _ |/ /_/ /________ _____/ /_/ /  ___ ___ _  ___  / ___/_ ______/ /__ 
//   / _/ / /_  _/ / __ / __/ __/ __/ _ `/ __/ __/ /__/ _ `/  ' \/ _ \/ /__/ // / __/ / -_)
//  /_/  /_/ /_/__/_/ |_\__/\__/_/  \_,_/\__/\__/____/\_,_/_/_/_/ .__/\___/\_, /\__/_/\__/ 
//            /___/                                            /_/        /___/            
//
// Loops through lamp shows during attract mode.  Call with 0 to start, 255 to end
void F14_AttractLampCycle(byte Event) {                // play multiple lamp pattern series
  static byte attract_lamp_timer = 0;
  static byte current_show=0;

  switch (Event) {
    case 0:
      if (attract_lamp_timer) {
        F14_LampShowPlayer(LAMP_SHOW_TWINKLE,QUIT_SHOW);
      }
      F14_LampShowPlayer(LAMP_SHOW_ROTATE,START_SHOW);
      current_show=0;
      attract_lamp_timer = ActivateTimer(10000,1,F14_AttractLampCycle);
      F14_GIOn(255,0,0);
      LEDhandling(6, 100);
      break;
    case 1:
      F14_LampShowPlayer(LAMP_SHOW_ROTATE,QUIT_SHOW);
      F14_LampShowPlayer(LAMP_SHOW_UPDOWN,START_SHOW);
      current_show = 1;
      attract_lamp_timer = ActivateTimer(10000,2,F14_AttractLampCycle);
      F14_GIOn(0,255,0);
      LEDhandling(6, 101);
      break;
    case 2:
      F14_LampShowPlayer(LAMP_SHOW_UPDOWN,QUIT_SHOW);
      F14_LampShowPlayer(LAMP_SHOW_TWINKLE,START_SHOW);
      current_show = 3;
      attract_lamp_timer = ActivateTimer(10000,0,F14_AttractLampCycle);
      F14_GIOn(0,0,255);
      LEDhandling(6, 100);
      break;
    case QUIT_HANDLER:
      if (attract_lamp_timer) {
        KillTimer(attract_lamp_timer);
        attract_lamp_timer = 0;
        F14_GIOn(255,255,255);
      }    
      F14_LampShowPlayer(current_show,QUIT_SHOW);
      LEDhandling(6, 100);
  }

 
 }                               
 

void F14_AttractDisplayCycle(byte Step) {
  static byte Timer0 = 0;
  static byte Timer1 = 0;
  static byte Timer2 = 0;
  switch (Step) {
  case 0:                                             // stop command
    if (Timer0) {
      KillTimer(Timer0);
      Timer0 = 0;}
    if (Timer1) {
      KillTimer(Timer1);
      Timer1 = 0;}
    if (Timer2) {
      KillTimer(Timer2);
      Timer2 = 0;}
    ScrollUpper(100);                                 // stop scrolling
    ScrollLower2(100);
    return;
  case 1:                                             // attract mode title 'page'
    WriteUpper2(" SECONDSORTIE ");
    Timer1 = ActivateTimer(50, 99, F14_AttractDisplayCycle);
    WriteLower2(" F-14    F-14 ");
    Timer2 = ActivateTimer(1000, 100, F14_AttractDisplayCycle);
    if (NoPlayers) {                                  // if there were no games before skip the next step
      Step++;}
    else {
      Step = 3;}
    break;
  case 2:                                             // show scores of previous game
    WriteUpper2("                ");                  // erase display
    WriteLower2("                ");
    for (i=1; i<=NoPlayers; i++) {                    // display the points of all active players
      ShowNumber(8*i-1, Points[i]);}
    Timer1 = ActivateTimer(50, 99, F14_AttractDisplayCycle);
    Timer2 = ActivateTimer(900, 100, F14_AttractDisplayCycle);
    Step++;
    break;
  case 3:                                             // attract mode title 'page'
    WriteUpper2(" HIGH  SCORES ");
    Timer1 = ActivateTimer(50, 99, F14_AttractDisplayCycle);
    WriteLower2("                ");
    Timer2 = ActivateTimer(1000, 100, F14_AttractDisplayCycle);
    Step++;
    break;
  case 4:                                             // Show highscores
    WriteUpper2("1-              ");
    WriteLower2("2-              ");
    for (i=0; i<3; i++) {
      *(DisplayUpper2+8+2*i) = DispPattern1[(HallOfFame.Initials[i]-32)*2];
      *(DisplayUpper2+8+2*i+1) = DispPattern1[(HallOfFame.Initials[i]-32)*2+1];
      *(DisplayLower2+8+2*i) = DispPattern2[(HallOfFame.Initials[3+i]-32)*2];
      *(DisplayLower2+8+2*i+1) = DispPattern2[(HallOfFame.Initials[3+i]-32)*2+1];}
    ShowNumber(15, HallOfFame.Scores[0]);
    ShowNumber(31, HallOfFame.Scores[1]);
    Timer1 = ActivateTimer(50, 99, F14_AttractDisplayCycle);
    Timer2 = ActivateTimer(900, 100, F14_AttractDisplayCycle);
    Step++;
    break;
  case 5:
    WriteUpper2("3-              ");
    WriteLower2("4-              ");
    for (i=0; i<3; i++) {
      *(DisplayUpper2+8+2*i) = DispPattern1[(HallOfFame.Initials[6+i]-32)*2];
      *(DisplayUpper2+8+2*i+1) = DispPattern1[(HallOfFame.Initials[6+i]-32)*2+1];
      *(DisplayLower2+8+2*i) = DispPattern2[(HallOfFame.Initials[9+i]-32)*2];
      *(DisplayLower2+8+2*i+1) = DispPattern2[(HallOfFame.Initials[9+i]-32)*2+1];}
    ShowNumber(15, HallOfFame.Scores[2]);
    ShowNumber(31, HallOfFame.Scores[3]);
    Timer1 = ActivateTimer(50, 99, F14_AttractDisplayCycle);
    Timer2 = ActivateTimer(900, 100, F14_AttractDisplayCycle);
    Step++;
    break;
  case 6:                                             // attract mode title 'page'
    WriteUpper2("ARDUINOPINBALL");
    Timer1 = ActivateTimer(50, 99, F14_AttractDisplayCycle);
    WriteLower2("CONTROL  APC  ");
    Timer2 = ActivateTimer(1000, 100, F14_AttractDisplayCycle);
    Step=1;
    break;
  
  case 99:                                             // scrolling routine called here to keep track of the timer
    Timer1 = 0;
    ScrollUpper(0);
    return;
  case 100:
    Timer2 = 0;
    ScrollLower2(0);
    return;}
  F14_CheckForLockedBalls(0);                          // check for a ball in the outhole
  Timer0 = ActivateTimer(4000, Step, F14_AttractDisplayCycle);}  // come back for the next 'page'


//      ___________ __     ___   __  __                  __  __  ___          __    ______       __
//     / ____<  / // /    /   | / /_/ /__________ ______/ /_/  |/  /___  ____/ /__ / ___/ |     / /
//    / /_   / / // /_   / /| |/ __/ __/ ___/ __ `/ ___/ __/ /|_/ / __ \/ __  / _ \\__ \| | /| / / 
//   / __/  / /__  __/  / ___ / /_/ /_/ /  / /_/ / /__/ /_/ /  / / /_/ / /_/ /  __/__/ /| |/ |/ /  
//  /_/    /_/  /_/____/_/  |_\__/\__/_/   \__,_/\___/\__/_/  /_/\____/\__,_/\___/____/ |__/|__/   
//               /_____/                                                                           
// 
// Handles switch activation during attract mode.  Very few will actually process
// just start, outhole (in case ball drain), settings button etc
void F14_AttractModeSW(byte Button) {                  // Attract Mode switch behaviour
  if (APC_settings[DebugMode]){
    Serial.print("Attract mode switch = ");            // print address reference table
    Serial.println((byte)Button);
  }
  switch (Button) {
  case 8:                                             // high score reset
    digitalWrite(Blanking, LOW);                      // invoke the blanking
    rstc_start_software_reset(RSTC); //call reset
    break;
  case 20:                                            // outhole
    ActivateTimer(200, 0, F14_CheckForLockedBalls);    // check again in 200ms
    break;

  case 72:                                            // Service Mode
    BlinkScore(0);                                    // stop score blinking
    KillAllTimers();
    BallWatchdogTimer = 0;
    CheckReleaseTimer = 0;
    LampPattern = NoLamps;                            // Turn off all lamps
    ReleaseAllSolenoids();
    if (APC_settings[DebugMode]) {                    // deactivate serial interface in debug mode
      Serial.end();}
    if (!QuerySwitch(73)) {                           // Up/Down switch pressed?
      WriteUpper("  TEST  MODE    ");
      WriteLower("                ");
      AppByte = 0;
      ActivateTimer(1000, 0, F14_Testmode);}
    else {
      Settings_Enter();}
    break;
  case 3:                                             // start game
    if (F14_CountBallsInTrunk() == game_settings[F14set_InstalledBalls] || (F14_CountBallsInTrunk() == game_settings[F14set_InstalledBalls]-1 && QuerySwitch(game_settings[F14set_PlungerLaneSwitch]))) { // Ball missing?
      Switch_Pressed = DummyProcess;                  // Switches do nothing
      F14_AttractDisplayCycle(0);
      if (APC_settings[Volume]) {                     // system set to digital volume control?
        analogWrite(VolumePin,255-APC_settings[Volume]);} // adjust PWM to volume setting
      else {
        digitalWrite(VolumePin,HIGH);}                // turn off the digital volume control
      for (i=0; i< 8; i++) {                          // turn off all lamps
        LampColumns[i] = 0;}
      LampPattern = LampColumns;
      NoPlayers = 0;
      WriteUpper("                ");
      WriteLower("                ");
      Ball = 1;
      F14_AddPlayer();
      Player = 1;
      ExBalls = 0;
      Bonus = 1;
      BonusMultiplier = 1;
      InLock = 0;
      Multiballs = 1;
      F14_AttractLampCycle(255);
      for (i=1; i < 5; i++) {
        //LockedBalls[i] = 0;
        F14_YagovKills[i]=0;
        F14_ExtraBallLit[i]=0;
        Points[i] = 0;
        F14_Kills[i]=0;  // How many kills (Alpha -> Golf) has the player made
        F14_TomcatsCompleted[i]=0;
        for (byte j=0; j<3; j++) {
          F14_LockStatus[i][j]=0;
          F14_LandingStatus[i][j]=0;
          } // status of locks for each player.  0 not active, 1 is lit, 2 is locked
        for (byte j=0; j<12; j++) {
          F14_TomcatTargets[i][j]=0;}
        }
      F14_LockOccupied[0] = 0;  //no ball physically in any of the locks
      F14_LockOccupied[1] = 0;
      F14_LockOccupied[2] = 0;
      F14_LocksClearing = 0; 
      F14_LineOfDeathHandler(RESET_KILL_LAMPS);  // sort the kill lamps out
      F14_RescueTargetHandler(START_HANDLER); // start the rescue target flip/flop
      F14_1to6Handler(1);       // start the 1-6 lamps
      F14_NewBall(4); // release a new ball (4 expected balls in the trunk)
      F14_TomcatTargetLamps();
      F14_Bonus = 0;
      F14_Multiplier = 1;
      ActivateSolenoid(0, 23);                        // enable flipper fingers
      ActivateSolenoid(0, 24);}

    }}



void F14_AddPlayer() {
  if (APC_settings[DebugMode]){
    Serial.println("F14_AddPlayer");            // print address reference table
  }

  if ((NoPlayers < 4) && (Ball == 1)) {               // if actual number of players < 4
    NoPlayers++;                                      // add a player
    Points[NoPlayers] = 0;                            // delete the points of the new player
    ShowPoints(NoPlayers);}}                          // and show them

// Called at game over
void F14_CheckForLockedBalls(byte Event) {             // check if balls are locked and release them
  if (APC_settings[DebugMode]){
    Serial.println("F14_CheckForLockedBalls");            // print address reference table
  }

  UNUSED(Event);
  if (QuerySwitch(10)) {                     // for the outhole
    ActA_BankSol(1);}
  if (QuerySwitch(22)) {  // Lock1
    ActivateSolenoid(0,10);
  }
  if (QuerySwitch(23)) {  // Lock 2
    ActivateTimer(1000,5,F14_ActivateSolenoid);
  }
  if (QuerySwitch(21)) {  // Lock 3
    ActivateTimer(1000, 7, F14_ActivateSolenoid);
  }
  if (QuerySwitch(24)) {
    ActivateSolenoid(0,3);
  }
}                                                     // add the locks of your game here

// Normal coil activation, but can be called from a timer
void F14_ActivateSolenoid (byte Coil) {
  ActivateSolenoid(0,Coil);
}

void F14_NewBall(byte Balls) {                         // release ball (Event = expected balls on ramp)
  if (APC_settings[DebugMode]){
    Serial.println("F14_NewBall");            // print address reference table
  }

  F14_ShowAllPoints(0);
  F14_RescueKickerHandler(START_HANDLER);                         // Light the kickback at ball start
  F14_LockHandler(7); // reset locks back to lit instead of locked if no longer contain a ball.
  F14_LockLampHandler();                                 // Lock handler reset
  F14_Multiplier = 1;
  F14_Bonus = 0;
  ExBalls = 0;
  F14_BonusHandler(BONUS_LAMP_REFRESH);                                // Reset bonus lamps
  if (F14_Kills[Player] == 7) {  // reset kills if we completed them all on previous ball
    F14_Kills[Player] = 0;
  }
  F14_LineOfDeathHandler(RESET_KILL_LAMPS);
  F14_SpinnerHandler(SPINNER_RESET);
  F14_OrbitHandler(ORBIT_RESET);
  F14_LaunchBonusHandler(LAUNCH_BONUS_RESET);
  PlayMusic(50, "1_02.snd");                      // play music track
  QueueNextMusic("1_02.snd");  //loop it

  if (APC_settings[DisplayType] < 2) {                // credit display present?
    *(DisplayUpper+16) = LeftCredit[32 + 2 * Ball];}  // show current ball in left credit
  BlinkScore(1);                                      // start score blinking
  Switch_Released = F14_CheckShooterLaneSwitch;
  if (!QuerySwitch(game_settings[F14set_PlungerLaneSwitch])) {
    ActA_BankSol(game_settings[F14set_ShooterLaneFeeder]);               // release ball
    Switch_Pressed = F14_BallReleaseCheck;             // set switch check to enter game
    CheckReleaseTimer = ActivateTimer(5000, Balls-1, F14_CheckReleasedBall);} // start release watchdog
  else {
    Switch_Pressed = F14_ResetBallWatchdog;}}

void F14_GiveBall(byte Balls) {                         // release ball (Event = expected balls on ramp)
if (APC_settings[DebugMode]){
    Serial.println("F14_GiveBall");            // print address reference table
    
  }
  F14_ShowAllPoints(0);
  if (APC_settings[DisplayType] < 2) {                // credit display present?
    *(DisplayUpper+16) = LeftCredit[32 + 2 * Ball];}  // show current ball in left credit
  BlinkScore(1);                                      // start score blinking
  Switch_Released = F14_CheckShooterLaneSwitch;
  if (!QuerySwitch(game_settings[F14set_PlungerLaneSwitch])) {
    ActA_BankSol(game_settings[F14set_ShooterLaneFeeder]);               // release ball
    Switch_Pressed = F14_BallReleaseCheck;             // set switch check to enter game
    CheckReleaseTimer = ActivateTimer(5000, Balls-1, F14_CheckReleasedBall);} // start release watchdog
  else {
    Switch_Pressed = F14_ResetBallWatchdog;}}


void F14_CheckShooterLaneSwitch(byte Switch) {
    if (APC_settings[DebugMode]){
    Serial.print("F14_CheckShooterLaneSwitch with switch ");            // print address reference table
    Serial.println((byte)Switch);
  }

  if (Switch == 16) { // shooter lane switch released?
    Switch_Released = DummyProcess;
    //LampShowXX(99);
    if (!BallWatchdogTimer) {
      BallWatchdogTimer = ActivateTimer(30000, 0, F14_SearchBall);}}}

void F14_BallReleaseCheck(byte Switch) {               // handle switches during ball release
  if (APC_settings[DebugMode]){
    Serial.print("F14_BallReleaseCheck with switch ");            // print address reference table
    Serial.println((byte)Switch);
  }

  if (Switch > 15) {                                  // edit this to be true only for playfield switches
    if (CheckReleaseTimer) {
      KillTimer(CheckReleaseTimer);
      CheckReleaseTimer = 0;}                         // stop watchdog
    Switch_Pressed = F14_ResetBallWatchdog;
    if (Switch == game_settings[F14set_PlungerLaneSwitch]) { // ball is in the shooter lane
      Switch_Released = F14_CheckShooterLaneSwitch;}   // set mode to register when ball is shot
    else {
      if (!BallWatchdogTimer) {
        BallWatchdogTimer = ActivateTimer(30000, 0, F14_SearchBall);}}} // set switch mode to game
  F14_GameMain(Switch);}                               // process current switch

void F14_ResetBallWatchdog(byte Switch) {              // handle switches during ball release
  if (APC_settings[DebugMode]){
    Serial.print("F14_ResetBallWatchdog with switch ");            // print address reference table
    Serial.println((byte)Switch);
  }

  if (Switch > 19) {                                  // edit this to be true only for playfield switches
    if (BallWatchdogTimer) {
      KillTimer(BallWatchdogTimer);}                  // stop watchdog
    BallWatchdogTimer = ActivateTimer(30000, 0, F14_SearchBall);}
  F14_GameMain(Switch);}                               // process current switch

void F14_SearchBall(byte Counter) {                    // ball watchdog timer has run out
  if (APC_settings[DebugMode]){
    Serial.print("F14_SearchBall with counter ");            // print address reference table
    Serial.println((byte)Counter);
  }

  BallWatchdogTimer = 0;
  if (QuerySwitch(10)) {
    BlockOuthole = false;
    ActivateTimer(1000, 0, F14_ClearOuthole);}
  else {
    if (QuerySwitch(16)) { // if ball is waiting to be launched
      BallWatchdogTimer = ActivateTimer(30000, 0, F14_SearchBall);}  // restart watchdog
    else {                                            // if ball is really missing
      byte c = F14_CountBallsInTrunk();                // recount all balls
      if (c == 4) { // found all balls in trunk?
        if (BlockOuthole) {                           // is the outhole blocked
          F14_BallEnd(0);}                             // then it was probably a ball loss gone wrong
        else {
          ActivateTimer(1000, 4, F14_NewBall);}} // otherwise try it with a new ball
      else {
        byte c2 = 0;                                  // counted balls in lock
                  // count balls in lock here with 5 being a warning when the switch states don't add up
        if (c == 5) {                                 // balls have not settled yet
          WriteUpper("  LOCK  STUCK   ");
          BallWatchdogTimer = ActivateTimer(1000, 0, F14_SearchBall);} // and try again in 1s
        else {
          if (c2 > InLock) {                          // more locked balls found than expected?
            F14_HandleLock(0);                         // lock them
            BallWatchdogTimer = ActivateTimer(30000, 0, F14_SearchBall);}
          else {
            WriteUpper("  BALL  SEARCH  ");
            ActivateSolenoid(0, F14_SearchCoils[Counter]); // fire coil to get ball free
            Counter++;
            if (!F14_SearchCoils[Counter]) {           // all coils fired?
              Counter = 0;}                           // start again
            BallWatchdogTimer = ActivateTimer(1000, Counter, F14_SearchBall);}}}}}} // come again in 1s if no switch is activated



//     ___________   ___       __________    _                  ______       __  __        __    _______           __  
//    / __<  / / /  / _ )___ _/ / / __/ /__ (_)__  ___  ___ ___/ / __ \__ __/ /_/ /  ___  / /__ / ___/ /  ___ ____/ /__
//   / _/ / /_  _/ / _  / _ `/ / /\ \/  '_// / _ \/ _ \/ -_) _  / /_/ / // / __/ _ \/ _ \/ / -_) /__/ _ \/ -_) __/  '_/
//  /_/  /_/ /_/__/____/\_,_/_/_/___/_/\_\/_/ .__/ .__/\__/\_,_/\____/\_,_/\__/_//_/\___/_/\__/\___/_//_/\__/\__/_/\_\ 
//            /___/                        /_/  /_/                                                                    
// Called if a trough switch activates during game play.  This can happen if 
// a ball skips the outhole switch and jumps right to the trough.
void F14_BallSkippedOutholeCheck(byte Event) {
  
  static byte trough_settle_timer=0;
  
  switch (Event) {
    case 0:  // If the timer is already running, do nothing, otherwise start the timer.
      if (!trough_settle_timer) {
         trough_settle_timer = ActivateTimer(1000, 1, F14_BallSkippedOutholeCheck);
      }
      break;
    case 1:  // timer has allowed the trough to settle, so work out if we have more balls than expected
      trough_settle_timer = 0;
      if ( ((F14_CountBallsInTrunk() + InLock + Multiballs) > 4) && !BlockOuthole ) {
        BlockOuthole = true;
        F14_BallEnd(0);  // if we do have more than expected, should be safe to end ball
      }
  }

}


//      ___________ __    _____ __                  ___    ________        _       __      
//     / ____<  / // /   / ___// /_  ____ _      __/   |  / / / __ \____  (_)___  / /______
//    / /_   / / // /_   \__ \/ __ \/ __ \ | /| / / /| | / / / /_/ / __ \/ / __ \/ __/ ___/
//   / __/  / /__  __/  ___/ / / / / /_/ / |/ |/ / ___ |/ / / ____/ /_/ / / / / / /_(__  ) 
//  /_/    /_/  /_/____/____/_/ /_/\____/|__/|__/_/  |_/_/_/_/    \____/_/_/ /_/\__/____/  
//               /_____/                                                                   
//
// The standard ShowAllPoints just shows the scores, but we want the display to show
// the ball number too.
void F14_ShowAllPoints(byte Dummy) {
  
  ShowAllPoints(0); // do the standard display

  // Then display the ball count.  Need to revisit when 4 players.
  if (NoPlayers < 4) {
    switch (Ball) {
      case 1:
        WritePlayerDisplay((char *) " BALL 1",4);
        break;
      case 2:
        WritePlayerDisplay((char *) " BALL 2",4);
        break;
      case 3:
        WritePlayerDisplay((char *) " BALL 3",4);
        break;
      case 4:
        WritePlayerDisplay((char *) " BALL 4",4);
        break;
      case 5:
        WritePlayerDisplay((char *) " BALL 5",4);
        break;
      
    }
  }
}

byte F14_CountBallsInTrunk() {
  byte Balls = 0;
  for (i=0; i<4; i++) { // check how many balls are on the ball ramp
    if (QuerySwitch(game_settings[F14set_BallThroughSwitches+i])) {
      if (Balls < i) {
        return 5;}                                    // send warning
      Balls++;}}
  return Balls;}

void F14_CheckReleasedBall(byte Balls) {               // ball release watchdog
  if (APC_settings[DebugMode]){
    Serial.print("F14_CheckReleasedBall with balls ");            // print address reference table
    Serial.println((byte)Balls);
  }

  CheckReleaseTimer = 0;
  BlinkScore(0);                                      // stop blinking to show messages
  WriteUpper("WAITINGFORBALL  ");                     // indicate a problem
  WriteLower("                ");
  if (Balls == 10) {                                  // indicating a previous trunk error
    WriteUpper("                ");
    WriteLower("                ");
    F14_ShowAllPoints(0);
    BlinkScore(1);
    ActA_BankSol(2);  // kick ball to shooter lane
    } 
  byte c = F14_CountBallsInTrunk();
  if (c == Balls) {                                   // expected number of balls in trunk
    WriteUpper("  BALL MISSING  ");
    if (QuerySwitch(10)) { // outhole switch still active?
      ActA_BankSol(1);}}  // shove the ball into the trunk
  else {                                              //
    if (c == 5) {                                     // balls not settled
      WriteLower(" TRUNK  ERROR   ");
      Balls = 10;}
    else {
      if ((c > Balls) || !c) {                        // more balls in trunk than expected or none at all
        WriteUpper("                ");
        WriteLower("                ");
        F14_ShowAllPoints(0);
        BlinkScore(1);
        ActA_BankSol(2);}}} // release again
  CheckReleaseTimer = ActivateTimer(5000, Balls, F14_CheckReleasedBall);}


//     ___________  _____               __  ___     _    
//    / __<  / / / / ___/__ ___ _  ___ /  |/  /__ _(_)__ 
//   / _/ / /_  _// (_ / _ `/  ' \/ -_) /|_/ / _ `/ / _ \
//  /_/  /_/ /_/__\___/\_,_/_/_/_/\__/_/  /_/\_,_/_/_//_/
//            /___/                                      
//
// This is kind of the "main loop" of the game.  Switch events get handled here.
// Generally tried to keep this function clean and have it call to handlers that
// look after the various functions that are going on.
void F14_GameMain(byte Event) {                        // game switch events
  static unsigned long prev_switch_hit[75];
  
  unsigned long time_now;

  // Debounce the switches - only pass on a switch event if it hasn't
  // activated in the previous 200ms
  time_now=millis();
  if (time_now - prev_switch_hit[Event] > 200) {
    prev_switch_hit[Event] = time_now;
  }
  else {
    return;
  }
  if (APC_settings[DebugMode]){
    Serial.print("Game mode switch ");            // print address reference table
    Serial.println((byte)Event);
  }
  switch (Event) {
  case 1:                                             // plumb bolt tilt
  case 2:                                             // ball roll tilt
  case 7:                                             // slam tilt
  case 9:                                            // playfield tilt
    WriteUpper(" TILT  WARNING  ");
    ActivateTimer(3000, 0, F14_ShowAllPoints);
    break;
  case 8:
    Serial.println("Game Info");
    Serial.print("Player Number ");
    Serial.println((byte)Player);
    Serial.print("Ball Number ");
    Serial.println((byte)Ball);
    Serial.print("In Lock ");
    Serial.println((byte)InLock);
    Serial.print("Multiballs ");
    Serial.println((byte)Multiballs);

    break;

  case 3:                                             // credit button
    F14_AddPlayer();
    break;
  case 11:   // if the trough switches activate 
  case 12:   // then it might be because the ball
  case 13:   // completely missed the outhole switch
  case 14:   // so check how many balls there
    F14_BallSkippedOutholeCheck(0);
    break;
  case 20:  // ramp entry
    F14_DivertorHandler(0);  // let the divertor know a ball is coming
    break;
  case 21:                                            //right eject
  case 22:                                            //left
  case 23:                                            //centre
    F14_LockHandler(Event);  // lock switch activated
    break;  
  case 24:    //vuk
   
    F14_vUKHandler(VUK_BALL_CAPTURED);
    break;
  case 25:  // left rescue target
    F14_CentreKillHandler(1); // will award kill if it was lit
    F14_RescueTargetHandler(RESCUE_TARGET_LEFT_HIT);
    break;
  case 26:
    F14_CentreKillHandler(1);
    F14_RescueTargetHandler(RESCUE_TARGET_RIGHT_HIT);
    break;
  case 30:
  case 31:
  case 32:
    F14_LockHandler(Event);
    break;
  // Lower TOMCAT targets
  case 33:
  case 34:    
  case 35:
  case 36:
  case 37:
  case 38:
    F14_TomcatTargetHandler(Event-33);
    break;
  // Centre 1-6 targets
  case 41:
  case 42:
  case 43:
  case 44:
  case 45:
  case 46:
    F14_1to6Handler(Event);
    break;
  
  case 47:
    F14_OrbitHandler(ORBIT_RIGHT_SWITCH_HIT);  //right orbit
    break;
  case 48: // Spinner
    F14_SpinnerHandler(SPINNER_HIT);
    break;
  // Upper TOMCAT targets
  case 49:
  case 50:
  case 51:
  case 52:
  case 53:
  case 54:
    F14_HotStreakHandler(1);
    F14_TomcatTargetHandler(Event-43);
    break;
  // Line of death (Yagov)
  case 55:
    F14_LineOfDeathHandler(LINE_OF_DEATH_HIT);
    break;
  case 56:  //left orbit switch
    F14_OrbitHandler(ORBIT_LEFT_SWITCH_HIT);
    break;
  case 59: // left inlane
    F14_OrbitHandler(ORBIT_LIGHT_RIGHT_SIDE_BONUS);  // light the right bonus x lane
    F14_LaunchBonusHandler(LAUNCH_BONUS_SCORE); // launch bonus
    F14_BonusHandler(0); // increment end of ball bonus
    if (F14_YagovKills[Player]==0) {  // if the inlane 'lite kill' are active, tell the handler
      F14_CentreKillHandler(0);
    }
    break;
  case 60:  //right inlane
    F14_OrbitHandler(ORBIT_LIGHT_LEFT_SIDE_BONUS);
    F14_BonusHandler(BONUS_INCREMENT); // increment end of ball bonus
    if (F14_YagovKills[Player]==0) { // if the inlane 'lite kill' are active, tell the handler
      F14_CentreKillHandler(0);
    }
    
    break;
  case 61: // left drain
    F14_RescueKickerHandler(RESCUE_OUTLANE_SWITCH_HIT);
    F14_BonusHandler(BONUS_INCREMENT); // increment end of ball bonus
    break;
  case 62: // right drain
    F14_BonusHandler(BONUS_INCREMENT); // increment end of ball bonus
    break;
  case 65: // left slingshot
    ActivateSolenoid(0, 17); // fire kicker
    Points[Player] += 10;  //give some points
    ShowPoints(Player);  // display them
    ActC_BankSol(3); // flasher
    PlaySound(50, "0_BE.snd");
    break;
  case 66: // right slingshot
    ActivateSolenoid(0, 18);  // fire the sling
    Points[Player] += 10;
    ShowPoints(Player);
    ActC_BankSol(2); // flasher
    PlaySound(50, "0_BE.snd");
    break;
  case 68: // pop bumper
    Points[Player] += 100;
    ShowPoints(Player);
    ActivateSolenoid(0, 20);
    break;
  default:
    if (Event == 10) {
      ActivateTimer(200, 0, F14_ClearOuthole);}        // check again in 200ms
  }}


//      ___________ __     __  __      __ _____ __                  __   __  __                ____         
//     / ____<  / // /    / / / /___  / // ___// /_________  ____ _/ /__/ / / /___ _____  ____/ / /__  _____
//    / /_   / / // /_   / /_/ / __ \/ __|__ \/ __/ ___/ _ \/ __ `/ //_/ /_/ / __ `/ __ \/ __  / / _ \/ ___/
//   / __/  / /__  __/  / __  / /_/ / /____/ / /_/ /  /  __/ /_/ / ,< / __  / /_/ / / / / /_/ / /  __/ /    
//  /_/    /_/  /_/____/_/ /_/\____/\__/____/\__/_/   \___/\__,_/_/|_/_/ /_/\__,_/_/ /_/\__,_/_/\___/_/     
//               /_____/                                                                                    
// After an orbit loop is completed, the hot streak lamp lights for a short time
// Hits to the top TOMCAT targets score 10,000 -> 100,000 and reset the timer
// Event 0 - enable Hotstreak
// Event 1 - score hotstreak if applicable
// Event 2 - hotstreak timeout
void F14_HotStreakHandler(byte Event) {
  static byte streak_timer = 0;
  static byte streak_display_timer = 0;
  static byte streak_score = 0;
  static byte streak_step = 0;
  
 if (APC_settings[DebugMode]){
    Serial.print("F14_HotStreakHandler event ");            // print address reference table
    Serial.println((byte)Event);
  }


  switch(Event) {
    case 0:
      if (streak_timer) {  // if already running, ignore
        break;
      }
      else {
        streak_score = 0;
        TurnOnLamp(3); // Hot Streak
        streak_timer = ActivateTimer(2000,2,F14_HotStreakHandler);  // for 2 seconds only
      }
      break;
    case 1:
      if (!streak_timer) {  // if not running, just exit
        break;
      }
      
      KillTimer(streak_timer); // Kill existing timer
      streak_timer = 0;
      if (streak_score < 10) {
        streak_score ++;
      }

      WriteUpper2("              ");
      WriteLower2("              ");
      SwitchDisplay(0);  // over to buffer 2
     
      streak_timer = ActivateTimer(3000,2,F14_HotStreakHandler);  // reset the timer back to 2 seconds
      Points[Player] += streak_score * 10000;
      if (streak_score == 1) {  // if we just started scoring, kick off the display routine
        F14_HotStreakHandler(3);
      }
      break;
    case 2:  // Overall streak timer has run out, shut it down
      streak_timer = 0;
      if (streak_score) {
        SwitchDisplay(1);  // only need to put display back if we scored something
      }
      streak_score = 0;
      streak_step = 0;
      TurnOffLamp(3);
      if (streak_display_timer) {
        KillTimer(streak_display_timer);  // Kill the timer that was flashing the display
        streak_display_timer = 0;
      }
      break;
    case 3:  // flash the display 
      switch (streak_step) {
        case 0:
          WriteUpper2("  HOT         ");
          WriteLower2("              ");
          ShowNumber(30,streak_score*10000);
          streak_display_timer = ActivateTimer(300,3,F14_HotStreakHandler);
          streak_step = 1;
          break;
        case 1:
          WriteUpper2("        STREAK");
          WriteLower2("              ");
          ShowNumber(22,streak_score*10000);
          streak_display_timer = ActivateTimer(300,3,F14_HotStreakHandler);
          streak_step = 0;
          break;
      }

  }
}



//     ___________ ______                    __ ______                  __  __                     
//    / __<  / / //_  __/__  __ _  _______ _/ //_  __/__ ________ ____ / /_/ /  ___ ___ _  ___  ___
//   / _/ / /_  _/ / / / _ \/  ' \/ __/ _ `/ __// / / _ `/ __/ _ `/ -_) __/ /__/ _ `/  ' \/ _ \(_-<
//  /_/  /_/ /_/__/_/  \___/_/_/_/\__/\_,_/\__//_/  \_,_/_/  \_, /\__/\__/____/\_,_/_/_/_/ .__/___/
//            /___/                                         /___/                       /_/        
// Update the T-O-M-C-A-T lamps based on the status of the shots
void F14_TomcatTargetLamps() {
  
  for (byte i=0; i<12; i++) {
    // Turn off lamps and blinkers
    RemoveBlinkLamp(F14TomcatTargetLampNumbers[i]);
    TurnOffLamp(F14TomcatTargetLampNumbers[i]);
    // Set them to blink on status 0 (not hit) or on for 1 (hit)
    switch (F14_TomcatTargets[Player][i]) {
      case 0: AddBlinkLamp(F14TomcatTargetLampNumbers[i],100);
      case 1: TurnOnLamp(F14TomcatTargetLampNumbers[i]);
    }
  }
}

// When playing a lamp show, we use alternative buffers for both the lamps and LEDS
// this means that regular lamp commands (on/off/blink etc) can still be happening
// on the original buffers so can be switched back to afterwards
// We therefore have special versions of on/off which address this second buffer.
// No blink option for the second buffer as not needed and would get messy
void F14Show_TurnOnLamp(byte Lamp) {
  if (Lamp < 65) {                                    // is it a matrix lamp?
    Lamp--;
    F14_ShowLamps[Lamp / 8] |= 1<<(Lamp % 8);}
  else {                                              // lamp numbers > 64 are additional LEDs
    F14_ShowLEDhandling(3, Lamp - 65);}}

void F14Show_TurnOffLamp(byte Lamp) {
  if (Lamp < 65) {                                    // is it a matrix lamp?
    Lamp--;
    F14_ShowLamps[Lamp /8] &= 255-(1<<(Lamp % 8));}
  else {                                              // lamp numbers > 64 are additional LEDs
    F14_ShowLEDhandling(4, Lamp - 65);}}

byte F14_ShowLEDhandling(byte Command, byte Arg) {            // main LED handler 2nd buffer

  switch(Command) {
  case 3:                                             // turn on LED
    F14_ShowLEDs[Arg / 8] |= 1<<(Arg % 8);
    break;
  case 4:                                             // turn off LED
    F14_ShowLEDs[Arg / 8] &= 255-(1<<(Arg % 8));
    break;
  case 5:                                             // query LED
    return F14_ShowLEDs[Arg / 8] & 1<<(Arg % 8);
}
  return(0);}



//     ___________   __                   ______              ___  __                 
//    / __<  / / /  / /  ___ ___ _  ___  / __/ /  ___ _    __/ _ \/ /__ ___ _____ ____
//   / _/ / /_  _/ / /__/ _ `/  ' \/ _ \_\ \/ _ \/ _ \ |/|/ / ___/ / _ `/ // / -_) __/
//  /_/  /_/ /_/__/____/\_,_/_/_/_/ .__/___/_//_/\___/__,__/_/  /_/\_,_/\_, /\__/_/   
//            /___/              /_/                                   /___/          
//
// This routine takes care of flipping over to the second lamp/LED buffers, calling the
// required lampshow and then flipping back afterwards
// ShowNumber - which of the shows
// Arg - 0 to start the show, 255 to stop it
void F14_LampShowPlayer(byte ShowNumber, byte Arg) {

  // point the buffers correctly
  switch (Arg) {
    case 0: // start - point the LED and Lamp buffers to the F14Show versions
      LEDpattern = F14_ShowLEDs;
      LampPattern = F14_ShowLamps;
      break;
    case 255: // stop
      LEDinit(); 
      LampPattern = LampColumns;
      break;
  }
  // call the lamp show player
  switch (ShowNumber) {
    case 0:
      F14_LampShowRotate(Arg);  // rotating wheel lamp show
      break;
    case 1:
      F14_LampShowUpDown(Arg);
      break;
    case 2:
      if (Arg==0) {  // don't care about sending a stop as it stops by itself
        F14_LampShowCentrePulse(0);
      }
      break;
    case 3:
      F14_LampShowTwinkle(Arg);
      break;

  }
}

// Version of AddBlinkLamp that schedules immediately
void AddBlinkLampImmediate(byte Lamp, unsigned int Period) {
  RemoveBlinkLamp(Lamp);                              // if the lamp is already blinking then stop it
  if (Period) {                                       // Only if the Period is not 0
    byte a = 0;
    byte x = 0;
    byte b = 0;
    bool Flag = false;
    while (BlinkTimer[x]) {                         // look for a free timer slot
        x++;
        if (x > 65) {                                 // max 64 blink timers possible (starting from 1)
          ErrorHandler(6,0,Lamp);}}                   // show error 6
      BlinkingLamps[x][0] = Lamp;                     // add the lamp
      BlinkingNo[x] = 1;                              // set the number of lamps for this timer to 1
      //BlinkState[x] = true;                           // start with lamps on
      BlinkPeriod[x] = Period;
      BlinkTimers++;                                  // increase the number of blink timers
      BlinkTimer[x] = ActivateTimer(Period, x, BlinkLamps);}} // start a timer and store it's number


// Code for setting the various lamps associated with the locks.  Keeps this "fluff" out
// of the main logic of the lock handler.
void F14_LockLampHandler() {

// single ball play
if (Multiballs==1) {
      switch (F14_LockStatus[Player][0]) {
        case 0:
          RemoveBlinkLamp(58); TurnOffLamp(58);
          break;
        case 1:
          AddBlinkLamp(58,100);
          break;
        case 2:
          RemoveBlinkLamp(58); TurnOnLamp(58);
          break;
      }
      switch (F14_LockStatus[Player][1]) {
        case 0:
          RemoveBlinkLamp(57); TurnOffLamp(57);
          break;
        case 1:
          AddBlinkLamp(57,100);
          break;
        case 2:
          RemoveBlinkLamp(57); TurnOnLamp(57);
          break;
      }
      switch (F14_LockStatus[Player][2]) {
        case 0:
          RemoveBlinkLamp(59); TurnOffLamp(59);
          break;
        case 1:
          AddBlinkLamp(59,100);
          break;
        case 2:
          RemoveBlinkLamp(59); TurnOnLamp(59);
          break;
      }
      
      //F14_LockHandler(10); // beacon check
      switch (F14_LandingStatus[Player][0]) {
        case 0:
          RemoveBlinkLamp(61);  TurnOffLamp(61);
          break;
        case 1:
          RemoveBlinkLamp(61);  TurnOnLamp(61);
          break;
        }
      switch (F14_LandingStatus[Player][1])   {
        case 0:
          RemoveBlinkLamp(60);  TurnOffLamp(60);
          break;
        case 1:
          RemoveBlinkLamp(60);  TurnOnLamp(60);
          break;

      }
      switch (F14_LandingStatus[Player][2])   {
        case 0:
          RemoveBlinkLamp(62);  TurnOffLamp(62);
          break;
        case 1:
          RemoveBlinkLamp(62);  TurnOnLamp(62);
          break;

      }
    // during single ball the beacons should be on and release lamp lit if all balls locked
    if (F14_LockStatus[Player][0]==2 && F14_LockStatus[Player][1]==2 && F14_LockStatus[Player][2]==2) {
      ActivateSolenoid(0,16);
      TurnOnLamp(111);
    }
    else {
      ReleaseSolenoid(16);
      TurnOffLamp(111);
    }
    // Landing at vuk should be off
    TurnOffLamp(40); 
    // Lock On should be lit if at least one lock is lit but not locked
    if (F14_LockStatus[Player][0]==1 || F14_LockStatus[Player][1]==1 || F14_LockStatus[Player][2]==1) {
      TurnOnLamp(48);
      
    }
    else {
      TurnOffLamp(48);
    }
    
  }
  // multiball in progress
  else {
    // The red lock lamps are off during multiball
    TurnOffLamp(57);  RemoveBlinkLamp(57);
    TurnOffLamp(58);  RemoveBlinkLamp(58);
    TurnOffLamp(59);  RemoveBlinkLamp(59);
    // The blue landing lights are either on or blinking
    if (F14_LandingStatus[Player][0]==0) {
      AddBlinkLamp(61, 100);
    }
    else {
      RemoveBlinkLamp(61); TurnOnLamp(61);
    }
    if (F14_LandingStatus[Player][1]==0) {
      AddBlinkLamp(60, 100);
    }
    else {
      RemoveBlinkLamp(60); TurnOnLamp(60);
    }
    if (F14_LandingStatus[Player][2]==0) {
      AddBlinkLamp(62, 100);
    }
    else {
      RemoveBlinkLamp(62); TurnOnLamp(62);
    }
    // during multiball the beacons should be on
    ActivateSolenoid(0,16);
    // Landing at the vUK should be on, release and lock on off
    TurnOnLamp(40); RemoveBlinkLamp(48); TurnOffLamp(111);
  }
}

// Lock handler
// Event 0 - TOMCAT completed
// 1 - enable lock 1
// 2 - enable lock 2
// 3 - enable lock 3
// 4 lock ball in 1
// 5 lock ball in 2
// 6 lock ball in 3
// 7 handle the lamps associated with the locks (3 red, 3 blue, lock on, release and landing)
// 8 start multiball
// 31 middle ramp switch
// 32 lower ramp switch
// 21-23 ball landed in lock
void F14_LockHandler(byte Event) {
  static byte locks_available;
  static char conv[5];

  if (APC_settings[DebugMode]){
    Serial.print("F14_LockHandler called with event ");            // print address reference table
    Serial.println((byte)Event);
    Serial.println("-> Pre handle");
    Serial.print(" -> Lock 1 status = ");
    Serial.println((byte) F14_LockStatus[Player][0]);
    Serial.print(" -> Lock 2 status = ");
    Serial.println((byte) F14_LockStatus[Player][1]);
    Serial.print(" -> Lock 3 status = ");
    Serial.println((byte) F14_LockStatus[Player][2]);
    Serial.print(" -> Lock 1 occupied = ");
    Serial.println((byte) F14_LockOccupied[0]);
    Serial.print(" -> Lock 2 occupied = ");
    Serial.println((byte) F14_LockOccupied[1]);
    Serial.print(" -> Lock 3 occupied = ");
    Serial.println((byte) F14_LockOccupied[2]);
    Serial.print(" -> Landing 1 status = ");
    Serial.println((byte) F14_LandingStatus[Player][0]);
    Serial.print(" -> Landing 2 status = ");
    Serial.println((byte) F14_LandingStatus[Player][1]);
    Serial.print(" -> Landing 3 status = ");
    Serial.println((byte) F14_LandingStatus[Player][2]);
    Serial.print(" -> InLock = ");
    Serial.println((byte) InLock);
    Serial.print(" -> MultiBalls = ");
    Serial.println((byte) Multiballs);
    

  }
 
  // Check how many locks we have spare
  locks_available = 0;
  for (byte i=0; i<3; i++) {
    if (F14_LockStatus[Player][i]==0) {
      locks_available++;
    }
  }

  switch (Event) {
    case 0:  // Completed TOMCAT, need to light a lock.  SHould make this more random.
      if (!locks_available)
        return;
      
      //F14_AnimationHandler(2,0);  // lock is lit animation (moved to target handler)

      if (F14_LockStatus[Player][0]==0) {
        F14_LockHandler(1);
      }
      else if (F14_LockStatus[Player][1]==0) {
        F14_LockHandler(2);
      }
      else {
        F14_LockHandler(3);
      }
      break;
    case 1: //enable lock 1
      F14_LockStatus[Player][0] = 1;
      F14_LockLampHandler();
      break;
    case 2: //enable lock 2
      F14_LockStatus[Player][1] = 1;
      F14_LockLampHandler();
      break;
    case 3: //enable lock 3
      F14_LockStatus[Player][2] = 1;
      F14_LockLampHandler();
      break;
    case 4: // Ball locked in 1
      F14_LockStatus[Player][0]=2;  // set the status
      if (F14_LockOccupied[0]==0){  // if the lock didn't have a ball in, then we need to issue another one
        InLock++;
        ActivateTimer(1000,1,F14_GiveBall);
      }
      F14_LockOccupied[0]=1;  // Set the lock as occupied
      F14_LockLampHandler();
      F14_AnimationHandler(ANIMATION_BALL_LOCKED ,ANIMATION_START);  // Enemy locked
      F14_LampShowPlayer(2,0);
      break;
    case 5: // Ball locked in 2
      F14_LockStatus[Player][1]=2;
      if (F14_LockOccupied[1]==0){
        InLock++;
        ActivateTimer(1000,1,F14_GiveBall);
      }
      F14_LockOccupied[1]=1;
      F14_LockLampHandler();
      F14_AnimationHandler(ANIMATION_BALL_LOCKED ,ANIMATION_START);  // Enemy locked
      F14_LampShowPlayer(2,0);
      break;
    case 6: // Ball locked in 3
      F14_LockStatus[Player][2]=2;
      if (F14_LockOccupied[2]==0){
        InLock++;
        ActivateTimer(1000,1,F14_GiveBall);
        //F14_GiveBall(1);
      }
      F14_LockOccupied[2]=1;
      F14_LockLampHandler();
      F14_AnimationHandler(ANIMATION_BALL_LOCKED ,ANIMATION_START);  // Enemy locked
      F14_LampShowPlayer(2,0);
      break;
    // if a player has a lock in place (status 2), but the lock doesn't actually contain a ball
    // then reset the status of the lock back to 1 (lock is lit)
    case 7:
      if (F14_LockStatus[Player][0] == 2 && F14_LockOccupied[0] == 0) {
        F14_LockStatus[Player][0]=1;
      }
      if (F14_LockStatus[Player][1] == 2 && F14_LockOccupied[1] == 0) {
        F14_LockStatus[Player][2]=1;
      }
      if (F14_LockStatus[Player][2] == 2 && F14_LockOccupied[2] == 0) {
        F14_LockStatus[Player][2]=1;
      }
      break;
    case 8:  // multiball intro
      F14_AnimationHandler(ANIMATION_START_MULTIBALL, ANIMATION_START);   // Flashing stuff, will get called back with Event 9 when done.
      break;
    case 9:  // start multiball
      Multiballs = 4; // There will be 4 on the playfield
      InLock = 0; // and none in the locks
      ActivateSolenoid(0,10);  // Clear lock 1
      ActivateTimer(1000,5,F14_ActivateSolenoid);  // do lock 2 in 1 second
      ActivateTimer(1200,7, F14_ActivateSolenoid); // and lock 3 shortly after
      ActivateTimer(2000,VUK_EJECT,F14_vUKHandler); // Then the vUK can clear the ball there
      F14_LocksClearing=1; // make a note that we are clearing locks
      for (byte i=0; i< 3; i++ ) { // reset status of locks
        F14_LockStatus[Player][i]=0;
        F14_LockOccupied[i]=0;
      }
      F14_LockLampHandler();  // update the lamps
      break;
    case 30:  // lower ramp
      if (QuerySwitch(21)) {  // If the lock has a ball in, we need to kick it out
        ActA_BankSol(7);
        F14_LockOccupied[2] = 2;  // Let the lock handler know we're expecting a replacement ball
      }
      break;
  
    case 31:  // middle ramp
      if (QuerySwitch(22)) {  // If the lock has a ball in, we need to kick it out
        ActivateSolenoid(0, 10);
        F14_LockOccupied[0] = 2; // Let the lock handler know we're expecting a replacement ball
      }
      ReleaseSolenoid (22);  // Can also release the divertor coil early
      break;
    case 32:  // upper ramp
      if (QuerySwitch(23)) {  // If the lock has a ball in, we need to kick it out
        ActA_BankSol(5);
        F14_LockOccupied[1] = 2; // Let the lock handler know we're expecting a replacement ball
      }
      ReleaseSolenoid (21);  // Can also release the divertor coil early
      break;
    case 22: // Lock number 1
      if (Multiballs==1) { // single ball play
        F14_BonusHandler(BONUS_INCREMENT); // increment end of ball bonus
        if (F14_LockOccupied[0]==2) {  // If waiting for a refill, mark lock with ball and exit
          F14_LockOccupied[0]=1;
          break;
        }
        else if (F14_LockOccupied[0]==1) {  // if we already knew a ball was in there, ignore the noisy switch
          break;
        }
        switch (F14_LockStatus[Player][0]) {  // check the lock status
          case 0:
            ActivateTimer(1000,10,F14_ActivateSolenoid);  // if not lit or locked, kick the ball out
            
            break;
          case 1:
            F14_LockHandler(4);  // If lit, lock the ball
            break;
          case 2: // Switch for lock, when ball already locked.  Probably bouncy switch
            break;
        }
      }
      else { // multiball is running
          F14_LandingHandler(0);
        }
      
      break;
    case 23: // Lock number 2
      if (Multiballs==1) {
        F14_BonusHandler(BONUS_INCREMENT); // increment end of ball bonus
        if (F14_LockOccupied[1]==2) {  // If waiting for a refill, mark lock with ball and exit
          F14_LockOccupied[1]=1;
          break;
        }
        else if (F14_LockOccupied[1]==1) {
          break;
        }
        switch (F14_LockStatus[Player][1]) {
          case 0:
            ActivateTimer(1000,5,F14_ActivateSolenoid);  // if not lit or locked, kick the ball out
            //ActA_BankSol(5);
            break;
          case 1:
            F14_LockHandler(5);
            break;
          case 2: // Switch for lock, when ball already locked.  Probably bouncy switch
            break;
        }
      }
      else {
        F14_LandingHandler(1);
      }
      break;
    case 21: // Lock number 3
      if (Multiballs==1) {
        F14_BonusHandler(BONUS_INCREMENT); // increment end of ball bonus
        if (F14_LockOccupied[2]==2) {  // If waiting for a refill, mark lock with ball and exit
          F14_LockOccupied[2]=1;
          break;
        }

        else if (F14_LockOccupied[2]==1) {
          break;
        }
        switch (F14_LockStatus[Player][2]) {
          case 0:
            ActivateTimer(1000,7,F14_ActivateSolenoid);  // if not lit or locked, kick the ball out
            //ActA_BankSol(7);
            break;
          case 1:
            F14_LockHandler(6);
            break;
          case 2: // Switch for lock, when ball already locked.  Probably bouncy switch
            break;
        }
      }
      else {
        F14_LandingHandler(2);
      }
      break;
      

  }
  if (APC_settings[DebugMode]){
    Serial.println("-> Post handle");
    Serial.print(" -> Lock 1 status = ");
    Serial.println((byte) F14_LockStatus[Player][0]);
    Serial.print(" -> Lock 2 status = ");
    Serial.println((byte) F14_LockStatus[Player][1]);
    Serial.print(" -> Lock 3 status = ");
    Serial.println((byte) F14_LockStatus[Player][2]);
    Serial.print(" -> Lock 1 occupied = ");
    Serial.println((byte) F14_LockOccupied[0]);
    Serial.print(" -> Lock 2 occupied = ");
    Serial.println((byte) F14_LockOccupied[1]);
    Serial.print(" -> Lock 3 occupied = ");
    Serial.println((byte) F14_LockOccupied[2]);
    Serial.print(" -> Landing 1 status = ");
    Serial.println((byte) F14_LandingStatus[Player][0]);
    Serial.print(" -> Landing 2 status = ");
    Serial.println((byte) F14_LandingStatus[Player][1]);
    Serial.print(" -> Landing 3 status = ");
    Serial.println((byte) F14_LandingStatus[Player][2]);
    Serial.print(" -> InLock = ");
    Serial.println((byte) InLock);
    Serial.print(" -> MultiBalls = ");
    Serial.println((byte) Multiballs);
    
  }
 
}

// Handle what happens during multiball when a ball reaches a lock.
// This could be included in the lock handler but that code is already
// complicated enough :)
void F14_LandingHandler(byte Lock) {
int landing_bonus;
byte landing_count=0;

  if (APC_settings[DebugMode]) {                      // activate serial interface in debug mode
    Serial.print("F14_LandingHandler for lock, pre status ");
    Serial.println((byte)Lock);
    Serial.print("Landing 1 = ")  ;
    Serial.println((byte)F14_LandingStatus[Player][0]);
    Serial.print("Landing 2 = ")  ;
    Serial.println((byte)F14_LandingStatus[Player][1]);
    Serial.print("Landing 3 = ")  ;
    Serial.println((byte)F14_LandingStatus[Player][2]);

  }

  // the first ball into a landing is actually still the multiball starting up, it's the ball that was in the vuk
  // so we don't score a landing for the first one.
  if (!F14_LocksClearing) { 
    // Set the status on this one
    F14_LandingStatus[Player][Lock]=1;

    // Work out how many landed  
    for (byte i=0; i<3; i++) {
      if (F14_LandingStatus[Player][i]==1) {
        landing_count++;
      }
    }

    // Work out the bonus
    if (landing_count==1) {
      landing_bonus = 150000;
    }
    else if (landing_count==2) {
      landing_bonus = 250000;
    }
    else {
      landing_bonus = 703850;
      for (byte i=0; i<3; i++) {
        F14_LandingStatus[Player][i]=0;
      }
    }

    Points[Player]+=landing_bonus;

    // Sort out the lamps
    
    F14_LockLampHandler();
  }
  else {
    F14_LocksClearing = 0;  // locks are now cleared.
  }

  // Kick the ball back out
  switch (Lock) {
    case 0:
      ActivateTimer(1000,10,F14_ActivateSolenoid);
      break;
    case 1:
      ActivateTimer(1000,5,F14_ActivateSolenoid);
      break;
    case 2:
      ActivateTimer(1000,7,F14_ActivateSolenoid);
      break;
  }

  if (APC_settings[DebugMode]) {                      // activate serial interface in debug mode
    Serial.print("F14_LandingHandler for lock, post status ");
    Serial.println((byte)Lock);
    Serial.print("Landing 1 = ")  ;
    Serial.println((byte)F14_LandingStatus[Player][0]);
    Serial.print("Landing 2 = ")  ;
    Serial.println((byte)F14_LandingStatus[Player][1]);
    Serial.print("Landing 3 = ")  ;
    Serial.println((byte)F14_LandingStatus[Player][2]);

  }


}


//     ___________   ___                     __ __             ____       
//    / __<  / / /  / _ )___  ___  __ _____ / // /__ ____  ___/ / /__ ____
//   / _/ / /_  _/ / _  / _ \/ _ \/ // (_-</ _  / _ `/ _ \/ _  / / -_) __/
//  /_/  /_/ /_/__/____/\___/_//_/\_,_/___/_//_/\_,_/_//_/\_,_/_/\__/_/   
//            /___/                                                       

// Handle bonus and bonus multiplier
// Event 0 - increment bonus
// Event 1 - increment multiplier
// Event 2 - update bonus and multiplier lamps
void F14_BonusHandler(byte Event){

  
  switch(Event) {
    case BONUS_INCREMENT:
      if (F14_Bonus < 127) {  // max is 127, playfield can't display more
        F14_Bonus++;
        F14_BonusHandler(BONUS_LAMP_REFRESH);  // update the lamps
      }
      break;
    case BONUS_MULT_INCREMENT:
      if (F14_Multiplier < 8) {
        F14_Multiplier++;
        F14_BonusHandler(BONUS_LAMP_REFRESH);  // update the lamps
      }
      break;
    case BONUS_LAMP_REFRESH:
      if (F14_Bonus % 2)
        TurnOnLamp(17);
      else
        TurnOffLamp(17);
      if (F14_Bonus % 4 > 1)
        TurnOnLamp(18);
      else 
        TurnOffLamp(18);
      if (F14_Bonus % 8 > 3)
        TurnOnLamp(19);
      else
        TurnOffLamp(19);
      if (F14_Bonus % 16 > 7)
        TurnOnLamp(20);
      else
        TurnOffLamp(20);
      if (F14_Bonus % 32 > 15)
        TurnOnLamp(21);
      else
        TurnOffLamp(21);
      if (F14_Bonus % 64 > 31)
        TurnOnLamp(22);
      else  
        TurnOffLamp(22);
      if (F14_Bonus > 63)
        TurnOnLamp(23);
      else  
        TurnOffLamp(23);
      for (byte i=1; i<8; i++) {
        if (F14_Multiplier > i)
          TurnOnLamp(24+i);
        else
          TurnOffLamp(24+i);
      }
      break;
  }
}


//     ___________   __   _          ____  ______           __  __   __ __             ____       
//    / __<  / / /  / /  (_)__  ___ / __ \/ _/ _ \___ ___ _/ /_/ /  / // /__ ____  ___/ / /__ ____
//   / _/ / /_  _/ / /__/ / _ \/ -_) /_/ / _/ // / -_) _ `/ __/ _ \/ _  / _ `/ _ \/ _  / / -_) __/
//  /_/  /_/ /_/__/____/_/_//_/\__/\____/_//____/\__/\_,_/\__/_//_/_//_/\_,_/_//_/\_,_/_/\__/_/   
//            /___/                                                                               

// Handle the line of death kickback
// Event 0 - switch activated, so kick the ball back and increment the kills
// Event 1 - award kill without firing kickback
// Event 2 - just light the correct lamps based on the kill count (when switching players for example)
void F14_LineOfDeathHandler(byte Event) {
   if (APC_settings[DebugMode]){
    Serial.print("F14_LineOfDeathHandler event ");            // print address reference table
    Serial.println((byte)Event);
  }

  static int kill_step = 0;
  static int kill_loop = 0;
  switch (Event){
    case LINE_OF_DEATH_HIT:
      ActivateSolenoid(0, 12);  // fall through to case 1 is correct in this case
    case AWARD_KILL: 
      if (F14_ExtraBallLit[Player]) {
        F14_AwardExtraBall();
        F14_ExtraBallLit[Player] = 0;
        TurnOffLamp(64);
      }
      if (F14_Kills[Player]==7)
        return;
      F14_Kills[Player]++;
      //TurnOnLamp(101+F14_Kills[Player]);
      WriteUpper2("              ");
      WriteLower2("              ");
      SwitchDisplay(0);  // use the second buffer
      Points[Player] += 50000;
      switch (F14_Kills[Player]) {
        case 1:
          WriteUpper2(" ALPHA  KILL  ");  
          break;
        case 2:
          WriteUpper2(" BRAVO  KILL  ");  
          break;
        case 3:
          WriteUpper2("CHARLIE KILL  ");  
          break;
        case 4:
          WriteUpper2(" DELTA  KILL  ");  
          break;
        case 5:
          WriteUpper2("  ECHO  KILL  ");  
          break;
        case 6:
          WriteUpper2("  FOX   KILL  ");  
          break;
        case 7:
          WriteUpper2("  GOLF  KILL  ");  
          if (!ExBalls) {
            F14_ExtraBallLit[Player]=1;
          }
          break;
      }
      WriteLower2("              ");
      kill_step = 0;
      kill_loop = 0;
      F14_LineOfDeathHandler(2);  // update the lamps
      ActivateTimer(200,LINE_OF_DEATH_STROBE,F14_LineOfDeathHandler);
      break;
    case RESET_KILL_LAMPS:
      for (byte i=1; i < 8; i++) {
        if (i > F14_Kills[Player])
          TurnOffLamp(i+101);
        else
          TurnOnLamp(i+101);
      }
      // inlanes lite kill shot if Yagov not completed once
      if (F14_YagovKills[Player]==0) {
        TurnOnLamp(39); // both inlanes
      }
      else {
        TurnOffLamp(39);
      }
      if (F14_ExtraBallLit[Player]) {
        TurnOnLamp(64);
      }
      else {
        TurnOffLamp(64);
      }
      break;
    case LINE_OF_DEATH_STROBE:
      switch(kill_step) {
        case 0:
          WriteLower2("      2 5       ");
          break;
        case 1:
          WriteLower2("     22 55      ");
          break;
        case 2:
          WriteLower2("    222 555     ");
          break;
        case 3:
          WriteLower2("   222   555    ");
          break;
        case 4:
          WriteLower2("  222     555   ");
          break;
        case 5:
          WriteLower2(" 222       555  ");
          break;              
        case 6:
          WriteLower2("222         555 ");
          break;              
        case 7:
          WriteLower2("22           55 ");
          break;              
        case 8:
          WriteLower2("2             5 ");
          break;              
        case 9:
          WriteLower2("                ");
          break;
      }
      kill_step++;
      if (kill_step==10) {
        kill_loop++;
        kill_step=0;
      }
      if (kill_loop<5) {
        ActivateTimer(40,LINE_OF_DEATH_STROBE,F14_LineOfDeathHandler);
      }
      else {
        SwitchDisplay(1);  // all done, back to normal
      }
  }
}

// Simple handler for the inlane -> kill target
// Event 0 - light the centre kill after inlane active
// Event 1 - centre kill hit
// Event 2 - timer expired
void F14_CentreKillHandler(byte Event) {
  static byte centre_kill_timer=0;

  switch (Event) {
    case 0:
      if (!centre_kill_timer) {
        AddBlinkLamp(109,100);
        centre_kill_timer = ActivateTimer(1500, 2, F14_CentreKillHandler);
      }
      break;
    case 1:
      if (centre_kill_timer) {
        F14_LineOfDeathHandler(AWARD_KILL); // award kill without firing kickback
        F14_CentreKillHandler(2); // shut this down, only awarded once
      }
      break;
    case 2:
      if (centre_kill_timer) {
        centre_kill_timer = 0;
        RemoveBlinkLamp(109);
      }
  }
}



//     ___________   __                      __   ___                     __ __             ____       
//    / __<  / / /  / /  ___ ___ _____  ____/ /  / _ )___  ___  __ _____ / // /__ ____  ___/ / /__ ____
//   / _/ / /_  _/ / /__/ _ `/ // / _ \/ __/ _ \/ _  / _ \/ _ \/ // (_-</ _  / _ `/ _ \/ _  / / -_) __/
//  /_/  /_/ /_/__/____/\_,_/\_,_/_//_/\__/_//_/____/\___/_//_/\_,_/___/_//_/\_,_/_//_/\_,_/_/\__/_/   
//            /___/                                                                                    

// The launch bonus is awarded when a left inlane is followed by a shot to the vuk
// within 2 seconds.  During this time the bonus lamps strobe.
// Event 0 - enable the bonus
// Event 1 - score the bonus if applicable
// Event 2 - timeout
// Event 3 - run the lamp effect
// Event 4 - reset between players
void F14_LaunchBonusHandler(byte Event) {
  static byte bonus_enabled = 0;
  static byte strobe_step = 0;
  static byte strobe_loop = 0;
  static byte strobe_timer = 0;
  static int current_bonus = 50000;
  
  if (APC_settings[DebugMode] && Event != 3){
    Serial.print("F14_LaunchBonusHandler event ");            // print address reference table
    Serial.println((byte)Event);
  }


  switch (Event) {
    case START_HANDLER:
      if (bonus_enabled) {
        break;
      }
      bonus_enabled = 1;
      for (byte i=0; i<8; i++) {
        TurnOffLamp(17+i); // turn off the bonus lamps
      }
      strobe_loop=0;
      strobe_step=0;
      strobe_timer = ActivateTimer(100,LAUNCH_BONUS_LAMP_STROBE,F14_LaunchBonusHandler);
      
      break;
    case LAUNCH_BONUS_SCORE:
      if (bonus_enabled) {
        if (F14_LaunchBonus < 50) {
          F14_LaunchBonus += 5;
        }
        Points[Player] += F14_LaunchBonus * 10000;
        F14_AnimationHandler(ANIMATION_LAUNCH_BONUS,ANIMATION_START);  // Launch bonus animation
        strobe_loop = 10;  // this will shut down the timer
      }
      else { // no bonus, just call back to vuk handler
        //ActivateTimer(10,VUK_CALL_FROM_LAUNCH_BONUS,F14_vUKHandler);
        F14_vUKHandler(VUK_CALL_FROM_LAUNCH_BONUS);
      }
      break;
    case QUIT_HANDLER:
      bonus_enabled = 0;
      strobe_timer = 0;
      F14_BonusHandler(BONUS_LAMP_REFRESH);  // reset the bonus lamps after we've been strobing them
      F14_LockLampHandler(); // and the release/lock lamps
      break;
    case LAUNCH_BONUS_LAMP_STROBE:
      switch (strobe_step) {  // a little work could remove this monster case statement and calculate it
        case 0:
          TurnOnLamp(17);
          break;
        case 1:
          TurnOnLamp(18);
          break;
        case 2:
          TurnOffLamp(17);
          TurnOnLamp(19);
          break;
        case 3:
          TurnOffLamp(18);
          TurnOnLamp(20);
          break;
        case 4:
          TurnOffLamp(19);
          TurnOnLamp(21);
          break;
        case 5:
          TurnOffLamp(20);
          TurnOnLamp(22);
          break;
        case 6:
          TurnOffLamp(21);
          TurnOnLamp(23);
          break;
        case 7:
          TurnOffLamp(22);
          TurnOnLamp(48);
          break;
        case 8:
          TurnOffLamp(23);
          TurnOnLamp(111);
          break;
        case 9:
          TurnOffLamp(48);
          TurnOnLamp(40);
          break;
        case 10:
          TurnOffLamp(111);
          break;
        case 11:
          TurnOffLamp(40);
          break;
      }
      strobe_step++;
      if (strobe_step == 12)   {
        strobe_step=0;
        strobe_loop++;
      }
      if (strobe_loop>9) {
        F14_LaunchBonusHandler(QUIT_HANDLER); // switch it off
      }
      else {
        strobe_timer = ActivateTimer(20,LAUNCH_BONUS_LAMP_STROBE,F14_LaunchBonusHandler);
      }
      break;
    case LAUNCH_BONUS_RESET:
      F14_LaunchBonus = 0;
      break;
  }
}


//     ___________   ____     _                   __ __             ____       
//    / __<  / / /  / __/__  (_)__  ___  ___ ____/ // /__ ____  ___/ / /__ ____
//   / _/ / /_  _/ _\ \/ _ \/ / _ \/ _ \/ -_) __/ _  / _ `/ _ \/ _  / / -_) __/
//  /_/  /_/ /_/__/___/ .__/_/_//_/_//_/\__/_/ /_//_/\_,_/_//_/\_,_/_/\__/_/   
//            /___/  /_/                                                       
// Small handler for the spinner
// Event 0 = spinner hit
// Event 1 = light spinner 2k
// Event 2 = reset spinner 2k


void F14_SpinnerHandler(byte Event) {
  static byte spinner_2k_lit=0;

  switch (Event) {
    case SPINNER_HIT:
      if (spinner_2k_lit) {
        Points[Player] += 2000;
      }
      else {
        Points[Player] += 10;
      }
      ShowPoints(Player);
      PlaySound(49,"0_C1.snd");
      break;
    case SPINNER_LIGHT:
      spinner_2k_lit = 1;
      TurnOnLamp(56);
      break;
    case SPINNER_RESET:
      spinner_2k_lit = 0;
      TurnOffLamp(56);
      break;
  }
}

// Divertor handler
// Called when a ball is about to enter the ramp with the 2 diverters
// Called by the switch just above the shooter lane and also the vUK eject
// Decides where to route the ball based on the status of the locks
// Event 0 - incoming ball
void F14_DivertorHandler(byte Event) {
  byte destination_lock;

  
  switch (Event) {
    case 0:
      // If multiball is running, then we need to select a lock which has landing available
      if (Multiballs > 1) {
        if (F14_LandingStatus[Player][0]==0) {
          destination_lock = 0;
        }
        else if (F14_LandingStatus[Player][1] == 0) {
          destination_lock = 1;
        }
        else {
          destination_lock = 2;
        }
      }
      else { // no multiball, pick a lit lock first, else unlit, else whatever
        if (F14_LockStatus[Player][0]==1) {
          destination_lock = 0;
        }
        else if (F14_LockStatus[Player][1]==1) {
          destination_lock = 1;
        }
        else if (F14_LockStatus[Player][2]==1) {
          destination_lock = 2;
        }
        else if (F14_LockStatus[Player][0]==0) {
          destination_lock = 0;
        }
        else if (F14_LockStatus[Player][1]==0) {
          destination_lock = 1;
        }
        else if (F14_LockStatus[Player][2]==0) {
          destination_lock = 2;
        }

        else {
          destination_lock = random(3);
        } 
      }
      if (APC_settings[DebugMode]){
        Serial.print("F14_DivertorHandler chose destination ");            // print address reference table
        Serial.println((byte)destination_lock);
      }
  

      switch (destination_lock) {
        case 0:
          ActivateSolenoid(2000,22);
          break;
        case 1:
          ActivateSolenoid(2000,21);
          break;
        case 2:
          break;
      }

  }

}

// Handle one of the TOMCAT targets being hit
void F14_TomcatTargetHandler(byte Target) {
  byte lit_target_count = 0;

  // Target is still flashing, so score it
  if (F14_TomcatTargets[Player][Target] == 0) {
    Points[Player] += 1000;
    F14_BonusHandler(BONUS_INCREMENT);  // increment the end of ball bonus
    F14_TomcatTargets[Player][Target]  = 1;   // Mark the target as hit
      if (Target < 6) {  // and the corresponding one on the other half of playfield
        F14_TomcatTargets[Player][Target+6] = 1;
      }
      else {
        F14_TomcatTargets[Player][Target-6] = 1;
      }
      // How many are lit at the moment?
      for (byte i = 0; i < 12; i++) {
        if (F14_TomcatTargets[Player][i]==1) {
          lit_target_count++;
        }
      }
      // If all targets hit, reset them (need to add lock logic here sometime)
      if (lit_target_count == 12){
        for (byte j=0; j<12; j++) {
          F14_TomcatTargets[Player][j]=0;}
        if (Multiballs==1) { // not during multiball
          F14_AnimationHandler(ANIMATION_LOCK_IS_LIT, ANIMATION_START);
          F14_LockHandler(0); // Tell the lock handler we can light another lock
          F14_TomcatsCompleted[Player] += 1;
          if (F14_TomcatsCompleted[Player] == 1) {
            F14_LockHandler(0);  // for the first completion only, light another lock
          }
        }
      }
      F14_TomcatTargetLamps();
      
  }
  else { // target has already been hit, just score 100
    Points[Player] += 100;
  }
  ShowPoints(Player);
}

// Handler for the 1-6 targets in the centre of the playfield.  
// These just light sequentially 6 -> 1 -> 6
// If the lit one is hit, scores 10000, otherwise 500
// 
// Event 1 - start the light movement
// Event 2 - stop the light movement
// Event 3 - move to the next light
// Events 41 - 46 switch hit
// Routine indexes lamps / targets from 0-5 (to represent 1-6)
void F14_1to6Handler(byte Event) {
  static byte one_to_six_timer = 0;
  static byte current_lamp=0;
  static int direction=1;


  switch (Event) {
    case 1:
      F14_1to6Handler(3);
      break;
    case 2:
      if (one_to_six_timer) {
        KillTimer(one_to_six_timer);
        one_to_six_timer = 0;
      }
      for (byte i=0; i<6; i++) {
        TurnOffLamp(F14_1to6LampNumbers[i]);
      }
      break;
    case 3:
      //if (APC_settings[DebugMode]) {
      //  Serial.print("1 to 6 lamp number");
      //  Serial.println(current_lamp);
     // }
      TurnOffLamp(F14_1to6LampNumbers[current_lamp]);
      current_lamp += direction;
      if (current_lamp == 0 or current_lamp == 5) {
        direction = direction * -1;
      }
      TurnOnLamp(F14_1to6LampNumbers[current_lamp]);
      one_to_six_timer = ActivateTimer(250,3,F14_1to6Handler);
      break;
    // switch hit
    case 41:
    case 42:
    case 43:
    case 44:
    case 45:
    case 46:
      if (current_lamp == F14_1to6SwitchNumbers[Event-41]) {
        Points[Player] += 10000;
        F14_SpinnerHandler(SPINNER_LIGHT); // increase spinner score to 2k
        F14_OrbitHandler(ORBIT_LIGHT_BOTH_SIDE_BONUS);  // light left and right orbit bonusx
      }
      else {
        Points[Player] += 500;
      }
      ShowPoints(Player);
      break;
  }
  
}

// Some shots will spot a TOMCAT letter, this routine will find an unlit
// one and call the handler to light it
byte F14_SpotTomcat() {
  // Find the first unlit target, there will always be 1 as all 12 are never lit
  byte spot_target;
  for (spot_target=0; spot_target<12; spot_target++){
    if (F14_TomcatTargets[Player][spot_target]==0)
      break;
  }
  F14_TomcatTargetHandler(spot_target);
  if (APC_settings[DebugMode]) {
    Serial.print("F14_SpotTomcat spotted ");
    Serial.println((byte) spot_target); }
}




//     ___________  ____      __   _ __  __ __             ____       
//    / __<  / / / / __ \____/ /  (_) /_/ // /__ ____  ___/ / /__ ____
//   / _/ / /_  _// /_/ / __/ _ \/ / __/ _  / _ `/ _ \/ _  / / -_) __/
//  /_/  /_/ /_/__\____/_/ /_.__/_/\__/_//_/\_,_/_//_/\_,_/_/\__/_/   
//            /___/                                                   

// Orbit handler
// Event 0 - ball hit right switch
// Event 1 - ball hit left switch
// Event 2 - light bonus X right side
// Event 3 - light bonus X left side
// Event 4 - light bonus X both sides
// Event 6 - bonus X times out
// Event 7 - kill any running timers
// Event 8 - reset the orbit (end of ball)
// Operation of the orbit...
// Ball travelling from left to right will light left bonus x lamp if loop completed.
// Ball travelling from right to left will light right bonux x lamp if loop completed.
// Hitting the lit 1-6 target will light both bonus x lamps
// If loop is shot with bonus x lamp lit, bonus multiplier will increase
// Bonus x lamps stay lit for 5 seconds, then flash for 2 seconds (grace period), then deactivate
void F14_OrbitHandler(byte Event) {
  static byte right_bonusX_lit = 0;
  static byte left_bonusX_lit = 0;
  static byte anti_clock_timer = 0;
  static byte clockwise_timer = 0;
  static byte orbit_bonusx_timer = 0;

  /*if (APC_settings[DebugMode]) {
    Serial.print("Orbit Event = ");
    Serial.println(Event);
    Serial.print("Anti Clock Timer = ");
    Serial.println(anti_clock_timer);
    Serial.print("Clockwise Timer = ");
    Serial.println(clockwise_timer);
    Serial.print("Orbit right lit = ");
    Serial.println(right_bonusX_lit);
    Serial.print("Orbit left lit = ") ;
    Serial.println(left_bonusX_lit);
    
  }*/
  switch (Event) {
    // Switch at right of orbit hit
    // If the ball was travelling from left to right, the loop has been made
    // If loop made and left bonus X was lit, increase multiplier
    // If loop made and left bonus X not lit, light it
    case ORBIT_RIGHT_SWITCH_HIT:  // switch at right side of orbit hit
      if (clockwise_timer) {
        KillTimer(clockwise_timer);
        clockwise_timer = 0;
        if (left_bonusX_lit) {
          F14_BonusHandler(BONUS_MULT_INCREMENT);
          F14_HotStreakHandler(0);  // enable hotstreak
          F14_OrbitHandler(ORBIT_MADE);  // extend the timer
        }
        else {
          F14_OrbitHandler(ORBIT_LIGHT_LEFT_SIDE_BONUS);
        }
      }
      else if (anti_clock_timer==0) {
        anti_clock_timer = ActivateTimer(1500, ORBIT_ANTI_CLOCK_TIMEOUT, F14_OrbitHandler);
      }

      break;
    case ORBIT_LEFT_SWITCH_HIT:  // switch at left side hit
      if (anti_clock_timer) {
        KillTimer(anti_clock_timer);
        anti_clock_timer = 0;
        if (right_bonusX_lit) {
          F14_BonusHandler(BONUS_MULT_INCREMENT);  // bump the multiplier
          F14_HotStreakHandler(0); // enable hotstreak
          F14_OrbitHandler(ORBIT_MADE); // extend the timer
        }
        else {
          F14_OrbitHandler(ORBIT_LIGHT_RIGHT_SIDE_BONUS);
        }
      }
      else if (clockwise_timer==0) {
        clockwise_timer = ActivateTimer(1500, ORBIT_CLOCK_TIMEOUT, F14_OrbitHandler);
      }
      break;
    case ORBIT_LIGHT_RIGHT_SIDE_BONUS:
      right_bonusX_lit = 1;
      AddBlinkLamp(55,150);
      if (orbit_bonusx_timer) {
        KillTimer(orbit_bonusx_timer);
        orbit_bonusx_timer = 0;
      }
      orbit_bonusx_timer = ActivateTimer(8000, ORBIT_BONUSX_TIMED_OUT, F14_OrbitHandler);
      break;
    case ORBIT_LIGHT_LEFT_SIDE_BONUS:
      left_bonusX_lit = 1;
      AddBlinkLamp(32,150);
      if (orbit_bonusx_timer) {
        KillTimer(orbit_bonusx_timer);
        orbit_bonusx_timer = 0;
      }
      orbit_bonusx_timer = ActivateTimer(8000, ORBIT_BONUSX_TIMED_OUT, F14_OrbitHandler);
      
      break;
    case ORBIT_LIGHT_BOTH_SIDE_BONUS:
      left_bonusX_lit = 1;
      right_bonusX_lit = 1;
      AddBlinkLamp(32,150);
      AddBlinkLamp(55,150);
      if (orbit_bonusx_timer) {
        KillTimer(orbit_bonusx_timer);
        orbit_bonusx_timer = 0;
      }
      orbit_bonusx_timer = ActivateTimer(8000, ORBIT_BONUSX_TIMED_OUT, F14_OrbitHandler);
      break;
    case ORBIT_BONUSX_TIMED_OUT: // timed out
      RemoveBlinkLamp(32);
      RemoveBlinkLamp(55);
      left_bonusX_lit = 0;
      right_bonusX_lit = 0;
      orbit_bonusx_timer = 0;
      break;
    case ORBIT_MADE:  // orbit made while bonusx lit, reset the timer
      if (orbit_bonusx_timer) {
        KillTimer(orbit_bonusx_timer);
        orbit_bonusx_timer = 0;
      }
      orbit_bonusx_timer = ActivateTimer(5000, ORBIT_BONUSX_TIMED_OUT, F14_OrbitHandler);
    case ORBIT_RESET:  // reset the orbit handler (new ball for example)
      if (orbit_bonusx_timer) {
        KillTimer(orbit_bonusx_timer);
        orbit_bonusx_timer = 0;
      }
      if (clockwise_timer) {
        KillTimer(clockwise_timer);
        clockwise_timer = 0;
      }
      if (anti_clock_timer) {
        KillTimer(anti_clock_timer);
        anti_clock_timer = 0;
      }
      left_bonusX_lit=0;
      right_bonusX_lit=0;
      RemoveBlinkLamp(32);
      RemoveBlinkLamp(55);
      break;
    case ORBIT_ANTI_CLOCK_TIMEOUT:
      anti_clock_timer = 0;
      break;
    case ORBIT_CLOCK_TIMEOUT:
      clockwise_timer = 0;
      break;
  }
}


//     ___________   ___                     ______                  __  __ __             ____       
//    / __<  / / /  / _ \___ ___ ______ ____/_  __/__ ________ ____ / /_/ // /__ ____  ___/ / /__ ____
//   / _/ / /_  _/ / , _/ -_|_-</ __/ // / -_) / / _ `/ __/ _ `/ -_) __/ _  / _ `/ _ \/ _  / / -_) __/
//  /_/  /_/ /_/__/_/|_|\__/___/\__/\_,_/\__/_/  \_,_/_/  \_, /\__/\__/_//_/\_,_/_//_/\_,_/_/\__/_/   
//            /___/                                      /___/                                        

// regular START_HANDLER 0 = start the flip/flop between rescue targets
// regular QUIT_HANLDER  255 = stop the flip/flop


void F14_RescueTargetHandler(byte Event){
  static byte lit_target = 0;  // 0 is left target, 1 is right target
  static byte rescue_target_timer = 0;
  switch(Event) {
    case START_HANDLER:
      F14_RescueTargetHandler(RESCUE_TARGET_LEFT_LIGHT);  // just call the start for the left lamp
      break;
    case QUIT_HANDLER:  // kill the timer and switch the lamps off
      if (rescue_target_timer) {
          KillTimer(rescue_target_timer);
          rescue_target_timer = 0;
        }
        TurnOffLamp(5);
        TurnOffLamp(7);
      break;
    case RESCUE_TARGET_LEFT_HIT:
      if (lit_target==0){
        F14_RescueKickerHandler(START_HANDLER); // left target hit and is lit, call the kicker handler
        PlaySound(50, "0_C1.snd");
      }
      else {
        PlaySound(50, "0_C0.snd");
      }
      break;
    case RESCUE_TARGET_RIGHT_HIT:
      if (lit_target==1){
        F14_RescueKickerHandler(START_HANDLER); // right target hit and is lit, call the kicker handler      
        PlaySound(50, "0_C1.snd");
      }
      else {
        PlaySound(50, "0_C0.snd");
      }
      break;
    case RESCUE_TARGET_LEFT_LIGHT:  // Light left target
      lit_target = 0;
      TurnOnLamp(5);
      TurnOffLamp(7);
      rescue_target_timer = ActivateTimer(1000, RESCUE_TARGET_RIGHT_LIGHT, F14_RescueTargetHandler);  // move to right in 1 sec
      break;
    case RESCUE_TARGET_RIGHT_LIGHT:  // Light right target
      lit_target = 1;
      TurnOnLamp(7);
      TurnOffLamp(5);
      rescue_target_timer = ActivateTimer(1000, RESCUE_TARGET_LEFT_LIGHT, F14_RescueTargetHandler);  // move to left in 1 sec
      break;

  }
}


//     ___________   ___                       __ ___     __           __ __             ____       
//    / __<  / / /  / _ \___ ___ ______ _____ / //_(_)___/ /_____ ____/ // /__ ____  ___/ / /__ ____
//   / _/ / /_  _/ / , _/ -_|_-</ __/ // / -_) ,< / / __/  '_/ -_) __/ _  / _ `/ _ \/ _  / / -_) __/
//  /_/  /_/ /_/__/_/|_|\__/___/\__/\_,_/\__/_/|_/_/\__/_/\_\\__/_/ /_//_/\_,_/_//_/\_,_/_/\__/_/   
//            /___/                                                                                 

// Handler for the rescue kickback (left outlane)
// regular START_HANDLER 0 - light the kickback (called from F14_RescueTargetHandler) and on ball start

// Event 1 - ball hit outlane switch
// Event 2 - grace period expires
void F14_RescueKickerHandler(byte Event){
  static byte grace_timer=0;
  static byte F14_RescueKickerStatus = 0;  // 0 not lit, 1 lit, 2 grace period
  switch (Event){
    case START_HANDLER:  // light kickback
      if (grace_timer) {  // kill the grace timer if we have one
        KillTimer(grace_timer);
        grace_timer = 0;
      }
      F14_RescueKickerStatus = 1;
      TurnOnLamp(8);  // switch on the outlane lamp
      break;
    case RESCUE_OUTLANE_SWITCH_HIT:  // ball in outlane
      switch (F14_RescueKickerStatus){
        case 1:
          ActivateSolenoid(0,13);  // Fire the kicker
          if (random(2)==0) {  // "That was close" or "Pull Up"
            PlaySound(51, "0_B7.snd");
          }
          else {
            PlaySound(51, "0_BB.snd");
          }
          F14_RescueKickerStatus = 2;
          TurnOffLamp(8);  // Turn the lamp off
          AddBlinkLamp(8,100);  // then blink it
          grace_timer = ActivateTimer(2000, RESCUE_GRACE_PERIOD_TIMEDOUT, F14_RescueKickerHandler);  // cancel in 2 secs
          break;
        case 2:
          ActivateSolenoid(0,13);  // if in grace period just fire kicker again
          break;
      }
      break;
    case RESCUE_GRACE_PERIOD_TIMEDOUT:  // time is up
      grace_timer = 0;
      RemoveBlinkLamp(8);
      F14_RescueKickerStatus = 0;
      break;
  }
}


//     ___________       __  ____ ____ __             ____       
//    / __<  / / / _  __/ / / / //_/ // /__ ____  ___/ / /__ ____
//   / _/ / /_  _/| |/ / /_/ / ,< / _  / _ `/ _ \/ _  / / -_) __/
//  /_/  /_/ /_/__|___/\____/_/|_/_//_/\_,_/_//_/\_,_/_/\__/_/   
//            /___/                                              

// This handles various things for the vUK (popper top right)
// The incoming event will specify what the routine is being called for

// 0 = ball landed in kicker
// 1 = play animation ahead of eject
// 2 = eject ball
void F14_vUKHandler(byte Event) {
  
  byte ball_to_be_locked = 0;
  byte multiball_to_start = 0;
  static byte noisy_switch_timer = 0;

  
  
  if (APC_settings[DebugMode]){
    Serial.print("F14_vUKHandler event ");            // print address reference table
    Serial.println((byte)Event);
  }

  
  switch (Event) {
    // ball has landed in kicker.  If it's going to be sent to a lock, we need to do the WEAPONS etc animation
    // before ejecting.  Also need to award launch bonus if needed
    case VUK_BALL_CAPTURED:             // will need expanding when lock logic coded
      if (noisy_switch_timer) {
        if (APC_settings[DebugMode]){
          Serial.println("F14_vUKHandler ignoring noisy switch");
        }   
        return;
      }
      else {
        noisy_switch_timer = ActivateTimer(100,VUK_NOISY_SWITCH_TIMEOUT,F14_vUKHandler);
      }
      if (Multiballs==1) { // single ball play
        F14_LaunchBonusHandler(LAUNCH_BONUS_SCORE);  // award the launch bonus if applicable
                                    // that will callback here with event 3 to continue
        F14_BonusHandler(BONUS_INCREMENT); // increment end of ball bonus
        F14_BonusHandler(BONUS_INCREMENT); // increment end of ball bonus (again for 2k!)
        F14_SpotTomcat();
        F14_LampShowPlayer(0,0);
      }
      else { // in multiball
        F14_AnimationHandler(ANIMATION_SAFE_LANDING,ANIMATION_START); // safe landing, no callback on that one, it just plays
        ActivateTimer(2000,VUK_PLAY_FLASHERS,F14_vUKHandler);
        // play some fancy animation with callback to launch ball
      }
      break;
    case VUK_PLAY_FLASHERS:  // eject the ball
      PlayFlashSequence((byte *)F14_LockedOnSeq);
      
      ActivateTimer(1000,VUK_EJECT,F14_vUKHandler);
      break;
    case VUK_EJECT:  // Timer over, send the ball on its way
      F14_LampShowPlayer(0,255);
      noisy_switch_timer = ActivateTimer(100,VUK_NOISY_SWITCH_TIMEOUT,F14_vUKHandler);  // handle noisy switch on eject
      ActA_BankSol(3);
      F14_DivertorHandler(0);   // let the divertor know a ball is on the way
      
      break;
    case VUK_CALL_FROM_LAUNCH_BONUS:  // continue running after launch bonus
        // work out if the ball is going to be locked (at least one lock with status 1)
        for (byte i=0; i<3; i++) {
          if (F14_LockStatus[Player][i]==1) {
            ball_to_be_locked=1;
          }
        }
        // work out if multiball will kick off - all locks status 2
        if (F14_LockStatus[Player][0]==2  && F14_LockStatus[Player][1]==2 && F14_LockStatus[Player][2]==2) {
          multiball_to_start = 1;
        }

        if (ball_to_be_locked) {
          F14_LockIsLitAnimation(99); // Kill that animation, if running
          F14_AnimationHandler(ANIMATION_WEAPONS,ANIMATION_START); // WEAPONS SYSTEMS etc.  Will call back to vuk handler when done.
        }
        else if (multiball_to_start) {
          F14_LockHandler(8);
          
          
        }
        else {
          F14_vUKHandler(1);  // otherwise just kick the ball out.
        }
        break;
    case VUK_NOISY_SWITCH_TIMEOUT:
      noisy_switch_timer = 0;
      break;
  }
}


//     ___________   ___        _            __  _           __ __             ____       
//    / __<  / / /  / _ | ___  (_)_ _  ___ _/ /_(_)__  ___  / // /__ ____  ___/ / /__ ____
//   / _/ / /_  _/ / __ |/ _ \/ /  ' \/ _ `/ __/ / _ \/ _ \/ _  / _ `/ _ \/ _  / / -_) __/
//  /_/  /_/ /_/__/_/ |_/_//_/_/_/_/_/\_,_/\__/_/\___/_//_/_//_/\_,_/_//_/\_,_/_/\__/_/   
//            /___/                                                                       

// Central Handler for animations (display, flashers etc) 
// Animation = which one
// Status 0 = start, Status 1 = finished
// Not all animations care when they are finished, but sometimes we need to pause gameplay
// so those animations will call back for handling
void F14_AnimationHandler(byte Animation, byte Status) {

  if (APC_settings[DebugMode]){
    Serial.print("F14_AnimationHandler called with show ");            // print address reference table
    Serial.print((byte)Animation);
    Serial.print(" and status ");            // print address reference table
    Serial.println((byte)Status);
  }
  
  switch(Animation) {
    case ANIMATION_WEAPONS:
      if (Status == ANIMATION_START) {
        F14_WeaponsAnimation(ANIMATION_START);  // WEAPONS SYSTEMS etc
      }
      else {
        F14_vUKHandler(VUK_PLAY_FLASHERS);  // callback to vuk handler when animation is complete
      }
      break;
    case ANIMATION_BALL_LOCKED:
      F14_LockedBallAnimation(ANIMATION_START);   // Enemy X locked
      break;
    case ANIMATION_LOCK_IS_LIT:
      F14_LockIsLitAnimation(ANIMATION_START);
      break;
    case ANIMATION_START_MULTIBALL:
      if (Status==ANIMATION_START) {
        F14_MultiBallAnimation(ANIMATION_START);
      }
      else {
        F14_LockHandler(9); // tell the lock handler we're ready to go!
      }
      break;
    case ANIMATION_LAUNCH_BONUS:
      if (Status == ANIMATION_START) {
        F14_LaunchBonusAnimation(ANIMATION_START);
      }
      else {
        F14_vUKHandler(VUK_CALL_FROM_LAUNCH_BONUS);  // call back to vuk handler when animation complete.
      }
      break;
    case ANIMATION_SAFE_LANDING:
      F14_SafeLandingAnimation(ANIMATION_START);
      break;
  }
}



void F14_GIOn(byte Red, byte Green, byte Blue) {  // Colour not used at the moment
  //LEDsetColorMode(1);
  LEDsetColor(Red, Green, Blue);
  if (F14_GI_IsOn) {
    F14_SelectGILEDcolor(1);
  }
  else {
    for (int i=65; i < 102; i++) {
      TurnOnLamp(i);
      F14Show_TurnOnLamp(i);}
    //TurnOnLamp(115);
  }    
  
  F14_GI_IsOn = 1;
}

//void F14_GIChangeColour(byte Lamp) {
//  LEDchangeColor(Lamp);
//  if (Lamp < 102) {
//    ActivateTimer(17,Lamp+1,F14_GIChangeColour);
//  }
//}

void F14_GIOff() {
  /* if (APC_settings[DebugMode]){
    Serial.println("GI Off");
  }*/
  for (int i=65; i < 102; i++) {TurnOffLamp(i);}
  F14_GI_IsOn = 0;
}

byte LEDGIPattern[8] = {0b11111111,0b11111111,0b11111111,0b11111111,0b11111111,0b11111000,0b00000000,0b00000000};

void F14_SelectGILEDcolor(byte State) {              // Change the color of several LEDs without turning them on
  switch (State) {
  case 1:                                             // step 1
    LEDsetColorMode(4);                               // freeze LED states
    ActivateTimer(20, 2, F14_SelectGILEDcolor);      // wait 20ms for the refresh cycle to end
    break;
  case 2:                                             // step 2
    LEDsetColorMode(3);                               // LEDs will change their color without being turned on
    LEDpattern = LEDGIPattern;                     // apply pattern to specify which LEDs are affected
    ActivateTimer(20, 3, F14_SelectGILEDcolor);      // wait 20ms for the refresh cycle to end
    break;
  case 3:                                             // step 3
    LEDsetColorMode(4);                               // freeze LED states
    ActivateTimer(20, 4, F14_SelectGILEDcolor);      // wait 20ms for the refresh cycle to end
    break;
  case 4:
    LEDsetColorMode(1);                               // LEDs keep their color
    LEDinit();                                        // switch back to normal lamp pattern
    break;}}

void F14_ClearOuthole(byte Event) {
  UNUSED(Event);
  if (QuerySwitch(10)) { // outhole switch still active?
    if (!BlockOuthole && !C_BankActive) {             // outhole blocked?
      BlockOuthole = true;                            // block outhole until this ball has been processed
      ActivateSolenoid(30, 1); // put ball in trunk
      ActivateTimer(2000, 0, F14_BallEnd);}
    else {
      ActivateTimer(2000, 0, F14_ClearOuthole);}}}     // come back in 2s if outhole is blocked

void F14_HandleLock(byte Balls) {
      // do something with your lock
}

void F14_BallEnd(byte Event) {
  byte BallsInTrunk = F14_CountBallsInTrunk();
  if ((BallsInTrunk == 5)||(BallsInTrunk < 4+1-Multiballs-InLock)) {
    InLock = 0;
    if (Multiballs == 1) {
      for (i=0; i<3; i++) {                           // Count your locked balls here
        if (QuerySwitch[21+i]) {
          InLock++;}}}
    WriteLower(" BALL   ERROR   ");
    if (QuerySwitch(10)) { // ball still in outhole?
      ActA_BankSol(1); // make the coil a bit stronger
      ActivateTimer(2000, Event, F14_BallEnd);}        // and come back in 2s
    else {
      if (Event < 11) {                               // have I been here already?
        Event++;
        ActivateTimer(1000, Event, F14_BallEnd);}      // if not try again in 1s
      else {                                          // ball may be still in outhole
        BlockOuthole = false;
        Event = 0;
        F14_ClearOuthole(0);}}}
  else {
    switch (Multiballs) {
    case 4:                                           // goto 3 ball multiball
      Multiballs = 3;
      if (BallsInTrunk != 1) {                        // not 1 ball in trunk
        ActivateTimer(1000, 0, F14_BallEnd);}          // check again later
      else {
        BlockOuthole = false;}                        // remove outhole block
      break;
    case 3:                                           // goto 2 ball multiball
      Multiballs = 2;
      if (BallsInTrunk != 2) {                        // not 1 ball in trunk
        ActivateTimer(1000, 0, F14_BallEnd);}          // check again later
      else {
        BlockOuthole = false;}                        // remove outhole block
      break;
    
    case 2:                                           // end multiball
      Multiballs = 1;
      ReleaseSolenoid(16); // switch off the beacons
      F14_LockLampHandler(); // tidy up the lock lamps
      F14_LocksClearing=0; // if we were clearing locks, we're not now
      WriteLower("                ");
      if (BallsInTrunk == 4) { // all balls in trunk?
        ActivateTimer(1000, 0, F14_BallEnd);}
      else {
        BlockOuthole = false;}                        // remove outhole block
      break;
    case 1:                                           // end of ball
      if (BallsInTrunk + InLock != 4) {
        WriteUpper(" COUNT  ERROR   ");
        InLock = 0;
//        for (i=0; i<3; i++) {                       // check how many balls are on the ball ramp
//          if (Switch[41+i]) {
//            InLock++;}}
        ActivateTimer(1000, 0, F14_BallEnd);}
      else {
        BlinkScore(0);                                // stop score blinking
        F14_AwardBonus(AWARD_BONUS);
        StopPlayingMusic();
        //F14_BallEnd2(BallsInTrunk);                    // add bonus count here and start BallEnd2 afterwards
      }}}}

void F14_BallEnd2(byte Balls) {
  if (BallWatchdogTimer) {
    KillTimer(BallWatchdogTimer);
    BallWatchdogTimer = 0;}
  if (ExBalls) {                                      // Player has extra balls
    ExBalls--;
    TurnOffLamp(4);
    AddBlinkLamp(4,250);
    ActivateTimer(1000, 0 , F14_ExtraBallReleased);
    ActivateTimer(100, AppByte, F14_NewBall);
    BlockOuthole = false;}                            // remove outhole block
  else {                                              // Player has no extra balls
    if ((Points[Player] > HallOfFame.Scores[3]) && (Ball == APC_settings[NofBalls])) { // last ball & high score?
      Switch_Pressed = DummyProcess;                  // Switches do nothing
      F14_CheckHighScore(Player);}
    else {
      F14_BallEnd3(Balls);}}}

void F14_BallEnd3(byte Balls) {
  BlockOuthole = false;                               // remove outhole block
  if (Player < NoPlayers) {                           // last player?
    Player++;
    ActivateTimer(1000, Balls, F14_NewBall);}
  else {
    if (Ball < APC_settings[NofBalls]) {
      Player = 1;
      Ball++;
      ActivateTimer(1000, Balls, F14_NewBall);}
    else {                                            // game end
      ReleaseSolenoid(23);                            // disable flipper fingers
      ReleaseSolenoid(24);
      F14_CheckForLockedBalls(0);
      GameDefinition.AttractMode();}}}

void F14_AwardExtraBall() {
  ExBalls ++;
  TurnOnLamp(4);
}



void F14_ExtraBallReleased(byte Event) {
  if (QuerySwitch(16)) {                              // ball still in the shooting lane?
    ActivateTimer(2000, Event, F14_ExtraBallReleased);}  // come back in2s
  else {                                              // ball has been shot
    RemoveBlinkLamp(4);
    TurnOffLamp(4);
    if (ExBalls) {                                    // player still has an extra balls
      TurnOnLamp(4);}}}

// End of ball bonus
// Event 0 - award the bonus
// all other events are internal timers / loops for the display
// main display is in 3 sections
// 1 - display        BONUS 1234567
// 2 - display 1X 2X 3X etc
// 3 - count it down into the score
void F14_AwardBonus (byte Event) {
  static int total_bonus;
  static byte bonus_timer;
  static byte temp_bonus;

  switch (Event) {
    case AWARD_BONUS:
      total_bonus = F14_Bonus * F14_Multiplier * 1000;
      temp_bonus = F14_Bonus;
      WritePlayerDisplay((char *) "BONUS  ",1);
      DisplayScore(2, F14_Bonus * 1000);
      break;
    case 1:
      WritePlayerDisplay((char *) "   1X  ",1);
      if (F14_Multiplier == 1) {
        Event = 9;
      }
      break;
    case 2:
      WritePlayerDisplay((char *) "   2X  ",1);
      DisplayScore(2, F14_Bonus * 1000 * 2);
      if (F14_Multiplier == 2) {
        Event = 9;
      }
      break;
    case 3:
      WritePlayerDisplay((char *) "   3X  ",1);
      DisplayScore(2, F14_Bonus * 1000 * 3);
      if (F14_Multiplier == 3) {
        Event = 9;
      }
      break;
    case 4:
      WritePlayerDisplay((char *) "   4X  ",1);
      DisplayScore(2, F14_Bonus * 1000 * 4);
      if (F14_Multiplier == 4) {
        Event = 9;
      }
      break;
    case 5:
      WritePlayerDisplay((char *) "   5X  ",1);
      DisplayScore(2, F14_Bonus * 1000 * 5);
      if (F14_Multiplier == 5) {
        Event = 9;
      }
      break;
    case 6:
      WritePlayerDisplay((char *) "   6X  ",1);
      DisplayScore(2, F14_Bonus * 1000 * 6);
      if (F14_Multiplier == 6) {
        Event = 9;
      }
      break;
    case 7:
      WritePlayerDisplay((char *) "   7X  ",1);
      DisplayScore(2, F14_Bonus * 1000 * 7);
      if (F14_Multiplier == 7) {
        Event = 9;
      }
      break;
    case 8:
      WritePlayerDisplay((char *) "   8X  ",1);
      DisplayScore(2, F14_Bonus * 1000 * 8);
      if (F14_Multiplier == 8) {
        Event = 9;
      }
      break;
    case 9:
      DisplayScore(1, Points[Player]);
      DisplayScore(2, total_bonus);
      break;
      
    case 10:
      DisplayScore(1, Points[Player]);
      DisplayScore(2, total_bonus);
      if (total_bonus==0) {
        Event = 11;  // jump out once countdown complete
        break;
      }
      if (F14_Bonus == 0) {
        F14_Bonus = temp_bonus;
        F14_Multiplier  --;
      }
      else {
        F14_Bonus --;
      }
      total_bonus = total_bonus - 1000;
      Points[Player] = Points[Player] + 1000;
      F14_BonusHandler(BONUS_LAMP_REFRESH); // update the lamps as we count down.
      break;


  }

  if (Event > 10) {
    bonus_timer = 0;
    WriteUpper("              ");
    F14_BallEnd2(F14_CountBallsInTrunk()); // continue ending the ball
  }
  else if (Event < 10) {
    bonus_timer = ActivateTimer(500, Event+1, F14_AwardBonus);
  }
  else {
    bonus_timer = ActivateTimer(50, 10,  F14_AwardBonus);
  }
}

void F14_ResetHighScores(bool change) {                // delete the high scores file
  if (change) {                                       // if the start button has been pressed
    if (SDfound) {
      SD.remove(GameDefinition.HighScoresFileName);
      struct HighScores HScBuffer;                    // create new high score table
      HallOfFame = HScBuffer;                         // copy it to the hall of fame
      WriteLower(" SCORES DONE    ");}
    else {
      WriteLower(" SCORES NO SD   ");}}
  else {
    WriteLower(" SCORES         ");}}

void F14_CheckHighScore(byte Player) {

}

// Test mode

void F14_Testmode(byte Select) {
  Switch_Pressed = F14_Testmode;
  switch(AppByte) {                                   // which testmode?
  case 0:                                             // display test
    switch(Select) {                                  // switch events
    case 0:                                           // init (not triggered by switch)
      *(DisplayLower) = 0;                            // clear credit display
      *(DisplayLower+16) = 0;
      *(DisplayUpper) = 0;
      *(DisplayUpper+16) = 0;
      if (APC_settings[DisplayType] < 6) {            // Sys11 display
        WriteUpper("DISPLAY TEST    ");
        WriteLower("                ");}
      else {                                          // Sys3 - 9 display
        for(byte c=0; c<16; c++) {                    // clear numerical displays
          *(DisplayLower+2*c) = 255;                  // delete numbers
          *(DisplayLower+1+2*c) = 0;}                 // delete commas
        DisplayScore(1, 1);}                          // show test number
      AppByte2 = 0;
      break;
    case 3:                                           // credit button
      if (APC_settings[DisplayType] < 6) {            // Sys11 display
        WriteUpper("0000000000000000");
        WriteLower("0000000000000000");
        AppByte2 = ActivateTimer(1000, 32, F14_DisplayCycle);}
      else {
        for(byte c=0; c<16; c++) {                    // clear numerical displays
          *(DisplayLower+2*c) = 0;}                   // delete numbers
        AppByte2 = ActivateTimer(1000, 0, F14_DisplayCycle);}
      break;
    case 72:                                          // advance button
      if (AppByte2) {
        KillTimer(AppByte2);
        AppByte2 = 0;}
      else {
        AppByte++;}
      F14_Testmode(0);}
    break;
    case 1:                                           // switch edges test
      switch(Select) {                                // switch events
      case 0:                                         // init (not triggered by switch)
        AppByte2 = 0;
        if (APC_settings[DisplayType] < 6) {          // Sys11 display
          WriteUpper(" SWITCH EDGES   ");
          WriteLower("                ");}
        else {                                        // Sys3 - 9 display
          for(byte c=0; c<16; c++) {                  // clear numerical displays
            *(DisplayLower+2*c) = 255;                // delete numbers
            *(DisplayLower+1+2*c) = 0;}               // delete commas
          DisplayScore(1, 2);}                        // show test number
        break;
      case 72:                                        // advance button
        if (AppByte2) {
          AppByte2 = 0;}
        else {
          AppByte++;}
        F14_Testmode(0);
        break;
      case 3:                                         // credit button
        if (!AppByte2) {
          WriteUpper(" LATEST EDGES   ");
          AppByte2 = 1;
          break;}
        /* no break */
      default:                                        // all other switches
        if (APC_settings[DisplayType] == 6) {         // Sys6 display?
          for (byte i=0; i<10; i++) {                 // move all characters in the lower display row 4 chars to the left
            DisplayLower[2*i] = DisplayLower[2*i+8];}
          *(DisplayLower+24) = ConvertNumLower((byte) Select / 10, 0); // and insert the switch number to the right of the row
          *(DisplayLower+26) = ConvertNumLower((byte) (Select % 10), 0);}
        else if (APC_settings[DisplayType] == 7) {    // Sys7 display?
          for (byte i=1; i<24; i++) {                 // move all characters in the lower display row 4 chars to the left
            DisplayLower[i] = DisplayLower[i+8];}
          *(DisplayLower+28) = ConvertNumLower((byte) Select / 10, 0); // and insert the switch number to the right of the row
          *(DisplayLower+30) = ConvertNumLower((byte) (Select % 10), 0);}
        else {
          for (byte i=1; i<24; i++) {                 // move all characters in the lower display row 4 chars to the left
            DisplayLower[i] = DisplayLower[i+8];}
          *(DisplayLower+30) = DispPattern2[32 + 2 * (Select % 10)]; // and insert the switch number to the right of the row
          *(DisplayLower+31) = DispPattern2[33 + 2 * (Select % 10)];
          *(DisplayLower+28) = DispPattern2[32 + 2 * (Select - (Select % 10)) / 10];
          *(DisplayLower+29) = DispPattern2[33 + 2 * (Select - (Select % 10)) / 10];}}
      break;
      case 2:                                           // solenoid test
        switch(Select) {                                // switch events
        case 0:                                         // init (not triggered by switch)
          if (APC_settings[DisplayType] < 6) {          // Sys11 display
            WriteUpper("  COIL  TEST    ");
            WriteLower("                ");}
          else {                                        // Sys3 - 9 display
            for(byte c=0; c<16; c++) {                  // clear numerical displays
              *(DisplayLower+2*c) = 255;                // delete numbers
              *(DisplayLower+1+2*c) = 0;}               // delete commas
            DisplayScore(1, 3);}                        // show test number
          AppByte2 = 0;
          break;
        case 3:
          WriteUpper(" FIRINGCOIL NO  ");
          AppBool = false;
          AppByte2 = ActivateTimer(1000, 1, F14_FireSolenoids);
          break;
        case 72:
          if (AppByte2) {
            KillTimer(AppByte2);
            AppByte2 = 0;}
          else {
            AppByte++;}
          F14_Testmode(0);}
        break;
        case 3:                                           // single lamp test
          switch(Select) {                                // switch events
          case 0:                                         // init (not triggered by switch)
            if (APC_settings[DisplayType] < 6) {          // Sys11 display
              WriteUpper(" SINGLE LAMP    ");
              WriteLower("                ");}
            else {                                        // Sys3 - 9 display
              for(byte c=0; c<16; c++) {                  // clear numerical displays
                *(DisplayLower+2*c) = 255;                // delete numbers
                *(DisplayLower+1+2*c) = 0;}               // delete commas
              DisplayScore(1, 4);}                        // show test number
            AppByte2 = 0;
            for (i=0; i<8; i++){                          // erase lamp matrix
              LampColumns[i] = 0;}
            LampPattern = LampColumns;                    // and show it
            break;
          case 3:
            WriteUpper(" ACTUAL LAMP    ");
            AppByte2 = ActivateTimer(1000, 1, F14_ShowLamp);
            break;
          case 72:
            LampPattern = NoLamps;
            if (AppByte2) {
              KillTimer(AppByte2);
              AppByte2 = 0;}
            else {
              AppByte++;}
            F14_Testmode(0);}
          break;
          case 4:                                           // all lamps test
            switch(Select) {                                // switch events
            case 0:                                         // init (not triggered by switch)
              if (APC_settings[DisplayType] < 6) {          // Sys11 display
                WriteUpper("  ALL   LAMPS   ");
                WriteLower("                ");}
              else {                                        // Sys3 - 9 display
                for(byte c=0; c<16; c++) {                  // clear numerical displays
                  *(DisplayLower+2*c) = 255;                // delete numbers
                  *(DisplayLower+1+2*c) = 0;}               // delete commas
                DisplayScore(1, 5);}                        // show test number
              AppByte2 = 0;
              break;
            case 3:
              WriteUpper("FLASHNG LAMPS   ");
              AppByte2 = ActivateTimer(1000, 1, F14_ShowAllLamps);
              break;
            case 72:
              LampPattern = NoLamps;
              if (AppByte2) {
                KillTimer(AppByte2);
                AppByte2 = 0;}
              else {
                AppByte++;}
              F14_Testmode(0);}
            break;
            case 5:                                           // all music test
              switch(Select) {                                // switch events
              case 0:                                         // init (not triggered by switch)
                if (APC_settings[DisplayType] < 6) {          // Sys11 display
                  WriteUpper(" MUSIC  TEST    ");
                  WriteLower("                ");}
                else {                                        // Sys3 - 9 display
                  for(byte c=0; c<16; c++) {                  // clear numerical displays
                    *(DisplayLower+2*c) = 255;                // delete numbers
                    *(DisplayLower+1+2*c) = 0;}               // delete commas
                  DisplayScore(1, 6);}                        // show test number
                AppByte2 = 0;
                break;
              case 3:
                WriteUpper("PLAYING MUSIC   ");
                if (APC_settings[Volume]) {                   // system set to digital volume control?
                  analogWrite(VolumePin,255-APC_settings[Volume]);} // adjust PWM to volume setting
                AfterMusic = F14_RepeatMusic;
                AppByte2 = 1;
                PlayMusic(50, "MUSIC.BIN");
                break;
              case 72:
                AfterMusic = 0;
                digitalWrite(VolumePin,HIGH);                 // set volume to zero
                StopPlayingMusic();
                if (AppByte2) {
                  AppByte2 = 0;}
                else {
                  GameDefinition.AttractMode();
                  return;}
                F14_Testmode(0);}
              break;}}

void F14_ShowLamp(byte CurrentLamp) {                  // cycle all solenoids
  if (QuerySwitch(73)) {                              // Up/Down switch pressed?
    if (APC_settings[DisplayType] == 6) {             // Sys6 display?
      *(DisplayLower+24) = ConvertNumLower((byte) CurrentLamp / 10, 0); // and insert the switch number to the right of the row
      *(DisplayLower+26) = ConvertNumLower((byte) (CurrentLamp % 10), 0);}
    else if (APC_settings[DisplayType] == 7) {        // Sys7 display?
      *(DisplayLower+28) = ConvertNumLower((byte) CurrentLamp / 10, 0); // and insert the switch number to the right of the row
      *(DisplayLower+30) = ConvertNumLower((byte) (CurrentLamp % 10), 0);}
    else {                                            // Sys11 display
      *(DisplayLower+30) = DispPattern2[32 + 2 * (CurrentLamp % 10)]; // and show the actual solenoid number
      *(DisplayLower+31) = DispPattern2[33 + 2 * (CurrentLamp % 10)];
      *(DisplayLower+28) = DispPattern2[32 + 2 * (CurrentLamp - (CurrentLamp % 10)) / 10];
      *(DisplayLower+29) = DispPattern2[33 + 2 * (CurrentLamp - (CurrentLamp % 10)) / 10];}
    TurnOnLamp(CurrentLamp);                          // turn on lamp
    if (CurrentLamp > 1) {                            // and turn off the previous one
      TurnOffLamp(CurrentLamp-1);}
    else {
      TurnOffLamp(LampMax);}
    CurrentLamp++;                                    // increase the lamp counter
    if (CurrentLamp == LampMax+1) {                   // maximum reached?
      CurrentLamp = 1;}}                              // then start again
  AppByte2 = ActivateTimer(1000, CurrentLamp, F14_ShowLamp);} // come back in one second

void F14_ShowAllLamps(byte State) {                    // Flash all lamps
  if (State) {                                        // if all lamps are on
    LampColumns[0] = 0;                               // first column
    LampPattern = NoLamps;                            // turn them off
    State = 0;}
  else {                                              // or the other way around
    LampColumns[0] = 255;                             // first column
    LampPattern = AllLamps;
    State = 1;}
  AppByte2 = ActivateTimer(500, State, F14_ShowAllLamps);}  // come back in 500ms

void F14_FireSolenoids(byte Solenoid) {                // cycle all solenoids
  if (AppBool) {                                      // if C bank solenoid
    ActC_BankSol(Solenoid);
    *(DisplayLower+30) = DispPattern2[('C'-32)*2];    // show the C
    *(DisplayLower+31) = DispPattern2[('C'-32)*2+1];
    if (QuerySwitch(73)) {                            // Up/Down switch pressed?
      AppBool = false;
      Solenoid++;}}
  else {                                              // if A bank solenoid
    if (APC_settings[DisplayType] == 6) {             // Sys6 display?
      *(DisplayLower+24) = ConvertNumLower((byte) Solenoid / 10, 0); // and insert the switch number to the right of the row
      *(DisplayLower+26) = ConvertNumLower((byte) (Solenoid % 10), 0);}
    else if (APC_settings[DisplayType] == 7) {        // Sys7 display?
      *(DisplayLower+28) = ConvertNumLower((byte) Solenoid / 10, 0); // and insert the switch number to the right of the row
      *(DisplayLower+30) = ConvertNumLower((byte) (Solenoid % 10), 0);}
    else {                                            // Sys11 display
      *(DisplayLower+28) = DispPattern2[32 + 2 * (Solenoid % 10)]; // show the actual solenoid number
      *(DisplayLower+29) = DispPattern2[33 + 2 * (Solenoid % 10)];
      *(DisplayLower+26) = DispPattern2[32 + 2 * (Solenoid - (Solenoid % 10)) / 10];
      *(DisplayLower+27) = DispPattern2[33 + 2 * (Solenoid - (Solenoid % 10)) / 10];}
    if (!(*(GameDefinition.SolTimes+Solenoid-1))) {   // can this solenoid be turned on permanently?
      ActivateSolenoid(500, Solenoid);}               // then the duration must be specified
    else {
      ActivateSolenoid(0, Solenoid);}                 // activate the solenoid with default duration
    if ((Solenoid < 9) && game_settings[F14set_ACselectRelay]) { // A solenoid and Sys11 machine?
      *(DisplayLower+30) = DispPattern2[('A'-32)*2];  // show the A
      *(DisplayLower+31) = DispPattern2[('A'-32)*2+1];
      if (QuerySwitch(73)) {                          // Up/Down switch pressed?
        AppBool = true;}}
    else {
      if (APC_settings[DisplayType] < 6) {            // Sys11 display?
        *(DisplayLower+30) = DispPattern2[(' '-32)*2];// delete the C
        *(DisplayLower+31) = DispPattern2[(' '-32)*2+1];}
      if (QuerySwitch(73)) {                          // Up/Down switch pressed?
        Solenoid++;                                   // increase the solenoid counter
        if (Solenoid > 22) {                          // maximum reached?
          Solenoid = 1;}}}}                           // then start again
  AppByte2 = ActivateTimer(1000, Solenoid, F14_FireSolenoids);}   // come back in one second

void F14_DisplayCycle(byte CharNo) {                   // Display cycle test
  if (QuerySwitch(73)) {                              // cycle only if Up/Down switch is not pressed
    if (CharNo < 11) {                                // numerical display
      CharNo++;
      if (CharNo > 9) {
        CharNo = 0;}
      byte Comma = 0;
      byte Num = ConvertNumUpper(CharNo, 0);
      Num = ConvertNumLower(CharNo, Num);
      if (CharNo & 1) {                               // only do commas at every second run
        Comma = 129;}
      for(byte c=0; c<16; c++) {                      // clear numerical displays
        *(DisplayLower+2*c) = Num;                    // write numbers
        *(DisplayLower+1+2*c) = Comma;}}
    else {                                            // System11 display
      if (CharNo == 116) {                            // if the last character is reached
        CharNo = 32;}                                 // start from the beginning
      else {
        if (CharNo == 50) {                           // reached the gap between numbers and characters?
          CharNo = 66;}
        else {
          CharNo = CharNo+2;}}                        // otherwise show next character
      for (i=0; i<16; i++) {                          // use for all alpha digits
        if ((APC_settings[DisplayType] != 3 && APC_settings[DisplayType] != 4 && APC_settings[DisplayType] != 5) && ((i==0) || (i==8))) {
          DisplayUpper[2*i] = LeftCredit[CharNo];
          DisplayUpper[2*i+1] = LeftCredit[CharNo+1];
          DisplayLower[2*i] = RightCredit[CharNo];
          DisplayLower[2*i+1] = RightCredit[CharNo+1];}
        else {
          DisplayUpper[2*i] = DispPattern1[CharNo];
          DisplayUpper[2*i+1] = DispPattern1[CharNo+1];
          if (APC_settings[DisplayType] == 4 || APC_settings[DisplayType] == 5) { // 16 digit numerical
            DisplayLower[2*i] = DispPattern2[CharNo];
            DisplayLower[2*i+1] = ConvertTaxi(DispPattern2[CharNo]);}
          else {
            DisplayLower[2*i] = DispPattern2[CharNo];
            DisplayLower[2*i+1] = DispPattern2[CharNo+1];}}}}}
  AppByte2 = ActivateTimer(500, CharNo, F14_DisplayCycle);}   // restart timer

void F14_RepeatMusic(byte Dummy) {
  UNUSED(Dummy);
  //PlayMusic(50, "MUSIC.BIN");
  }

// Display for showing ball locked
// Event 0 - start the animation
// Event 99 - terminate early
void F14_LockedBallAnimation(byte Event) {
  static byte display_step_timer = 0;
  byte balls_locked = 0;

 /*if (APC_settings[DebugMode]){
    Serial.print("F14_LockedBallAnimation event ");            // print address reference table
    Serial.println((byte)Event);
  }*/


  for (byte i=0; i<3; i++) {
    if (F14_LockStatus[Player][i]==2) {
      balls_locked++;
    }
  }
  switch (Event) {
    case 0:
      if (!display_step_timer) {
        WriteUpper2("              ");
        WriteLower2("              ");
        SwitchDisplay(0); // second buffer
      }
      break;
    case 1:
    case 3:
    case 5:
    case 7:
      if (balls_locked == 1) {
        WriteUpper2("ENEMY 1 LOCKED");
      }
      else if (balls_locked == 2) {
        WriteUpper2("ENEMY 2 LOCKED");
      }
      else {
        WriteUpper2("ENEMY 3 LOCKED");
      }
      break;
    case 2:
    case 4:
    case 6:
    case 8:
      WriteUpper2("ENEMY   LOCKED");
      break;
    case 99:
      if (display_step_timer) {
        KillTimer(display_step_timer);
        display_step_timer = 0;
        SwitchDisplay(1);
      }
      break;
    }

    if (Event < 8) {
      display_step_timer = ActivateTimer(250, Event+1, F14_LockedBallAnimation);
    }
    else {
      display_step_timer = 0;
      SwitchDisplay(1);
    }
}

// Run the display animation when a lock is lit
// Event 0 - run the animation
// Event 1 - kill the animation early
void F14_LockIsLitAnimation(byte Event) {

static byte display_step_timer = 0;

 /*if (APC_settings[DebugMode]){
    Serial.print("F14_LockIsLitAnimation event ");            // print address reference table
    Serial.println((byte)Event);
  }*/


switch (Event) {
  case 0:
    if (!display_step_timer) {
      WriteUpper2("              ");
      WriteLower2("              ");
      SwitchDisplay(0);  // second buffer
    }
    break;

  case 2:
  case 12:
    WriteUpper2("      <>      ");
    break;
  case 3:
  case 13:
    WriteUpper2("     <<>>     ");
    break;
  case 4:
  case 14:
    WriteUpper2("    <<<>>>    ");
    break;
  case 5:
  case 15:
    WriteUpper2("   <<<  >>>   ");
    break;
  case 6:
  case 16:
    WriteUpper2("  <<<    >>>  ");
    break;
  case 7:
  case 17:
    WriteUpper2(" <<<      >>> ");
    break;
  case 8:
  case 18:
    WriteUpper2("<<<        >>>");
    break;
  case 9:
  case 19:
    WriteUpper2("<<          >>");
    break;
  case 10:
  case 20:
    WriteUpper2("<            >");
    break;
  case 11:
  case 21:
    WriteUpper2("              ");
    break;
  case 22:
    WriteUpper2("TOMCAT  TOMCAT");
    break;
  case 23:
    WriteUpper2("              ");
    break;
  case 24:
    WriteUpper2("TOMCAT  TOMCAT");
    break;
  case 25:
    WriteUpper2("              ");
    break;
  case 26:
    WriteUpper2("TOMCAT  TOMCAT");
    break;
  case 27:
    WriteUpper2("              ");
    break;
  case 28:
    WriteUpper2("TOMCAT  TOMCAT");
    break;
  case 29:
    WriteUpper2("              ");
    break;
  case 30:
    WriteUpper2("TOMCAT  TOMCAT");
    break;
  case 31:
    WriteUpper2("              ");
    break;
  case 99:
    if (display_step_timer) {
      KillTimer(display_step_timer);
      display_step_timer = 0;
      SwitchDisplay(1);
    }
    return;
    break;
  }
  
  if (Event > 31) {
    display_step_timer = 0;
    SwitchDisplay(1);
  }
  else if (Event > 21) {
    display_step_timer = ActivateTimer(200,Event + 1, F14_LockIsLitAnimation);
  }
  else  {
    display_step_timer = ActivateTimer(50,Event + 1, F14_LockIsLitAnimation);
  }
  
}

// uses C and ? as crosshairs in char set. >< will be like the aircraft in the crosshairs
void F14_WeaponsAnimation(byte Event) {
  static byte display_step_timer = 0;
  byte next_event;

  /*if (APC_settings[DebugMode]){
    Serial.print("F14_WeaponsAnimation event ");            // print address reference table
    Serial.println((byte)Event);
  }*/

  switch (Event) {
    case 0:
      if (display_step_timer) {
        KillTimer(display_step_timer);
        display_step_timer = 0;
      }

      WriteUpper2("              ");
      WriteLower2("              ");
      SwitchDisplay(0); // buffer 2
      //F14_GIOn(255,0,0);
      break;
    case 1:
      WriteUpper2("WEAPONSSYSTEMS");
      WriteLower2("              ");
    break;
    case 2:
      WriteLower2("<             ");
    break;
    case 3:
      WriteLower2("><            ");
    break;
    case 4:
      WriteLower2(" ><           ");
    break;
    case 5:
      WriteLower2("  ><          ");
    break;
    case 6:
      WriteLower2("?  ><         ");
    break;
    case 7:
      WriteLower2(" ?  ><        ");
    break;
    case 8:
      WriteLower2("  ?  ><       ");
    break;
    case 9:
      WriteLower2("C  ?  ><      ");
    break;
    case 10:
      WriteLower2(" C  ?  ><     ");
    break;
    case 11:
      WriteLower2("  C  ?  ><    ");
    break;
    case 12:
      WriteLower2("   C  ?  ><   ");
    break;
    case 13:
      WriteLower2("    C  ? ><   ");
    break;
    case 14:
      WriteLower2("    C  ?><    ");
    break;
    case 15:
      WriteLower2("     C  ?<    ");
    break;
    case 16:
      WriteLower2("      C >?    ");
    break;
    case 17:
      WriteLower2("       C><?   ");
    break;
    case 18:
      WriteLower2("        C< ?  ");
    break;
    case 19:
      WriteLower2("       >C  ?  ");
    break;
    case 20:
      WriteLower2("      ><C  ?  ");
    break;
    case 21:
      WriteLower2("     >< C  ?  ");
    break;
    case 22:
      WriteLower2("    >< C  ?   ");
    break;
    case 23:
      WriteLower2("   >< C  ?    ");
    break;
    case 24:
      WriteLower2("   ><C  ?     ");
    break;
    case 25:
      WriteLower2("   >C  ?      ");
    break;
    case 26:
      WriteLower2("   C< ?       ");
    break;
    case 27:
      WriteLower2("   C><?       ");
    break;
    case 28:
      WriteUpper2("LOCKED   ON   ");
      WriteLower2("   C><?       ");
      break;
    case 29:
      WriteUpper2("              ");
      WriteLower2("              ");
      break;
    case 30:
      WriteUpper2("LOCKED   ON   ");
      WriteLower2("   C><?       ");
      break;
    case 31:
      WriteUpper2("              ");
      WriteLower2("              ");
      break;
    case 32:
      WriteUpper2("LOCKED   ON   ");
      WriteLower2("   C><?       ");
      break;
    case 33:
      WriteUpper2("              ");
      WriteLower2("              ");
      break;
    case 99:
      if (display_step_timer) {
        KillTimer(display_step_timer);
        display_step_timer = 0;
        SwitchDisplay(1);
        //F14_GIOn(255,255,255);
      }
      break;
  }
  
  if (Event > 33) {
    display_step_timer = 0;
    SwitchDisplay(1);  // back to the old display
    //F14_GIOn(255,255,255);
    F14_AnimationHandler(ANIMATION_WEAPONS,ANIMATION_END); // let the handler know animation is done.
  }
  else if (Event < 28) {
    display_step_timer = ActivateTimer(75,Event+1,F14_WeaponsAnimation);
  }
  else {
    display_step_timer = ActivateTimer(150,Event+1,F14_WeaponsAnimation);
  }
}

// multiball intro
void F14_MultiBallAnimation(byte Event) {
  static byte display_step_timer = 0;
  /*if (APC_settings[DebugMode]){
    Serial.print("F14_MultiBallAnimation event ");            // print address reference table
    Serial.println((byte)Event);
  }*/

  switch (Event) {
    case 0:
      if (display_step_timer) {
        KillTimer(display_step_timer);
        display_step_timer = 0;
      }

      WriteUpper2("              ");
      WriteLower2("              ");
      SwitchDisplay(0); // buffer 2
      break;
    case 1:
      WriteUpper2("*DANGERDANGER*");
      WriteLower2("              ");
      break;
    case 2:
      WriteUpper2("              ");
      break;
    case 3:
      WriteUpper2("*DANGERDANGER*");
      break;
    case 4:
      WriteUpper2("              ");
      break;
    case 5:
      WriteUpper2("*DANGERDANGER*");
      break;
    case 6:
      WriteUpper2("              ");
      break;
    case 7:
      WriteUpper2("TARGETSSIGHTED");
      break;
    case 8:
      WriteUpper2("              ");
      break;
    case 9:
      WriteUpper2("TARGETSSIGHTED");
      break;
    case 10:
      WriteUpper2("              ");
      break;
    case 11:
      WriteUpper2("TARGETSSIGHTED");
      break;
    case 12:
      WriteUpper2("              ");
      break;
    case 13:
      WriteUpper2("DESTROYENEMIES");
      break;
    case 14:
      WriteUpper2("              ");
      break;
    case 15:
      WriteUpper2("DESTROYENEMIES");
      break;
    case 16:
      WriteUpper2("              ");
      break;
    case 17:
      WriteUpper2("DESTROYENEMIES");
      break;
    case 18:
      WriteUpper2("              ");
      break;
    
  }

  if (Event > 18) {
    display_step_timer = 0;
    SwitchDisplay(1);  // back to the old display
    F14_AnimationHandler(ANIMATION_START_MULTIBALL,ANIMATION_END); // let the handler know animation is done.
  }
  else {
    display_step_timer = ActivateTimer(200,Event+1,F14_MultiBallAnimation);
  }

}

void F14_LaunchBonusAnimation(byte Event){
static byte display_step_timer = 0;

  /*if (APC_settings[DebugMode]){
    Serial.print("F14_LaunchBonusAnimation event ");            // print address reference table
    Serial.println((byte)Event);
  }*/

  switch (Event) {
    case 0:
      if (display_step_timer) {
        KillTimer(display_step_timer);
        display_step_timer = 0;
      }

      WriteUpper2("              ");
      WriteLower2("              ");
      SwitchDisplay(0); // buffer 2
      break;
    case 1:
      WriteUpper2(" LAUNCHBONUS ");
      WriteLower2("             ");
      ShowNumber(30,F14_LaunchBonus*10000);
      ShowNumber(22,F14_LaunchBonus*10000);
      break;
    case 2:
      WriteUpper2("              ");
      WriteLower2("              ");
      break;
    case 3:
      WriteUpper2(" LAUNCHBONUS ");
      ShowNumber(30,F14_LaunchBonus*10000);
      ShowNumber(22,F14_LaunchBonus*10000);
      break;
    case 4:
      WriteUpper2("              ");
      WriteLower2("              ");
      break;
    case 5:
      WriteUpper2(" LAUNCHBONUS ");
      ShowNumber(30,F14_LaunchBonus*10000);
      ShowNumber(22,F14_LaunchBonus*10000);
      break;
    case 6:
      WriteUpper2("              ");
      WriteLower2("              ");
      break;
   
  }

  if (Event > 6) {
    display_step_timer = 0;
    SwitchDisplay(1);  // back to the old display
    F14_AnimationHandler(ANIMATION_LAUNCH_BONUS,ANIMATION_END); // let the handler know animation is done.
  }
  else {
    display_step_timer = ActivateTimer(500,Event+1,F14_LaunchBonusAnimation);
  }

}

void F14_SafeLandingAnimation(byte Event){
static byte display_step_timer = 0;


  if (APC_settings[DebugMode]){
    Serial.print("F14_SafeLandingAnimation event ");            // print address reference table
    Serial.println((byte)Event);
  }

  switch (Event) {
    case 0:
      if (display_step_timer) {
        KillTimer(display_step_timer);
        display_step_timer = 0;
      }

      WriteUpper2("              ");
      WriteLower2("              ");
      SwitchDisplay(0); // buffer 2
      break;
    case 1:
      WriteUpper2(" SAFE  LANDING");
      break;
    case 2:
      WriteUpper2("              ");
      break;
    case 3:
      WriteUpper2(" SAFE  LANDING");
      break;
    case 4:
      WriteUpper2("              ");
      break;
    case 5:
      WriteUpper2(" SAFE  LANDING");
      break;
    case 6:
      WriteUpper2("              ");
      break;
   
  }

  if (Event > 6) {
    display_step_timer = 0;
    SwitchDisplay(1);  // back to the old display
    F14_AnimationHandler(ANIMATION_LAUNCH_BONUS,ANIMATION_END); // let the handler know animation is done.
  }
  else {
    display_step_timer = ActivateTimer(500,Event+1,F14_SafeLandingAnimation);
  }

}


void F14_LampShowRotate (byte Step) {
static byte step_timer=0;
 
switch (Step) {
case 0:
  F14Show_TurnOnLamp(3);
  F14Show_TurnOnLamp(109);
  F14Show_TurnOnLamp(105);
  F14Show_TurnOffLamp(2);
  break;
case 1:
  F14Show_TurnOnLamp(104);
  F14Show_TurnOffLamp(105);
  F14Show_TurnOnLamp(5);
  F14Show_TurnOffLamp(2);
  F14Show_TurnOffLamp(111);
  F14Show_TurnOnLamp(54);
  break;
case 2:
  F14Show_TurnOnLamp(25);
  F14Show_TurnOffLamp(109);
  F14Show_TurnOffLamp(103);
  F14Show_TurnOffLamp(105);
  F14Show_TurnOffLamp(2);
  F14Show_TurnOnLamp(111);
  break;
case 3:
  F14Show_TurnOnLamp(26);
  F14Show_TurnOffLamp(109);
  F14Show_TurnOnLamp(102);
  F14Show_TurnOnLamp(103);
  F14Show_TurnOffLamp(104);
  break;
case 4:
  F14Show_TurnOffLamp(25);
  F14Show_TurnOffLamp(27);
  F14Show_TurnOffLamp(102);
  F14Show_TurnOffLamp(103);
  F14Show_TurnOffLamp(5);
  F14Show_TurnOnLamp(53);
  break;
case 5:
  F14Show_TurnOffLamp(26);
  F14Show_TurnOnLamp(27);
  F14Show_TurnOffLamp(102);
  F14Show_TurnOffLamp(5);
  F14Show_TurnOffLamp(59);
  F14Show_TurnOffLamp(111);
  F14Show_TurnOffLamp(114);
  F14Show_TurnOffLamp(54);
  break;
case 6:
  F14Show_TurnOnLamp(28);
  F14Show_TurnOnLamp(59);
  F14Show_TurnOffLamp(111);
  F14Show_TurnOnLamp(114);
  F14Show_TurnOffLamp(54);
  break;
case 7:
  F14Show_TurnOffLamp(27);
  F14Show_TurnOffLamp(3);
  F14Show_TurnOnLamp(62);
  F14Show_TurnOffLamp(52);
  break;
case 8:
  F14Show_TurnOffLamp(29);
  F14Show_TurnOffLamp(3);
  F14Show_TurnOffLamp(59);
  F14Show_TurnOffLamp(48);
  F14Show_TurnOffLamp(115);
  F14Show_TurnOnLamp(52);
  break;
case 9:
  F14Show_TurnOffLamp(28);
  F14Show_TurnOnLamp(29);
  F14Show_TurnOffLamp(3);
  F14Show_TurnOnLamp(39);
  F14Show_TurnOnLamp(8);
  F14Show_TurnOffLamp(62);
  F14Show_TurnOffLamp(59);
  F14Show_TurnOnLamp(48);
  F14Show_TurnOffLamp(33);
  F14Show_TurnOnLamp(115);
  F14Show_TurnOffLamp(53);
  break;
case 10:
  F14Show_TurnOffLamp(8);
  F14Show_TurnOffLamp(62);
  F14Show_TurnOnLamp(33);
  F14Show_TurnOffLamp(114);
  F14Show_TurnOffLamp(53);
  break;
case 11:
  F14Show_TurnOffLamp(29);
  F14Show_TurnOffLamp(30);
  F14Show_TurnOffLamp(39);
  F14Show_TurnOnLamp(34);
  F14Show_TurnOffLamp(116);
  break;
case 12:
  F14Show_TurnOffLamp(29);
  F14Show_TurnOnLamp(30);
  F14Show_TurnOffLamp(32);
  F14Show_TurnOnLamp(57);
  F14Show_TurnOffLamp(48);
  F14Show_TurnOffLamp(33);
  F14Show_TurnOnLamp(116);
  F14Show_TurnOffLamp(112);
  F14Show_TurnOffLamp(52);
  break;
case 13:
  F14Show_TurnOffLamp(23);
  F14Show_TurnOnLamp(32);
  F14Show_TurnOnLamp(60);
  F14Show_TurnOffLamp(35);
  F14Show_TurnOffLamp(115);
  F14Show_TurnOnLamp(112);
  F14Show_TurnOffLamp(52);
  break;
case 14:
  F14Show_TurnOnLamp(23);
  F14Show_TurnOffLamp(30);
  F14Show_TurnOffLamp(31);
  F14Show_TurnOffLamp(57);
  F14Show_TurnOnLamp(35);
  F14Show_TurnOffLamp(34);
  F14Show_TurnOnLamp(56);
  F14Show_TurnOffLamp(115);
  break;
case 15:
  F14Show_TurnOffLamp(30);
  F14Show_TurnOnLamp(31);
  F14Show_TurnOffLamp(60);
  F14Show_TurnOffLamp(57);
  break;
case 16:
  F14Show_TurnOffLamp(32);
  F14Show_TurnOffLamp(60);
  F14Show_TurnOnLamp(58);
  F14Show_TurnOffLamp(35);
  F14Show_TurnOnLamp(38);
  break;
case 17:
  F14Show_TurnOffLamp(22);
  F14Show_TurnOffLamp(23);
  F14Show_TurnOnLamp(55);
  F14Show_TurnOnLamp(61);
  F14Show_TurnOffLamp(56);
  F14Show_TurnOffLamp(116);
  F14Show_TurnOffLamp(110);
  F14Show_TurnOnLamp(49);
  break;
case 18:
  F14Show_TurnOnLamp(22);
  F14Show_TurnOffLamp(23);
  F14Show_TurnOffLamp(31);
  F14Show_TurnOffLamp(58);
  F14Show_TurnOffLamp(37);
  F14Show_TurnOffLamp(56);
  F14Show_TurnOffLamp(116);
  F14Show_TurnOnLamp(110);
  break;
case 19:
  F14Show_TurnOffLamp(31);
  F14Show_TurnOffLamp(61);
  F14Show_TurnOnLamp(37);
  F14Show_TurnOffLamp(38);
  F14Show_TurnOffLamp(112);
  break;
case 20:
  F14Show_TurnOffLamp(21);
  F14Show_TurnOnLamp(64);
  F14Show_TurnOnLamp(36);
  F14Show_TurnOffLamp(112);
  break;
case 21:
  F14Show_TurnOnLamp(21);
  F14Show_TurnOffLamp(22);
  F14Show_TurnOffLamp(55);
  F14Show_TurnOffLamp(1);
  F14Show_TurnOffLamp(37);
  F14Show_TurnOffLamp(113);
  F14Show_TurnOnLamp(50);
  break;
case 22:
  F14Show_TurnOffLamp(55);
  F14Show_TurnOnLamp(1);
  F14Show_TurnOffLamp(36);
  F14Show_TurnOnLamp(113);
  F14Show_TurnOffLamp(49);
  break;
case 23:
  F14Show_TurnOffLamp(20);
  F14Show_TurnOffLamp(36);
  F14Show_TurnOffLamp(110);
  F14Show_TurnOffLamp(49);
  break;
case 24:
  F14Show_TurnOffLamp(21);
  F14Show_TurnOnLamp(20);
  F14Show_TurnOffLamp(64);
  F14Show_TurnOffLamp(110);
  F14Show_TurnOnLamp(51);
  break;
case 25:
  F14Show_TurnOffLamp(19);
  F14Show_TurnOffLamp(63);
  F14Show_TurnOffLamp(50);
  break;
case 26:
  F14Show_TurnOnLamp(19);
  F14Show_TurnOffLamp(20);
  F14Show_TurnOnLamp(63);
  F14Show_TurnOffLamp(113);
  F14Show_TurnOffLamp(50);
  break;
case 27:
  F14Show_TurnOffLamp(18);
  F14Show_TurnOffLamp(108);
  F14Show_TurnOnLamp(40);
  F14Show_TurnOnLamp(2);
  F14Show_TurnOnLamp(7);
  F14Show_TurnOffLamp(113);
  break;
case 28:
  F14Show_TurnOnLamp(18);
  F14Show_TurnOffLamp(19);
  F14Show_TurnOnLamp(107);
  F14Show_TurnOnLamp(108);
  F14Show_TurnOffLamp(1);
  F14Show_TurnOffLamp(51);
  break;
case 29:
  F14Show_TurnOnLamp(17);
  F14Show_TurnOffLamp(18);
  F14Show_TurnOffLamp(109);
  F14Show_TurnOffLamp(106);
  F14Show_TurnOffLamp(108);
  F14Show_TurnOffLamp(1);
  F14Show_TurnOffLamp(63);
  F14Show_TurnOffLamp(51);
  break;
case 30:
  F14Show_TurnOffLamp(18);
  F14Show_TurnOnLamp(109);
  F14Show_TurnOffLamp(107);
  F14Show_TurnOffLamp(1);
  F14Show_TurnOffLamp(63);
  F14Show_TurnOffLamp(17);
  F14Show_TurnOffLamp(7);
  break;
  }
  if (Step == 255) {
    if (step_timer) {
      KillTimer(step_timer);
      step_timer=0;
    }
  }
  else if (Step==30) {
    step_timer=ActivateTimer(25,0,F14_LampShowRotate);
  }
  else {
    step_timer = ActivateTimer(25, Step + 1, F14_LampShowRotate);
  }
}


void F14_LampShowTwinkle(byte Step) {
  static byte twinkle_timer = 0;
  switch (Step) {
    case 0:
      for (byte i=1; i<64; i+=2) {
        F14Show_TurnOnLamp(i);
        F14Show_TurnOffLamp(i+1);
      }
      for (byte i=102; i<117; i+=2) {
        F14Show_TurnOnLamp(i);
        F14Show_TurnOffLamp(i+1);
      }

      twinkle_timer = ActivateTimer(100,1,F14_LampShowTwinkle);
      break;
    case 1:
      for (byte i=1; i<64; i+=2) {
        F14Show_TurnOffLamp(i);
        F14Show_TurnOnLamp(i+1);
      }
      for (byte i=102; i<117; i+=2) {
        F14Show_TurnOffLamp(i);
        F14Show_TurnOnLamp(i+1);
      }

      twinkle_timer = ActivateTimer(100,0,F14_LampShowTwinkle);
      break;
    case 255: //stop
      if (twinkle_timer) {
        KillTimer(twinkle_timer);
        twinkle_timer = 0;
      }
      break;
  }
}

void F14_LampShowUpDown (byte Step) {
static byte step_timer=0;
 
switch (Step) {
case 0:
  break;
case 1:
  F14Show_TurnOnLamp(51);
  F14Show_TurnOnLamp(54);
  break;
case 2:
  F14Show_TurnOnLamp(32);
  F14Show_TurnOnLamp(2);
  F14Show_TurnOnLamp(56);
  F14Show_TurnOffLamp(51);
  F14Show_TurnOnLamp(50);
  F14Show_TurnOnLamp(53);
  break;
case 3:
  F14Show_TurnOnLamp(49);
  F14Show_TurnOffLamp(54);
  F14Show_TurnOnLamp(52);
  break;
case 4:
  F14Show_TurnOffLamp(32);
  F14Show_TurnOnLamp(55);
  F14Show_TurnOnLamp(3);
  F14Show_TurnOnLamp(1);
  F14Show_TurnOffLamp(2);
  F14Show_TurnOffLamp(56);
  F14Show_TurnOffLamp(50);
  F14Show_TurnOffLamp(53);
  break;
case 5:
  F14Show_TurnOffLamp(49);
  F14Show_TurnOffLamp(52);
  break;
case 6:
  F14Show_TurnOffLamp(55);
  F14Show_TurnOffLamp(3);
  F14Show_TurnOffLamp(1);
  break;
case 7:
  F14Show_TurnOnLamp(40);
  break;
  break;
case 8:
  F14Show_TurnOffLamp(40);
  F14Show_TurnOnLamp(111);
  F14Show_TurnOnLamp(63);
  break;
  break;
case 9:
  F14Show_TurnOnLamp(64);
  F14Show_TurnOnLamp(48);
  F14Show_TurnOffLamp(111);
  F14Show_TurnOffLamp(63);
  F14Show_TurnOnLamp(116);
  F14Show_TurnOnLamp(112);
  break;
case 10:
  F14Show_TurnOnLamp(115);
  F14Show_TurnOnLamp(110);
  break;
case 11:
  F14Show_TurnOnLamp(23);
  F14Show_TurnOnLamp(31);
  F14Show_TurnOffLamp(64);
  F14Show_TurnOffLamp(48);
  F14Show_TurnOffLamp(116);
  F14Show_TurnOnLamp(114);
  F14Show_TurnOnLamp(113);
  F14Show_TurnOffLamp(112);
  break;
case 12:
  F14Show_TurnOnLamp(30);
  F14Show_TurnOffLamp(115);
  F14Show_TurnOffLamp(110);
  break;
case 13:
  F14Show_TurnOnLamp(22);
  F14Show_TurnOffLamp(23);
  F14Show_TurnOffLamp(31);
  F14Show_TurnOnLamp(109);
  F14Show_TurnOnLamp(35);
  F14Show_TurnOffLamp(114);
  F14Show_TurnOffLamp(113);
  break;
case 14:
  F14Show_TurnOnLamp(21);
  F14Show_TurnOnLamp(29);
  F14Show_TurnOffLamp(30);
  F14Show_TurnOnLamp(5);
  F14Show_TurnOnLamp(38);
  F14Show_TurnOnLamp(7);
  break;
case 15:
  F14Show_TurnOffLamp(22);
  F14Show_TurnOffLamp(109);
  F14Show_TurnOffLamp(35);
  F14Show_TurnOnLamp(34);
  F14Show_TurnOnLamp(37);
  break;
case 16:
  F14Show_TurnOffLamp(21);
  F14Show_TurnOnLamp(28);
  F14Show_TurnOffLamp(29);
  F14Show_TurnOnLamp(20);
  F14Show_TurnOffLamp(5);
  F14Show_TurnOnLamp(33);
  F14Show_TurnOffLamp(38);
  F14Show_TurnOffLamp(7);
  break;
case 17:
  F14Show_TurnOnLamp(27);
  F14Show_TurnOffLamp(34);
  F14Show_TurnOffLamp(37);
  F14Show_TurnOnLamp(36);
  break;
case 18:
  F14Show_TurnOnLamp(19);
  F14Show_TurnOffLamp(28);
  F14Show_TurnOffLamp(20);
  F14Show_TurnOffLamp(33);
  break;
case 19:
  F14Show_TurnOnLamp(18);
  F14Show_TurnOnLamp(26);
  F14Show_TurnOnLamp(39);
  F14Show_TurnOffLamp(36);
  break;
case 20:
  F14Show_TurnOffLamp(19);
  F14Show_TurnOffLamp(27);
  break;
case 21:
  F14Show_TurnOnLamp(17);
  F14Show_TurnOnLamp(25);
  F14Show_TurnOffLamp(39);
  break;
case 22:
  F14Show_TurnOffLamp(18);
  F14Show_TurnOffLamp(26);
  F14Show_TurnOnLamp(105);
  break;
case 23:
  F14Show_TurnOffLamp(17);
  F14Show_TurnOffLamp(25);
  F14Show_TurnOnLamp(103);
  F14Show_TurnOnLamp(104);
  F14Show_TurnOnLamp(106);
  F14Show_TurnOnLamp(107);
  break;
case 24:
  F14Show_TurnOnLamp(102);
  F14Show_TurnOnLamp(108);
  break;
case 25:
  F14Show_TurnOffLamp(103);
  F14Show_TurnOffLamp(104);
  F14Show_TurnOffLamp(105);
  F14Show_TurnOffLamp(106);
  F14Show_TurnOffLamp(107);
  break;
case 26:
  F14Show_TurnOnLamp(16);
  F14Show_TurnOffLamp(102);
  F14Show_TurnOffLamp(108);
  break;
  break;
case 27:
  F14Show_TurnOnLamp(4);
  F14Show_TurnOffLamp(16);
  break;
case 28:
  F14Show_TurnOffLamp(4);
  F14Show_TurnOnLamp(16);
  break;
  break;
case 29:
  F14Show_TurnOffLamp(16);
  F14Show_TurnOnLamp(102);
  F14Show_TurnOnLamp(108);
  break;
case 30:
  F14Show_TurnOnLamp(103);
  F14Show_TurnOnLamp(104);
  F14Show_TurnOnLamp(105);
  F14Show_TurnOnLamp(106);
  F14Show_TurnOnLamp(107);
  break;
case 31:
  F14Show_TurnOffLamp(102);
  F14Show_TurnOffLamp(108);
  break;
case 32:
  F14Show_TurnOnLamp(17);
  F14Show_TurnOnLamp(25);
  F14Show_TurnOffLamp(103);
  F14Show_TurnOffLamp(104);
  F14Show_TurnOffLamp(106);
  F14Show_TurnOffLamp(107);
  break;
case 33:
  F14Show_TurnOnLamp(18);
  F14Show_TurnOnLamp(26);
  F14Show_TurnOffLamp(105);
  break;
case 34:
  F14Show_TurnOffLamp(17);
  F14Show_TurnOffLamp(25);
  F14Show_TurnOnLamp(39);
  break;
case 35:
  F14Show_TurnOnLamp(19);
  F14Show_TurnOnLamp(27);
  break;
case 36:
  F14Show_TurnOffLamp(18);
  F14Show_TurnOffLamp(26);
  F14Show_TurnOffLamp(39);
  F14Show_TurnOnLamp(36);
  break;
case 37:
  F14Show_TurnOffLamp(19);
  F14Show_TurnOnLamp(28);
  F14Show_TurnOnLamp(20);
  F14Show_TurnOnLamp(33);
  break;
case 38:
  F14Show_TurnOffLamp(27);
  F14Show_TurnOnLamp(34);
  F14Show_TurnOnLamp(37);
  F14Show_TurnOffLamp(36);
  break;
case 39:
  F14Show_TurnOnLamp(21);
  F14Show_TurnOffLamp(28);
  F14Show_TurnOnLamp(29);
  F14Show_TurnOffLamp(20);
  F14Show_TurnOnLamp(5);
  F14Show_TurnOffLamp(33);
  F14Show_TurnOnLamp(38);
  F14Show_TurnOnLamp(7);
  break;
case 40:
  F14Show_TurnOnLamp(22);
  F14Show_TurnOnLamp(109);
  F14Show_TurnOnLamp(35);
  F14Show_TurnOffLamp(34);
  F14Show_TurnOffLamp(37);
  break;
case 41:
  F14Show_TurnOffLamp(21);
  F14Show_TurnOffLamp(29);
  F14Show_TurnOnLamp(30);
  F14Show_TurnOffLamp(5);
  F14Show_TurnOffLamp(38);
  F14Show_TurnOffLamp(7);
  break;
case 42:
  F14Show_TurnOffLamp(22);
  F14Show_TurnOnLamp(23);
  F14Show_TurnOnLamp(31);
  F14Show_TurnOffLamp(109);
  F14Show_TurnOffLamp(35);
  F14Show_TurnOnLamp(114);
  F14Show_TurnOnLamp(113);
  break;
case 43:
  F14Show_TurnOffLamp(30);
  F14Show_TurnOnLamp(115);
  F14Show_TurnOnLamp(110);
  break;
case 44:
  F14Show_TurnOffLamp(23);
  F14Show_TurnOffLamp(31);
  F14Show_TurnOnLamp(64);
  F14Show_TurnOnLamp(48);
  F14Show_TurnOnLamp(116);
  F14Show_TurnOffLamp(114);
  F14Show_TurnOffLamp(113);
  F14Show_TurnOnLamp(112);
  break;
case 45:
  F14Show_TurnOffLamp(115);
  F14Show_TurnOffLamp(110);
  break;
case 46:
  F14Show_TurnOffLamp(64);
  F14Show_TurnOffLamp(48);
  F14Show_TurnOnLamp(111);
  F14Show_TurnOnLamp(63);
  F14Show_TurnOffLamp(116);
  F14Show_TurnOffLamp(112);
  break;
  break;
case 47:
  F14Show_TurnOnLamp(40);
  F14Show_TurnOffLamp(111);
  F14Show_TurnOffLamp(63);
  break;
  break;
case 48:
  F14Show_TurnOffLamp(40);
  break;
case 49:
  F14Show_TurnOnLamp(55);
  F14Show_TurnOnLamp(3);
  F14Show_TurnOnLamp(1);
  break;
case 50:
  F14Show_TurnOnLamp(49);
  F14Show_TurnOnLamp(52);
  break;
case 51:
  F14Show_TurnOnLamp(32);
  F14Show_TurnOffLamp(55);
  F14Show_TurnOffLamp(3);
  F14Show_TurnOffLamp(1);
  F14Show_TurnOnLamp(2);
  F14Show_TurnOnLamp(56);
  F14Show_TurnOnLamp(50);
  F14Show_TurnOnLamp(53);
  break;
case 52:
  F14Show_TurnOffLamp(49);
  F14Show_TurnOnLamp(54);
  F14Show_TurnOffLamp(52);
  break;
case 53:
  F14Show_TurnOffLamp(32);
  F14Show_TurnOffLamp(2);
  F14Show_TurnOffLamp(56);
  F14Show_TurnOnLamp(51);
  F14Show_TurnOffLamp(50);
  F14Show_TurnOffLamp(53);
  break;
case 54:
  F14Show_TurnOffLamp(51);
  F14Show_TurnOffLamp(54);
  }
if (Step == 255) {
    if (step_timer) {
      KillTimer(step_timer);
      step_timer=0;
    }
  }
  else if (Step==54) {
    step_timer=ActivateTimer(25,0,F14_LampShowUpDown);
  }
  else {
    step_timer = ActivateTimer(25, Step + 1, F14_LampShowUpDown);
  }

}


void F14_LampShowCentrePulse (byte Step) {
static byte step_timer=0;
 
switch (Step) {
case 0:
  break;
case 1:
  break;
case 2:
  break;
case 3:
  F14Show_TurnOnLamp(3);
  F14Show_TurnOnLamp(1);
  F14Show_TurnOnLamp(110);
  F14Show_TurnOnLamp(112);
  break;
case 4:
  F14Show_TurnOnLamp(116);
  F14Show_TurnOnLamp(115);
  F14Show_TurnOnLamp(114);
  F14Show_TurnOnLamp(113);
  break;
case 5:
  break;
case 6:
  F14Show_TurnOnLamp(109);
  break;
case 7:
  F14Show_TurnOnLamp(5);
  F14Show_TurnOnLamp(2);
  F14Show_TurnOnLamp(7);
  break;
case 8:
  break;
case 9:
  F14Show_TurnOnLamp(49);
  break;
case 10:
  F14Show_TurnOnLamp(50);
  F14Show_TurnOnLamp(53);
  F14Show_TurnOnLamp(52);
  break;
case 11:
  F14Show_TurnOnLamp(54);
  break;
case 12:
  F14Show_TurnOnLamp(21);
  F14Show_TurnOnLamp(22);
  F14Show_TurnOnLamp(51);
  break;
case 13:
  F14Show_TurnOnLamp(23);
  F14Show_TurnOnLamp(20);
  break;
case 14:
  break;
case 15:
  F14Show_TurnOnLamp(19);
  F14Show_TurnOnLamp(28);
  F14Show_TurnOnLamp(29);
  F14Show_TurnOnLamp(30);
  break;
case 16:
  F14Show_TurnOnLamp(18);
  F14Show_TurnOnLamp(27);
  F14Show_TurnOnLamp(31);
  F14Show_TurnOnLamp(48);
  break;
case 17:
  F14Show_TurnOnLamp(55);
  break;
case 18:
  F14Show_TurnOnLamp(26);
  break;
case 19:
  F14Show_TurnOnLamp(17);
  F14Show_TurnOnLamp(64);
  F14Show_TurnOnLamp(37);
  F14Show_TurnOnLamp(38);
  break;
case 20:
  F14Show_TurnOnLamp(25);
  F14Show_TurnOnLamp(36);
  F14Show_TurnOnLamp(111);
  break;
case 21:
  F14Show_TurnOnLamp(105);
  break;
case 22:
  F14Show_TurnOnLamp(106);
  F14Show_TurnOnLamp(107);
  F14Show_TurnOnLamp(35);
  break;
case 23:
  F14Show_TurnOnLamp(32);
  F14Show_TurnOnLamp(104);
  F14Show_TurnOnLamp(108);
  F14Show_TurnOnLamp(34);
  F14Show_TurnOnLamp(33);
  break;
case 24:
  F14Show_TurnOnLamp(103);
  F14Show_TurnOnLamp(63);
  F14Show_TurnOnLamp(56);
  break;
case 25:
  break;
case 26:
  F14Show_TurnOnLamp(16);
  F14Show_TurnOnLamp(102);
  F14Show_TurnOnLamp(40);
  break;
case 27:
  break;
case 28:
  break;
case 29:
  F14Show_TurnOnLamp(4);
  break;
case 30:
  break;
case 31:
  break;
case 32:
  F14Show_TurnOffLamp(4);
  break;
case 33:
  break;
case 34:
  break;
case 35:
  F14Show_TurnOffLamp(16);
  F14Show_TurnOffLamp(102);
  F14Show_TurnOffLamp(40);
  break;
case 36:
  break;
case 37:
  F14Show_TurnOffLamp(103);
  F14Show_TurnOffLamp(63);
  F14Show_TurnOffLamp(56);
  break;
case 38:
  F14Show_TurnOffLamp(32);
  F14Show_TurnOffLamp(104);
  F14Show_TurnOffLamp(108);
  F14Show_TurnOffLamp(34);
  F14Show_TurnOffLamp(33);
  break;
case 39:
  F14Show_TurnOffLamp(106);
  F14Show_TurnOffLamp(107);
  F14Show_TurnOffLamp(35);
  break;
case 40:
  F14Show_TurnOffLamp(105);
  break;
case 41:
  F14Show_TurnOffLamp(25);
  F14Show_TurnOffLamp(36);
  F14Show_TurnOffLamp(111);
  break;
case 42:
  F14Show_TurnOffLamp(17);
  F14Show_TurnOffLamp(64);
  F14Show_TurnOffLamp(37);
  F14Show_TurnOffLamp(38);
  break;
case 43:
  F14Show_TurnOffLamp(26);
  break;
case 44:
  F14Show_TurnOffLamp(55);
  break;
case 45:
  F14Show_TurnOffLamp(18);
  F14Show_TurnOffLamp(27);
  F14Show_TurnOffLamp(31);
  F14Show_TurnOffLamp(48);
  break;
case 46:
  F14Show_TurnOffLamp(19);
  F14Show_TurnOffLamp(28);
  F14Show_TurnOffLamp(29);
  F14Show_TurnOffLamp(30);
  break;
case 47:
  break;
case 48:
  F14Show_TurnOffLamp(23);
  F14Show_TurnOffLamp(20);
  break;
case 49:
  F14Show_TurnOffLamp(21);
  F14Show_TurnOffLamp(22);
  F14Show_TurnOffLamp(51);
  break;
case 50:
  F14Show_TurnOffLamp(54);
  break;
case 51:
  F14Show_TurnOffLamp(50);
  F14Show_TurnOffLamp(53);
  F14Show_TurnOffLamp(52);
  break;
case 52:
  F14Show_TurnOffLamp(49);
  break;
case 53:
  break;
case 54:
  F14Show_TurnOffLamp(5);
  F14Show_TurnOffLamp(2);
  F14Show_TurnOffLamp(7);
  break;
case 55:
  F14Show_TurnOffLamp(109);
  break;
case 56:
  break;
case 57:
  F14Show_TurnOffLamp(116);
  F14Show_TurnOffLamp(115);
  F14Show_TurnOffLamp(114);
  F14Show_TurnOffLamp(113);
  break;
case 58:
  F14Show_TurnOffLamp(3);
  F14Show_TurnOffLamp(1);
  F14Show_TurnOffLamp(110);
  F14Show_TurnOffLamp(112);
  break;
case 59:
  break;
case 60:
  break;
case 61:
  break;
  }
  if (Step > 62) {
    F14_LampShowPlayer(2,255);  // Tell the player we are done
  }
  else {
    ActivateTimer(20, Step + 1, F14_LampShowCentrePulse);
  }
}
