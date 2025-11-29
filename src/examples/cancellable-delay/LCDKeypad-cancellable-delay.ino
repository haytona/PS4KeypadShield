//////
// LCDKeypad example with a doScript() method for long running or repeating tasks
//
// The "arrow" keys will stay responsive even when script is running.
// Use the select key to start doScript. Press select again to cancel.
//
//////

//// INCLUDES:BEGIN

// SpinTimer library, https://github.com/dniklaus/spin-timer, 
//   add it by using the Arduino IDE Library Manager (search for spin-timer)
#include <SpinTimer.h>

// LcdKeypad, https://github.com/dniklaus/arduino-display-lcdkeypad, 
//   add it by using the Arduino IDE Library Manager (search for arduino-display-lcdkeypad)
#include <LcdKeypad.h>

// from SafeString library, https://github.com/PowerBroker2/SafeString
// use for non-blocking delay timer
#include <millisDelay.h>

//// INCLUDES:END

//// GLOBALS:BEGIN

const unsigned int SCRIPT_REPEATS = 10;

// cargo culted this line from docs
// I think so class can be instantiated
LcdKeypad* myLcdKeypad = 0; 

//// GLOBALS:END


// see inspiration here: https://forum.arduino.cc/t/calling-void-loop-as-a-function-inside-program/311764/12    
void mainLoop()
{
  scheduleTimers();  // Get the timer(s) ticked, in particular the LcdKeypad dirver's keyPollTimer
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
  String m_value;

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
        m_value = "U";
      }
      else if (LcdKeypad::DOWN_KEY == newKey)
      {
        m_value = "D";
      }
      else if (LcdKeypad::LEFT_KEY == newKey)
      {
        m_value = "L";
      }
      else if (LcdKeypad::RIGHT_KEY == newKey)
      {
        m_value = "R";
      }
      else if (LcdKeypad::SELECT_KEY == newKey)
      {
        m_value = "S"; // short for [S]elect or [S]tart or [S]top

        if ( not scriptRunning ) {
          m_lcdKeypad->setCursor(7, 1);
          m_lcdKeypad->print( "Starting  " );

          scriptRunning = true;
          cancelKeyPressed = false;


          doScript();


          scriptRunning = false; // reset as will here if cancelled or left to run
        } else {
          // we were running but select pressed again to cancel
          m_lcdKeypad->setCursor(7, 1);
          m_lcdKeypad->print( "CANCEL!   " );
          cancelKeyPressed = true;
          scriptRunning = false;
          // flow will return to the mainLoop called within doScript()
          // which will then return
        }
      }

      // print the char pressed. eg U = up
      m_lcdKeypad->setCursor(15, 0); // position cursor final char on 1st line
      m_lcdKeypad->print(m_value);   // print button
    }
  }

  // This is the workhorse of this code, it runs a sequence of events needed
  // to achieve the outcome on the device it is connected to.
  // Instead of delay() it uses cancellableDelay() which allows interruption by listening 
  // for cancelKeyPressed as configued up in handleKeyChanged 
  void doScript()
  {
    // this example pauses for 2s for each count
    // put your script here!
    for (int i = 1; i <= SCRIPT_REPEATS; i++) {
      //output counter for debugging
      m_lcdKeypad->setCursor(0, 1);
      m_lcdKeypad->print( String(i) + "of" + String(SCRIPT_REPEATS) + " " );

      if (cancellableDelay( 2000 )) return ;
    }
    m_lcdKeypad->setCursor(7, 1);
    m_lcdKeypad->print( "Finished  " );
  }

  // Will "delay" for the time passed in BUT it actually calls the mainLoop()
  // we have defined to replace the loop() to avoid recursion overflows
  // relies on global variable for key input having reset things
  //   cancelKeyPressed = boolean to know if it has been cancelled
  // Will return true if cancelled
  bool cancellableDelay( int delayTimeMs )
  {
    millisDelay cancelDelay;
    cancelDelay.start( delayTimeMs );

    //loop until time finished or cancelled
    while ( not cancelDelay.justFinished() ) {
      // Get the timer(s) ticked, in particular the LcdKeypad dirver's keyPollTimer
      mainLoop();

      //while we could use the keypad, this gives some
      //abstraction to set the var in the button code
      if ( cancelKeyPressed ) {
        cancelDelay.finish(); // cancel the timer
        return true;          // return as cancelled
      }
    }

    return false; // if we get here, it was NOT cancelled
  }

};
//// main class

void setup()
{
  myLcdKeypad = new LcdKeypad();  // instantiate an object of the LcdKeypad class, using default parameters
  
  // Attach the specific LcdKeypadAdapter implementation (dependency injection)
  myLcdKeypad->attachAdapter(new MyLcdKeypadAdapter(myLcdKeypad));
  
  myLcdKeypad->setCursor(0, 0);
  myLcdKeypad->print("LCDKeypad  key:");
}

