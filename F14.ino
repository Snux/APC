// F14 Tomcat - APC code to play original WMS game

byte F14_TomcatTargets[5][12]; // track status of the 12 Tomcat targets for each player - 0 shot not made, 1 shot made
const byte F14TomcatTargetLampNumbers[12] = {33,34,35,36,37,38,49,50,51,52,53,54};  // The lamp for the corresponding target
const byte F14_1to6LampNumbers[6] = {116,115,114,113,112,110};
const byte F14_1to6SwitchNumbers[6] = {43,42,41,44,45,46};

const byte F14_LockedOnSeq[23] = {25,10,26,10,27,10,28,10,29,10,30,10,29,10,28,10,27,10,26,10,25,10,0};


byte F14_Kills[5];  // How many kills (Alpha -> Golf) has the player made
byte F14_LockStatus[5][3]; // status of locks for each player.  0 not active, 1 is lit, 2 is locked
byte F14_LockedBalls;
byte F14_NextLock=0;
byte F14_RescueKickerStatus; // status of outlane rescue 0 unlit, 1 lit, 2 grace period
byte F14_Bonus;
byte F14_Multiplier;
byte F14_GI_IsOn = 0;
byte F14_Diverter_Destination=0;


const byte F14_OutholeSwitch = 10;                      // number of the outhole switch
const byte F14_BallThroughSwitches[4] = {11,12,13,14};    // ball through switches from the plunger lane to the outhole
const byte F14_PlungerLaneSwitch = 16;
const byte F14_ACselectRelay = 14;                     // solenoid number of the A/C select relay - set it to 0 if the game doesn't have one
const byte F14_OutholeKicker = 1;                      // solenoid number of the outhole kicker
const byte F14_ShooterLaneFeeder = 2;                  // solenoid number of the shooter lane feeder
const byte F14_InstalledBalls = 4;                     // number of balls installed in the game
const byte F14_SearchCoils[15] = {1,3,5,7,10,13,20,0}; // coils to fire when the ball watchdog timer runs out - has to end with a zero
unsigned int F14_SolTimes[32] = {50,50,50,50,50,50,30,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,0,0,100,100,100,100,100,100,100,100}; // Activation times for solenoids


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

                                    // Duration..11111110...22222111...33322222...43333333...44444444...55555554...66666555
                                    // Duration..65432109...43210987...21098765...09876543...87654321...65432109...43210987
const struct LampPat F14_AttractPat1[57] ={{250,0b00000001,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000010,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000100,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00001000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00010000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00100000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b01000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b10000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000001,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000010,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000100,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00001000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00010000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00100000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b01000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b10000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000001,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000010,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000100,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00001000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00010000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00100000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b01000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b10000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000001,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000010,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000100,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00001000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00010000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00100000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b01000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b10000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000001,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000010,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000100,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00001000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00010000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00100000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b01000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b10000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000001,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000010,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000100,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00001000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00010000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00100000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b01000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b10000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000001},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000010},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000100},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00001000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00010000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00100000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b01000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b10000000},
                                          {0,0,0,0,0,0,0,0}};

const struct LampPat F14_AttractPat2[57] ={{250,0b00000001,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000010,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000100,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00001000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00010000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00100000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b01000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b10000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000001,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000010,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000100,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00001000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00010000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00100000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b01000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b10000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000001,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000010,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000100,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00001000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00010000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00100000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b01000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b10000000,0b00000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000001,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000010,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000100,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00001000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00010000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00100000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b01000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b10000000,0b00000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000001,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000010,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000100,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00001000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00010000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00100000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b01000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b10000000,0b00000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000001,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000010,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000100,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00001000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00010000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00100000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b01000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b10000000,0b00000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000001},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000010},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000100},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00001000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00010000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00100000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b01000000},
                                          {250,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b10000000},
                                          {0,0,0,0,0,0,0,0}};

const struct LampFlow F14_AttractFlow[3] = {{3,F14_AttractPat1},{10,F14_AttractPat2},{0,0}};

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
  //LEDsetColorMode(2);
  F14_GIOn(255,0,0);}                                        //switch on the gi

