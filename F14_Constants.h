
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

const byte ANIMATION_WEAPONS = 0;
const byte ANIMATION_BALL_LOCKED = 1;
const byte ANIMATION_LOCK_IS_LIT = 2;
const byte ANIMATION_START_MULTIBALL = 3;
const byte ANIMATION_LAUNCH_BONUS = 4;
const byte ANIMATION_SAFE_LANDING = 5;
const byte ANIMATION_START = 0;
const byte ANIMATION_END = 1;

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

const byte AWARD_BONUS = 0;

const byte ORBIT_RIGHT_SWITCH_HIT = 0;
const byte ORBIT_LEFT_SWITCH_HIT = 1;
const byte ORBIT_LIGHT_RIGHT_SIDE_BONUS = 2;
const byte ORBIT_LIGHT_LEFT_SIDE_BONUS = 3;
const byte ORBIT_LIGHT_BOTH_SIDE_BONUS = 4;
const byte ORBIT_BONUSX_TIMED_OUT = 5;
const byte ORBIT_MADE = 6;
const byte ORBIT_RESET = 7;
const byte ORBIT_ANTI_CLOCK_TIMEOUT = 9;
const byte ORBIT_CLOCK_TIMEOUT = 10;

const byte BONUS_INCREMENT = 0;
const byte BONUS_MULT_INCREMENT = 1;
const byte BONUS_LAMP_REFRESH = 2;

const byte LAUNCH_BONUS_SCORE = 1;
const byte LAUNCH_BONUS_LAMP_STROBE = 3;
const byte LAUNCH_BONUS_RESET = 4;

const byte LINE_OF_DEATH_HIT = 0;
const byte AWARD_KILL = 1;
const byte RESET_KILL_LAMPS = 2;
const byte LINE_OF_DEATH_STROBE = 3;

const byte ENABLE_LOCK = 0;
const byte LIGHT_LOCK_1 = 1;
const byte LIGHT_LOCK_2 = 2;
const byte LIGHT_LOCK_3 = 3;
const byte LOCK_BALL_IN_1 = 4;
const byte LOCK_BALL_IN_2 = 5;
const byte LOCK_BALL_IN_3 = 6;
const byte RESET_BACK_TO_LIT = 7;
const byte MULTIBALL_INTRO = 8;
const byte START_MULTIBALL = 9;
const byte LOWER_RAMP_ACTIVE = 30;
const byte MIDDLE_RAMP_ACTIVE = 31;
const byte UPPER_RAMP_ACTIVE = 32;
const byte BALL_ARRIVED_LOCK_1 = 22;
const byte BALL_ARRIVED_LOCK_2 = 23;
const byte BALL_ARRIVED_LOCK_3 = 21;