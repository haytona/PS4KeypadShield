//// PS4KeyPad Art of Rally
//
// Sketch to repeatedly crash car on Art of Rally to get trophy
//
// TODO: describe rally setup and driving settings
// Best to crash once, then use the script from there to repeat, repeat
//
// If the script bugs out and you're crashing into the barrier at low speed
// you can use the keypad to restart; left button mapped to "options" to bring up menu.
// Up and down work as intended and right button mapped to X.

//// INCLUDES:BEGIN

// SpinTimer library, https://github.com/dniklaus/spin-timer, 
//   add it by using the Arduino IDE Library Manager (search for spin-timer)
#include <SpinTimer.h>

// LcdKeypad, https://github.com/dniklaus/arduino-display-lcdkeypad, 
//   add it by using the Arduino IDE Library Manager (search for arduino-display-lcdkeypad)
#include <LcdKeypad.h>

// from SafeString library, https://github.com/PowerBroker2/SafeString
//   add it by using the Arduino IDE Library Manager (search for safestring)
// use for non-blocking delay timer
#include <millisDelay.h>

// PS4Arduino library, https://github.com/Flamethr0wer/PS4Arduino
//   add it by using the Arduino IDE Library Manager (search for ps4arduino)
//   - need to also flash the ____ as PS4 controller AVR
#include "PS4Arduino.h"

//// INCLUDES:END

//// GLOBALS:BEGIN

// 10 for initial testing, once you have it working
// apparently you need 800 after career to get 1,000 total
const unsigned int SCRIPT_REPEATS = 10;
// cargo culted this line from LcdKeypad docs
LcdKeypad* myLcdKeypad = 0; 
//// GLOBALS:END

//// begin blink//metronome
// used to monitor blocking as it won't flash consistently then
// think of it as a metronome (every BLINK_TIME_MILLIS)
const unsigned int  BLINK_TIME_MILLIS = 1000;

class BlinkTimerAction : public SpinTimerAction
{
public:
  void timeExpired()
  {
    bool isLedOn = digitalRead(LED_BUILTIN);
     digitalWrite(LED_BUILTIN, !isLedOn);
  }
};
//see aso setup() below
//// end blink//metronome

// this is where the actual action happens. Needed to avoid recursion
// see inspiration here: https://forum.arduino.cc/t/calling-void-loop-as-a-function-inside-program/311764/12    
void mainLoop()
{
  scheduleTimers();  // Get the timer(s) ticked, in particular the LcdKeypad dirver's keyPollTimer

  PS4controller.maintainConnection();
}

void loop()
{
  // do all the things in mainLoop so that it can be called from
  // the non-blocking delays elsewhere in code
  mainLoop();  // call to avoid recursion
}

//// main class
// Implement specific LcdKeypadAdapter in order to allow receiving key press events
class MyLcdKeypadAdapter : public LcdKeypadAdapter
{
private:
  LcdKeypad* m_lcdKeypad;
  String m_value;                // store button input

  bool scriptRunning = false;    // are we running the script?
  bool cancelKeyPressed = false; // do we need to cancel?

public:
  MyLcdKeypadAdapter(LcdKeypad* lcdKeypad)
  : m_lcdKeypad(lcdKeypad)
  , m_value(7)
  { }

  // Specific handleKeyChanged() method implementation - define your actions here
  void handleKeyChanged(LcdKeypad::Key newKey)
  {
    if (0 != m_lcdKeypad)
    {
      if (LcdKeypad::UP_KEY == newKey)
      {
        m_value = "U:U";
        PS4controller.setDpad(DPAD_N); delay(150); PS4controller.setDpad(DPAD_RELEASED);
      }
      else if (LcdKeypad::DOWN_KEY == newKey)
      {
        m_value = "D:D";
        PS4controller.setDpad(DPAD_S); delay(150); PS4controller.setDpad(DPAD_RELEASED);
      }
      else if (LcdKeypad::LEFT_KEY == newKey)
      {
        m_value = "L:O"; // (L)eft=(O)ptions
        PS4controller.setButton(OPTIONS, 1); delay(150); PS4controller.setButton(OPTIONS, 0);
      }
      else if (LcdKeypad::RIGHT_KEY == newKey)
      {
        m_value = "R:X"; // (R)ight=(X)
        // press X for 150ms
        PS4controller.setButton(CROSS, 1); delay(150); PS4controller.setButton(CROSS, 0);
      }
      else if (LcdKeypad::SELECT_KEY == newKey)
      {
        // SELECT key is start/stop for the script
        m_value = "S:$"; // (S)ELECT && ($)cript

        if ( not scriptRunning ) {
          log2("(re)start..");
          scriptRunning = true;
          cancelKeyPressed = false;
          doScript();
          scriptRunning = false; // reset as will here if cancelled or left to run
        } else {
          // we were running but select pressed again to cancel
          log2("stopped.");
          cancelKeyPressed = true;
          scriptRunning = false;
          // flow will return to the mainLoop called within doScript()
          // which will then return
        }
      }

      // log keypress// print the char pressed. eg U = up
      m_lcdKeypad->setCursor(13, 0); // position cursor top right
      m_lcdKeypad->print(m_value);   // print keypress
    }
  }