void F14_AttractMode() {                               // Attract Mode
  ACselectRelay = game_settings[F14set_ACselectRelay]; // assign the number of the A/C select relay
  if (ACselectRelay) {
    F14_SolTimes[ACselectRelay-1] = 0;}                // allow A/C relay to be turned on permanently
  DispRow1 = DisplayUpper;
  DispRow2 = DisplayLower;
  digitalWrite(VolumePin,HIGH);                       // set volume to zero
  LampPattern = NoLamps;
  Switch_Pressed = F14_AttractModeSW;
  Switch_Released = DummyProcess;
  AppByte2 = 0;
  LampReturn = F14_AttractLampCycle;
  ActivateTimer(1000, 0, F14_AttractLampCycle);
  F14_AttractDisplayCycle(0);}

void F14_AttractLampCycle(byte Event) {                // play multiple lamp pattern series
  UNUSED(Event);
  PatPointer = F14_AttractFlow[AppByte2].FlowPat;      // set the pointer to the current series
  FlowRepeat = F14_AttractFlow[AppByte2].Repeat;       // set the repetitions
  AppByte2++;                                         // increase counter
  if (!F14_AttractFlow[AppByte2].Repeat) {             // repetitions of next series = 0?
    AppByte2 = 0;}                                    // reset counter
  ShowLampPatterns(1);}                               // call the player

