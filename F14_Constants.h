
//     ___________  _____              __            __    
//    / __<  / / / / ___/__  ___  ___ / /____ ____  / /____
//   / _/ / /_  _// /__/ _ \/ _ \(_-</ __/ _ `/ _ \/ __(_-<
//  /_/  /_/ /_/__\___/\___/_//_/___/\__/\_,_/_//_/\__/___/
//            /___/                                        
//
// The F14 code uses a number of constants to make the code a little
// more readable.  Rather than fill the main .ino file we just define
// them all here

const byte START_HANDLER = 0;  // start the handler
const byte QUIT_HANDLER=255;  // The handler should shut down any timers, switch off lamps etc

// Use these to make lamp show calls a little more readable
const byte LAMP_SHOW_ROTATE=0;
const byte LAMP_SHOW_UPDOWN=1;
const byte LAMP_SHOW_PULSE=2;
const byte LAMP_SHOW_TWINKLE=3;
const byte QUIT_SHOW=255;
const byte START_SHOW=0;

const byte SPINNER_HIT=0;
const byte SPINNER_LIGHT=1;
const byte SPINNER_RESET=2;

const byte RESCUE_TARGET_LEFT_HIT = 2;
const byte RESCUE_TARGET_RIGHT_HIT = 3;
const byte RESCUE_TARGET_LEFT_LIGHT = 4;
const byte RESCUE_TARGET_RIGHT_LIGHT = 5;

const byte RESCUE_OUTLANE_SWITCH_HIT=1;
const byte RESCUE_GRACE_PERIOD_TIMEDOUT=2;

const byte VUK_BALL_CAPTURED = 0;
const byte VUK_PLAY_FLASHERS = 1;
const byte VUK_EJECT = 2;
const byte VUK_CALL_FROM_LAUNCH_BONUS = 3;
const byte VUK_NOISY_SWITCH_TIMEOUT = 4;