  // This is the workhorse of this code, it runs a sequence of events needed
  // to achieve the outcome on the device it is connected to.
  // It allows interruption by listening for the cancelKeyPressed var 
  void doScript()
  {
    for (int i = 1; i <= SCRIPT_REPEATS; i++) {

      //output loop iteration for monitoring/debugging
      m_lcdKeypad->setCursor(0, 1);
      m_lcdKeypad->print( String(i) + " " ); // hack to avoid padding once over 9

      // X to restart
      PS4controller.setButton(CROSS, 1); delay(150); PS4controller.setButton(CROSS, 0);
      if (cancellableDelay( 500 )) return ;

      // X to confirm restart
      PS4controller.setButton(CROSS, 1); delay(150); PS4controller.setButton(CROSS, 0);

      // 1-3sec race load
      if (cancellableDelay( 3000 )) return;
      // 3s race countdown
      // doesn't matter if we start after countdown as not setting course time
      if (cancellableDelay( 3000 )) return;

      // full throttle!/R2
      PS4controller.setTrigger(RIGHT, 255);
      if (cancellableDelay( 2500 )) return ;

      //slight left steer // appears to be more left than a possible "dead zone"??
      // 0 = full left, 128 = centre, 256 = full right
      PS4controller.setJoystick(LEFT, X, 96);  // left a bit
      if (cancellableDelay( 450 )) return ;    // wait
      PS4controller.setJoystick(LEFT, X, 128); // centre the steering
      if (cancellableDelay( 2500 )) return ;

      //slight right steer
      PS4controller.setJoystick(LEFT, X, 200);
      if (cancellableDelay( 250 )) return ;
      PS4controller.setJoystick(LEFT, X, 128);  // left joystick in center

      if (cancellableDelay( 2000 )) return ;
      // we crash about here!
      PS4controller.setTrigger(RIGHT, 0);       // R2/throttle fully released

      // wait for end of stage and menu to display
      if (cancellableDelay( 7000 )) return ;
   }
    log2("all done");
  } // /doScript

  // todo: make better name for this
  // Will "delay" for the time passed in BUT it actually calls the mainLoop()
  // we have defined to replace the loop() to avoid recursion overflows
  // relies on class variables for key input having reset things
  //   cancelKeyPressed = boolean to know if it has been cancelled
  //     this potentially could be improved by using vars passed by reference?
  bool cancellableDelay( int delayTimeMs )
  {
    // https://www.forward.com.au/pfod/ArduinoProgramming/SafeString/docs/html/classmillis_delay.html
    millisDelay cancelDelay;
    cancelDelay.start( delayTimeMs );

    //loop until finished
    while ( not cancelDelay.justFinished() ) {
      // Get the timer(s) ticked, in particular the LcdKeypad dirver's keyPollTimer
      mainLoop();

      if ( cancelKeyPressed ) {
        cancelDelay.finish(); // cancel the timer
        return true;          //return as cancelled
      }
    }

    return false; // if we get here, it was NOT cancelled
  } // / cancellableDelay

  // log on 2nd line
  // simple func to allow me to move the log message area later
  // in the class so can access m_lcdKeypad
  void log2( String msg)
  {
    m_lcdKeypad->setCursor(6, 1);
    m_lcdKeypad->print("          "); // clear it out first, easier then sprintf
    m_lcdKeypad->setCursor(6, 1);
    m_lcdKeypad->print( msg );
  }

};
//// main class

// must be below the class setup
void setup()
{
  myLcdKeypad = new LcdKeypad();  // instantiate an object of the LcdKeypad class, using default parameters
  
  // Attach the specific LcdKeypadAdapter implementation (dependency injection)
  myLcdKeypad->attachAdapter(new MyLcdKeypadAdapter(myLcdKeypad));
  
  myLcdKeypad->setCursor(0, 0);   // position the cursor at beginning of the first line
  myLcdKeypad->print("PS4Keypad in:");   // print a Value label on the first line of the display

  //blink//metronome
  pinMode(LED_BUILTIN, OUTPUT);
  new SpinTimer(BLINK_TIME_MILLIS, new BlinkTimerAction(), SpinTimer::IS_RECURRING, SpinTimer::IS_AUTOSTART);

  PS4controller.begin();
}