void F14_AttractDisplayCycle(byte Step) {
  F14_CheckForLockedBalls(0);
  switch (Step) {
  case 0:
    //F14_GIOn(255,0,0);
    WriteUpper2("  F14  TOMCAT   ");
    ActivateTimer(50, 0, ScrollUpper);
    WriteLower2("                ");
    ActivateTimer(1000, 0, ScrollLower2);
    if (NoPlayers) {                                  // if there were no games before skip the next step
      Step++;}
    else {
      Step = 2;}
    break;
  case 1:
    //F14_GIOn(0,255,0);
    WriteUpper2("                ");                  // erase display
    WriteLower2("                ");
    for (i=1; i<=NoPlayers; i++) {                    // display the points of all active players
      ShowNumber(8*i-1, Points[i]);}
    ActivateTimer(50, 0, ScrollUpper);
    ActivateTimer(900, 0, ScrollLower2);
    Step++;
    break;
  case 2:                                             // Show highscores
    //F14_GIOff();
    WriteUpper2("1>              ");
    WriteLower2("2>              ");
    for (i=0; i<3; i++) {
      *(DisplayUpper2+8+2*i) = DispPattern1[(HallOfFame.Initials[i]-32)*2];
      *(DisplayUpper2+8+2*i+1) = DispPattern1[(HallOfFame.Initials[i]-32)*2+1];
      *(DisplayLower2+8+2*i) = DispPattern2[(HallOfFame.Initials[3+i]-32)*2];
      *(DisplayLower2+8+2*i+1) = DispPattern2[(HallOfFame.Initials[3+i]-32)*2+1];}
    ShowNumber(15, HallOfFame.Scores[0]);
    ShowNumber(31, HallOfFame.Scores[1]);
    ActivateTimer(50, 0, ScrollUpper);
    ActivateTimer(900, 0, ScrollLower2);
    Step++;
    break;
  case 3:
    //F14_GIOn(0,0,255);
    //F14_GIOff();
    WriteUpper2("3>              ");
    WriteLower2("4>              ");
    for (i=0; i<3; i++) {
      *(DisplayUpper2+8+2*i) = DispPattern1[(HallOfFame.Initials[6+i]-32)*2];
      *(DisplayUpper2+8+2*i+1) = DispPattern1[(HallOfFame.Initials[6+i]-32)*2+1];
      *(DisplayLower2+8+2*i) = DispPattern2[(HallOfFame.Initials[9+i]-32)*2];
      *(DisplayLower2+8+2*i+1) = DispPattern2[(HallOfFame.Initials[9+i]-32)*2+1];}
    ShowNumber(15, HallOfFame.Scores[2]);
    ShowNumber(31, HallOfFame.Scores[3]);
    ActivateTimer(50, 0, ScrollUpper);
    ActivateTimer(900, 0, ScrollLower2);
    Step = 0;}
  ActivateTimer(4000, Step, F14_AttractDisplayCycle);}

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
    ShowLampPatterns(0);                              // stop lamp animations
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
      ShowLampPatterns(0);                            // stop lamp animations
      KillAllTimers();
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
      for (i=1; i < 5; i++) {
        LockedBalls[i] = 0;
        Points[i] = 0;
        F14_Kills[i]=0;  // How many kills (Alpha -> Golf) has the player made
        for (byte j=0; j<3; j++) {
          F14_LockStatus[i][j]=0;} // status of locks for each player.  0 not active, 1 is lit, 2 is locked
        for (byte j=0; j<12; j++) {
          F14_TomcatTargets[i][j]=0;}
        }
      F14_LineOfDeathHandler(1);  // sort the kill lamps out
      F14_RescueTargetHandler(0); // start the rescue target flip/flop
      F14_1to6Handler(1);       // start the 1-6 lamps
      F14_NewBall(game_settings[F14set_InstalledBalls]); // release a new ball (3 expected balls in the trunk)
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

  ShowAllPoints(0);
  F14_RescueKickerHandler(0);                         // Light the kickback at ball start
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
  ShowAllPoints(0);
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
    Serial.println("F14_CheckShooterLaneSwitch with switch ");            // print address reference table
    Serial.println((byte)Switch);
  }

  if (Switch == 16) { // shooter lane switch released?
    Switch_Released = DummyProcess;
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

  if (Switch > 15) {                                  // edit this to be true only for playfield switches
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
  if (QuerySwitch(game_settings[F14set_OutholeSwitch])) {
    BlockOuthole = false;
    ActivateTimer(1000, 0, F14_ClearOuthole);}
  else {
    if (QuerySwitch(game_settings[F14set_PlungerLaneSwitch])) { // if ball is waiting to be launched
      BallWatchdogTimer = ActivateTimer(30000, 0, F14_SearchBall);}  // restart watchdog
    else {                                            // if ball is really missing
      byte c = F14_CountBallsInTrunk();                // recount all balls
      if (c == game_settings[F14set_InstalledBalls]) { // found all balls in trunk?
        if (BlockOuthole) {                           // is the outhole blocked
          F14_BallEnd(0);}                             // then it was probably a ball loss gone wrong
        else {
          ActivateTimer(1000, game_settings[F14set_InstalledBalls], F14_NewBall);}} // otherwise try it with a new ball
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

byte F14_CountBallsInTrunk() {
  byte Balls = 0;
  for (i=0; i<game_settings[F14set_InstalledBalls]; i++) { // check how many balls are on the ball ramp
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
    ShowAllPoints(0);
    BlinkScore(1);
    ActA_BankSol(game_settings[F14set_ShooterLaneFeeder]);}
  byte c = F14_CountBallsInTrunk();
  if (c == Balls) {                                   // expected number of balls in trunk
    WriteUpper("  BALL MISSING  ");
    if (QuerySwitch(game_settings[F14set_OutholeSwitch])) { // outhole switch still active?
      ActA_BankSol(game_settings[F14set_OutholeKicker]);}}  // shove the ball into the trunk
  else {                                              //
    if (c == 5) {                                     // balls not settled
      WriteLower(" TRUNK  ERROR   ");
      Balls = 10;}
    else {
      if ((c > Balls) || !c) {                        // more balls in trunk than expected or none at all
        WriteUpper("                ");
        WriteLower("                ");
        ShowAllPoints(0);
        BlinkScore(1);
        ActA_BankSol(game_settings[F14set_ShooterLaneFeeder]);}}} // release again
  CheckReleaseTimer = ActivateTimer(5000, Balls, F14_CheckReleasedBall);}

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
    Serial.print(time_now);
    Serial.print(" game mode switch = ");            // print address reference table
    Serial.println((byte)Event);
  }
  switch (Event) {
  case 1:                                             // plumb bolt tilt
  case 2:                                             // ball roll tilt
  case 7:                                             // slam tilt
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
  case 9:                                            // playfield tilt
    WriteUpper(" TILT  WARNING  ");
    ActivateTimer(3000, 0, ShowAllPoints);
    break;
  case 3:                                             // credit button
    F14_AddPlayer();
    break;
  case 20:  // ramp entry
    F14_Divertor_Handler(0);
    break;
  case 21:                                            //right eject
  case 22:                                            //left
  case 23:                                            //centre
    F14_LockHandler(Event);
    break;  
  case 24:    //vuk
   
    ActivateTimer(200, 0, F14_vUKHandler);
    break;
  case 25:  // left rescue target
    F14_RescueTargetHandler(2);
    break;
  case 26:
    F14_RescueTargetHandler(3);
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
    F14_TomcatTargetHandler(0,Event-33);
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
  // Upper TOMCAT targets
  case 47:
    F14_OrbitHandler(0);
    break;
  case 49:
  case 50:
  case 51:
  case 52:
  case 53:
  case 54:
    F14_TomcatTargetHandler(0,Event-43);
    break;
  // Line of death (Yagov)
  case 55:
    F14_LineOfDeathHandler(0);
    break;
  case 56:
    F14_OrbitHandler(1);
    break;
  case 61:
    F14_RescueKickerHandler(1);
    break;
  case 65: // left slingshot
    ActivateSolenoid(0, 17);
    break;
  case 66: // right slingshot
    ActivateSolenoid(0, 18);
    break;
  case 68: // pop bumper
    ActivateSolenoid(0, 20);
    break;
  default:
    if (Event == game_settings[F14set_OutholeSwitch]) {
      ActivateTimer(200, 0, F14_ClearOuthole);}        // check again in 200ms
  }}

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

// Lock handler
// Event 0 - TOMCAT completed

void F14_LockHandler(byte Event) {
  static byte locks_available;

  if (APC_settings[DebugMode]){
    Serial.print("Lock Handler called, event =  ");            // print address reference table
    Serial.println((byte)Event);
  }
 

  locks_available = 0;
  for (byte i=0; i<3; i++) {
    if (F14_LockStatus[Player][i]==0) {
      locks_available++;
    }
  }

  switch (Event) {
    case 0:  // Completed TOMCAT, need to light a lock
      if (!locks_available)
        return;
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
      AddBlinkLamp(58,300);
      break;
    case 2: //enable lock 2
      F14_LockStatus[Player][1] = 1;
      AddBlinkLamp(57,300);
      break;
    case 3: //enable lock 3
      F14_LockStatus[Player][2] = 1;
      AddBlinkLamp(59,300);
      break;
    case 4: // Ball locked in 1
      F14_LockStatus[Player][0]=2;
      RemoveBlinkLamp(58);
      TurnOnLamp(58);
      InLock++;
      F14_GiveBall(1);
      break;
    case 5: // Ball locked in 2
      F14_LockStatus[Player][1]=2;
      RemoveBlinkLamp(57);
      TurnOnLamp(57);
      InLock++;
      F14_GiveBall(1);
      break;
    case 6: // Ball locked in 3
      F14_LockStatus[Player][2]=2;
      RemoveBlinkLamp(59);
      TurnOnLamp(59);
      InLock++;
      F14_GiveBall(1);
      break;
    case 31:  // middle ramp
      ReleaseSolenoid (22);
      break;
    case 32:  // upper ramp
      ReleaseSolenoid (21);
      break;
    case 22: // Lock number 1
      switch (F14_LockStatus[Player][0]) {
        case 0:
          ActivateSolenoid(0, 10);
          break;
        case 1:
          F14_LockHandler(4);
          break;
        case 2: // Switch for lock, when ball already locked.  Probably bouncy switch
          break;
      }
    case 23: // Lock number 2
      switch (F14_LockStatus[Player][1]) {
        case 0:
          ActA_BankSol(5);
          break;
        case 1:
          F14_LockHandler(5);
          break;
        case 2: // Switch for lock, when ball already locked.  Probably bouncy switch
          break;
      }
      break;
    case 21: // Lock number 3
      switch (F14_LockStatus[Player][2]) {
        case 0:
          ActA_BankSol(7);
          break;
        case 1:
          F14_LockHandler(6);
          break;
        case 2: // Switch for lock, when ball already locked.  Probably bouncy switch
          break;
      }
      break;
      

  }
}

// Handle bonus and bonus multiplier
// Event 0 - increment bonus
// Event 1 - increment multiplier
// Event 2 - update bonus and multiplier lamps
void F14_BonusHandler(byte Event){
  switch(Event) {
    case 0:
      if (F14_Bonus < 127) {  // max is 127, playfield can't display more
        F14_Bonus++;
        F14_BonusHandler(2);  // update the lamps
      }
      break;
    case 1:
      if (F14_Multiplier < 8) {
        F14_Multiplier++;
        F14_BonusHandler(2);  // update the lamps
      }
      break;
    case 2:
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

// Handle the line of death kickback
// Event 0 - switch activated, so kick the ball back and increment the kills
// Event 1 - just light the correct lamps based on the kill count (when switching players for example)
void F14_LineOfDeathHandler(byte Event) {
  switch (Event){
    case 0:
      ActivateSolenoid(0, 12);
      if (F14_Kills[Player]==7)
        return;
      F14_Kills[Player]++;
      TurnOnLamp(101+F14_Kills[Player]);
      F14_BonusHandler(1);
      break;
    case 1:
      for (byte i=1; i < 8; i++) {
        if (i > F14_Kills[Player])
          TurnOffLamp(i+101);
        else
          TurnOnLamp(i+101);
      break;
      }
  }
}

// Small handler for the spinner
// Event 0 = spinner hit
// Event 1 = light spinner 2k
// Event 2 = reset spinner 2k
void F14_SpinnerHandler(byte Event) {
  static byte spinner_2k_lit=0;

  switch (Event) {
    case 0:
      if (spinner_2k_lit) {
        Points[Player] += 2000;
      }
      else {
        Points[Player] += 10;
      }
      break;
    case 1:
      spinner_2k_lit = 1;
      TurnOnLamp(56);
      break;
    case 2:
      spinner_2k_lit = 0;
      TurnOffLamp(56);
      break;
  }
}

// Divertor handler
// Called when a ball is about to enter the ramp with the 2 diverters
// Decides where to route the ball based on the status of the locks
// Event 0 - incoming ball
// Event 1 - called when timer expires
void F14_Divertor_Handler(byte Event) {
  byte destination_lock;

  
  switch (Event) {
    case 0:
      // choose which lock to send the ball to.  This could do with being a little more random
      // At the moment, pick the first lock that is lit, otherwise choose something random
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

      switch (destination_lock) {
        case 0:
          ActivateSolenoid(1500,22);
          break;
        case 1:
          ActivateSolenoid(1500,21);
          break;
        case 2:
          break;
      }

  }

}

// Handle one of the TOMCAT targets being hit
// Event 0 is a hit target
// Event 1 is a spotted target (handle as 0 for now)
void F14_TomcatTargetHandler(byte Event, byte Target) {
  byte lit_target_count = 0;

  // Target is still flashing, so score it
  if (F14_TomcatTargets[Player][Target] == 0) {
    Points[Player] += 1000;
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
        F14_LockHandler(0); // Tell the lock handler we can light another lock
      }
      F14_TomcatTargetLamps();
      
  }
  else { // target has already been hit, just score 100
    Points[Player] += 100;
  }
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
      one_to_six_timer = ActivateTimer(1000,3,F14_1to6Handler);
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
        F14_SpinnerHandler(1); // increase spinner score to 2k
        F14_OrbitHandler(4);  // light left and right orbit bonusx
      }
      else {
        Points[Player] += 500;
      }
      break;

  }
}

// Some shots will spot a TOMCAT letter, this routine will find an unlit
// one and call the handler to light it
byte F14_SpotTomcat() {
  // Find the first unlit target, there will always be 1 as all 12 are never lit
  for (byte i=0; i<12; i++){
    if (F14_TomcatTargets[Player][i]==0)
      break;
  }
  F14_TomcatTargetHandler(1,i);
}


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
    case 0:  // switch at right side of orbit hit
      if (clockwise_timer) {
        KillTimer(clockwise_timer);
        clockwise_timer = 0;
        if (left_bonusX_lit) {
          F14_BonusHandler(1);
          F14_OrbitHandler(6);  // extend the timer
        }
        else {
          F14_OrbitHandler(3);
        }
      }
      else if (anti_clock_timer==0) {
        anti_clock_timer = ActivateTimer(1500, 9, F14_OrbitHandler);
      }

      break;
    case 1:  // switch at left side hit
      if (anti_clock_timer) {
        KillTimer(anti_clock_timer);
        anti_clock_timer = 0;
        if (right_bonusX_lit) {
          F14_BonusHandler(1);
          F14_OrbitHandler(6); // extend the timer
        }
        else {
          F14_OrbitHandler(2);
        }
      }
      else if (clockwise_timer==0) {
        clockwise_timer = ActivateTimer(1500, 10, F14_OrbitHandler);
      }
      break;
    case 2:
      right_bonusX_lit = 1;
      AddBlinkLamp(55,300);
      if (orbit_bonusx_timer) {
        KillTimer(orbit_bonusx_timer);
      }
      orbit_bonusx_timer = ActivateTimer(8000, 5, F14_OrbitHandler);
      break;
    case 3:
      left_bonusX_lit = 1;
      AddBlinkLamp(32,300);
      if (orbit_bonusx_timer) {
        KillTimer(orbit_bonusx_timer);
      }
      orbit_bonusx_timer = ActivateTimer(8000, 5, F14_OrbitHandler);
      
      break;
    case 4:
      left_bonusX_lit = 1;
      right_bonusX_lit = 1;
      AddBlinkLamp(32,300);
      AddBlinkLamp(55,300);
      if (orbit_bonusx_timer) {
        KillTimer(orbit_bonusx_timer);
      }
      orbit_bonusx_timer = ActivateTimer(8000, 5, F14_OrbitHandler);
      break;
    case 5: // timed out
      RemoveBlinkLamp(32);
      RemoveBlinkLamp(55);
      left_bonusX_lit = 0;
      right_bonusX_lit = 0;
      orbit_bonusx_timer = 0;
      break;
    case 6:  // orbit made while bonusx lit, reset the timer
      if (orbit_bonusx_timer) {
        KillTimer(orbit_bonusx_timer);
        orbit_bonusx_timer = 0;
      }
      orbit_bonusx_timer = ActivateTimer(5000, 5, F14_OrbitHandler);
    case 7:  // reset the orbit handler (new ball for example)
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
    case 9:
      anti_clock_timer = 0;
      break;
    case 10:
      clockwise_timer = 0;
      break;
  }
}

// Event 0 = start the flip/flop between rescue targets
// Event 1 = stop the flip/flop
// Event 2 = left target hit
// Event 3 = right target hit
// Event 4 = light left target
// Event 5 = light right target
void F14_RescueTargetHandler(byte Event){
  static byte lit_target = 0;  // 0 is left target, 1 is right target
  static byte rescue_target_timer = 0;
  switch(Event) {
    case 0:
      F14_RescueTargetHandler(4);  // just call the start for the left lamp
      break;
    case 1:  // kill the timer and switch the lamps off
      if (rescue_target_timer) {
          KillTimer(rescue_target_timer);
          rescue_target_timer = 0;
        }
        TurnOffLamp(5);
        TurnOffLamp(7);
      break;
    case 2:
      if (lit_target==0){
        F14_RescueKickerHandler(0); // left target hit and is lit, call the kicker handler
      }
      break;
    case 3:
      if (lit_target==1){
        F14_RescueKickerHandler(0); // right target hit and is lit, call the kicker handler      
      }
      break;
    case 4:  // Light left target
      lit_target = 0;
      TurnOnLamp(5);
      TurnOffLamp(7);
      rescue_target_timer = ActivateTimer(1000, 5, F14_RescueTargetHandler);  // move to right in 1 sec
      break;
    case 5:  // Light right target
      lit_target = 1;
      TurnOnLamp(7);
      TurnOffLamp(5);
      rescue_target_timer = ActivateTimer(1000, 4, F14_RescueTargetHandler);  // move to left in 1 sec
      break;

  }
}

// Handler for the rescue kickback (left outlane)
// Event 0 - light the kickback (called from F14_RescueTargetHandler) and on ball start
// Event 1 - ball hit outlane switch
// Event 2 - grace period expires
void F14_RescueKickerHandler(byte Event){
  static byte grace_timer=0;
  switch (Event){
    case 0:  // light kickback
      if (grace_timer) {  // kill the grace timer if we have one
        KillTimer(grace_timer);
        grace_timer = 0;
      }
      F14_RescueKickerStatus = 1;
      TurnOnLamp(8);  // switch on the outlane lamp
      break;
    case 1:  // ball in outlane
      switch (F14_RescueKickerStatus){
        case 1:
          ActivateSolenoid(0,13);  // Fire the kicker
          F14_RescueKickerStatus = 2;
          TurnOffLamp(8);  // Turn the lamp off
          AddBlinkLamp(8,40);  // then blink it
          grace_timer = ActivateTimer(2000, 2, F14_RescueKickerHandler);  // cancel in 2 secs
          break;
        case 2:
          ActivateSolenoid(0,13);  // if in grace period just fire kicker again
          break;
      }
      break;
    case 2:  // time is up
      grace_timer = 0;
      RemoveBlinkLamp(8);
      F14_RescueKickerStatus = 0;
      break;
  }
}

// This handles various things for the vUK (popper top right)
// The incoming event will specify what the routine is being called for
// 0 = simply eject
void F14_vUKHandler(byte Event) {
  switch (Event) {
    case 0:             // will need expanding when lock logic coded
      PlayFlashSequence((byte *)F14_LockedOnSeq);
      ActivateTimer(2000,1,F14_vUKHandler);
      break;
    case 1:  // Timer over, send the ball on its way
      ActA_BankSol(3);
      F14_Divertor_Handler(0);   // let the divertor know a ball is on the way
      break;
  }
}

// This handles various things for the right eject (the one under the shooter lane)
// The incoming event will specify what the routine is being called for
// 0 = simply eject


void F14_RightEjectHandler(byte Event) {
  switch (Event) {
    case 0:             // will need expanding when lock logic coded
      ActA_BankSol(7);
      break;
    
  }
}

// This handles various things for the centre eject (the one under the shooter lane)
// The incoming event will specify what the routine is being called for
// 0 = simply eject
void F14_CentreEjectHandler(byte Event) {
  switch (Event) {
    case 0:             // will need expanding when lock logic coded
      ActA_BankSol(5);
      break;
    
  }
}

// This handles various things for the left eject (the one under the shooter lane)
// The incoming event will specify what the routine is being called for
// 0 = simply eject
void F14_LeftEjectHandler(byte Event) {
  switch (Event) {
    case 0:             // will need expanding when lock logic coded
      ActivateSolenoid(0, 10);
      break;
    
  }
}

void F14_GIOn(byte Red, byte Green, byte Blue) {  // Colour not used at the moment
  //LEDsetColor(Red, Green, Blue);
  /*if (F14_GI_IsOn) {
    for (int i=65; i < 102; i++) {
      LEDchangeColor(i);}
  } else {
   
   if (APC_settings[DebugMode]){
    Serial.print("GI on colour = ");            // print address reference table
    Serial.print((byte)Red);
    Serial.print(",");
    Serial.print((byte)Green);
    Serial.print(",");
    Serial.println((byte)Blue);
    Serial.println(",");
  
  }*/
  for (int i=65; i < 102; i++) {
    TurnOnLamp(i);}
  //TurnOnLamp(115);
      
  
  F14_GI_IsOn = 1;
}

void F14_GIOff() {
  /* if (APC_settings[DebugMode]){
    Serial.println("GI Off");
  }*/
  for (int i=65; i < 102; i++) {TurnOffLamp(i);}
  F14_GI_IsOn = 0;
}


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
  if ((BallsInTrunk == 5)||(BallsInTrunk < game_settings[F14set_InstalledBalls]+1-Multiballs-InLock)) {
    InLock = 0;
//    if (Multiballs == 1) {
//      for (i=0; i<3; i++) {                           // Count your locked balls here
//        if (Switch[41+i]) {
//          InLock++;}}}
    WriteLower(" BALL   ERROR   ");
    if (QuerySwitch(game_settings[F14set_OutholeSwitch])) { // ball still in outhole?
      ActA_BankSol(game_settings[F14set_OutholeKicker]); // make the coil a bit stronger
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
    case 3:                                           // goto 2 ball multiball
      Multiballs = 2;
      if (BallsInTrunk != 1) {                        // not 1 ball in trunk
        ActivateTimer(1000, 0, F14_BallEnd);}          // check again later
      else {
        BlockOuthole = false;}                        // remove outhole block
      break;
    case 2:                                           // end multiball
      Multiballs = 1;
      if (BallsInTrunk == game_settings[F14set_InstalledBalls]) { // all balls in trunk?
        ActivateTimer(1000, 0, F14_BallEnd);}
      else {
        BlockOuthole = false;}                        // remove outhole block
      break;
    case 1:                                           // end of ball
      if (BallsInTrunk + InLock != game_settings[F14set_InstalledBalls]) {
        WriteUpper(" COUNT  ERROR   ");
        InLock = 0;
//        for (i=0; i<3; i++) {                       // check how many balls are on the ball ramp
//          if (Switch[41+i]) {
//            InLock++;}}
        ActivateTimer(1000, 0, F14_BallEnd);}
      else {
        LockedBalls[Player] = 0;
        BlinkScore(0);                                // stop score blinking
        F14_BallEnd2(BallsInTrunk);                    // add bonus count here and start BallEnd2 afterwards
      }}}}

void F14_BallEnd2(byte Balls) {
  if (BallWatchdogTimer) {
    KillTimer(BallWatchdogTimer);
    BallWatchdogTimer = 0;}
  if (ExBalls) {                                      // Player has extra balls
    ExBalls--;
    ActivateTimer(1000, AppByte, F14_NewBall);
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
