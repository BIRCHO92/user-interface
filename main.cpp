#include <TM1637Display.h>
#include <Encoder.h>
#include <Led.h>
#include "OneButton.h"
#include <string.h>
#include <Wire.h>

#pragma region headers

/* Define display pins and make an array */
#define LCD_PIN_CLK 26 // Common clock pin for all TM1637 boards.
#define LCD_PIN_1 42   // Tidal vol
#define LCD_PIN_2 40   // Frequency
#define LCD_PIN_3 38   // IE
#define LCD_PIN_4 36   // Pmax
#define LCD_PIN_5 34   // Ptrig
#define LCD_PIN_6 28   // tv achieved
#define LCD_PIN_7 30   // PIP
#define LCD_PIN_8 32   // PEEP
const int DisplayPins[] = {LCD_PIN_1, LCD_PIN_2, LCD_PIN_3, LCD_PIN_4, LCD_PIN_5, LCD_PIN_6, LCD_PIN_7, LCD_PIN_8};
const uint8_t LCDbrightness = 7;
enum namesOfDisplays{ SetTidalVolume, SetFrequency, SetItoE, SetMaxPresure, SetTriggerPressure, AchievedVolume, AchievedPIP, AchievedPEEP };
const uint8_t nullSegments[] = {SEG_G, SEG_G, SEG_G, SEG_G};

/* Instantiate each display in an array of TM1637 objects. They will be initialised during setup. */
const int NumberOfDisplays = 8;
TM1637Display arrayOfDisplays[NumberOfDisplays];

/* Define SET PARAMETER LED pins and make an array */
#define LED_PIN_DISPLAY_1 27 // Tidal vol
#define LED_PIN_DISPLAY_2 29 // Frequency
#define LED_PIN_DISPLAY_3 31 // IE
#define LED_PIN_DISPLAY_4 33 // Pmax
#define LED_PIN_DISPLAY_5 35 // Ptrig
const int SetParameterLEDPins[] = {LED_PIN_DISPLAY_1, LED_PIN_DISPLAY_2, LED_PIN_DISPLAY_3, LED_PIN_DISPLAY_4, LED_PIN_DISPLAY_5};
const int NumberOfSetParameters = sizeof(SetParameterLEDPins) / sizeof(SetParameterLEDPins[0]);
Led arrayOfSetParameterLEDs[NumberOfSetParameters]; // Each of these LEDs is positioned next to a display.

/* Instantiate button objects */
OneButton startButton(4);   // ON
OneButton modeButton(5);    // START
OneButton defaultButton(6); // PAUSE
OneButton muteButton(A7);    // ALARM MUTE
OneButton selectButton(A0);  // SELECT (ENCODER BUTTON)

/* Crete a pointer array to allow looping through each button */
OneButton *arrayOfButtons[] = {&startButton, &modeButton, &defaultButton, &muteButton, &selectButton };
const int NumberOfButtons = sizeof(arrayOfButtons) / sizeof(arrayOfButtons[0]);
void CallWhenPressed(void *inButton);
void CallWhenUnpressed(void *inButton);
bool isButtonPressed[NumberOfButtons]; // Array of boolean values. This will be used to signal whether a button is being long pressed.
void CallWhenClicked(void *inButton);
bool isButtonClicked[NumberOfButtons]; // Array of boolean values. This will be used to signal whether a button has been short clicked.
enum namesOfButtons
{
    StartButton,
    ModeButton,
    DefaultButton,
    MuteButton,
    SelectButton
};

void ClearButtons(int new_mode = 123, int old_mode = 1234); // Set random values to avoid confusion, if no modes are passed. 
void ClearSetParameterLEDs();

/* Define and instantiate MODE LEDs */ // Issue: Move into LED Header
#define LED_PIN_RUN 8
#define LED_PIN_PAUSE 9
#define LED_PIN_VC 10
#define LED_PIN_PC 11
#define LED_PIN_DEFAULT_HIGH 12
#define LED_PIN_DEFAULT_MEDIUM 13
#define LED_PIN_DEFAULT_LOW 7

const int ArrayOfModeLEDPins[] = {LED_PIN_RUN, LED_PIN_PAUSE, LED_PIN_VC, LED_PIN_PC, LED_PIN_DEFAULT_HIGH, LED_PIN_DEFAULT_MEDIUM, LED_PIN_DEFAULT_LOW};
const int NumberOfModeLEDs = sizeof(ArrayOfModeLEDPins) / sizeof(ArrayOfModeLEDPins[0]);
Led arrayOfModeLEDs[NumberOfModeLEDs];
enum namesOfModeLeds
{
    RunLED,
    PauseLED,
    VCLed,
    PCLed,
    DefaultHighLED,
    DefaultMediumLED,
    DefaultLowLED
};

const uint8_t OffSegments[] = { 0,
	SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,           // O
	SEG_E | SEG_F | SEG_A | SEG_G,                           // F
	SEG_E | SEG_F | SEG_A | SEG_G                           // F
	};

/* Initialise Encoder */
Encoder mainEncoder(2, 3);
const int stepsPerDedent = 1; // Integer values that are incremented when the Encoder moves one dedent.
const int encoderSettingDirection = - 1;  // Swap directions here
const int encoderSelectingDirection = 1;
int encoderOutput, oldEncoderOutput, targetIndex, currentIndex;

/* Instantiate set and target variables each for the parameter and the parameter value */
int setParameterValues[NumberOfSetParameters]; //   The parameter values when confirmed and sent to main program
int targetParameterValues[NumberOfSetParameters]; // The paramater values as displayed whilst the user is changing them. 
int setParameterIndex = 0; // The parameter enum name or index used to select a parameter in above arrays. 
int targetParameterIndex = 0; // The enum name or index used when the user is selecting a different parameter
void DisplayReceivedParameterValues();

/* Define and instantiate the alarm LEDs, create array of state variables  */
#define LED_PIN_HIGH_PRES_ALARM A6
#define LED_PIN_LOW_PRES_ALARM A5
#define LED_PIN_LOW_MINUTE_VOL A4
#define LED_PIN_ELECTRONICS A3
#define LED_PIN_ALARM_MUTE  A2
const int ArrayOfAlarmLEDPins[] = {LED_PIN_HIGH_PRES_ALARM, LED_PIN_LOW_PRES_ALARM, LED_PIN_LOW_MINUTE_VOL, LED_PIN_ELECTRONICS, LED_PIN_ALARM_MUTE};
Led arrayOfAlarmLEDs[5];

const int numberOfAlarms = sizeof(ArrayOfAlarmLEDPins) / sizeof(ArrayOfAlarmLEDPins[0]);
bool isAlarmActive[numberOfAlarms] = {0, 0, 0, 0, 0}; 
void showAlarms();
void showAllAlarms();
void clearAllAlarms();

/* Initialise Interface, Operating and Ventilation Modes */
enum interfaceModes
{
    Selecting,
    Setting,
    Locked
};
int interfaceMode = Locked;
int newInterfaceMode = interfaceMode;
bool justChangedInterfaceMode = true;

enum operatingModes
{
    RunMode,
    PauseMode
};
int operatingMode = PauseMode;
int newOperatingMode = operatingMode;
bool justChangedOperatingMode = true;

enum ventilationModes
{
    VolumeControlMode,
    VolumeControlSetup,
    PressureControlMode,
    PressureControlSetup
};
int ventilationMode = VolumeControlMode;
int newVentilationMode = VolumeControlMode;
bool justChangedVentilationMode = true;
int isInPCMode = 0; // This is initially 0 and will become 1 if we change to either PC mode. 

enum defaultSettings
{
    DefaultLow,
    DefaultMedium,
    DefaultHigh,
    NoDefault
};

int defaultSetting = NoDefault;
int newDefaultSetting = defaultSetting;
void SetDefaultParameters(int ventilationMode, int defaultSetting);

const unsigned long TimeForInit = 2000; // Time for waterfall pattern during init.
void DisplayWaterfall(unsigned long TimeForInit);

unsigned long timeSinceIdle;
const unsigned long MaxTimeSinceIdle = 5000; // Display will lock after 5 seconds .
bool IsTimeToLock(unsigned long timeSinceIdle);

/* Initialise I2C */
#define DEVICE 8
void requestEvent();
void receiveEvent(int numberOfBytes);
const int NumberOfReceivedVals = 8;
int16_t receivedParameterValues[NumberOfReceivedVals];
uint8_t dataToSend[NumberOfSetParameters + 3]; // operating mode + ventilation mode + alarm mute + Set parameters 

#pragma endregion headers

#pragma region clinicalParameters

/* Initialise Clinical Parameters */

/* An array to allow lookup of the descrete set of values allowed for each parameter, in both VC and PC modes. */
const int LookupSetParameter[NumberOfSetParameters][2][4] =
    {
        // {VC, PC} {Initial value, Increment, Minimum value, Maximum value }
        {{300, 10, 200, 450}, // Tidal Volume / ml (VC)
         {200, 0, 200, 300}},      // Tidal Volume / ml (PC)

        {{12, 1, 5, 20}, // Frequency / min^-1 (VC)
         {12, 1, 5, 20}}, // (PC)

        {{12, 1, 11, 15}, // I/E ratio / 1:(value-10)
         {12, 1, 11, 15}},

        {{5, 1, 0, 60},  // Max Pressure (VC)
         {5, 1, 15, 30}}, // PC

        {{0, 1, 0, 5}, // Trigger pressure (VC)
         {0, 1, 0, 5}} // (PC)
    };
/* An array to allow lookup of the descrete set of default values allowed for each parameter, in both VC and PC modes. */
const int DefaultParameters[NumberOfSetParameters][2][3] =
    {
        // {VC, PC} {Small, Medium, Large}
        {{300, 350, 400}, // Tidal Volume / ml (VC)
         {250, 250, 250}},      // Tidal Volume / ml (PC)

        {{16, 14, 12}, // Frequency / min^-1 (VC)
         {16, 12, 10}}, // (PC)

        {{13, 12, 11}, // I/E ratio / 1:(value-10)
         {13, 12, 11}},

        {{30, 35, 40},  // Max Pressure (VC)
         {12, 15, 18}}, // PC

        {{0, 0, 0}, // Trigger pressure (VC)
         {0, 0, 0}} // (PC)
    };

enum
{
    TidalVolume,
    Frequency,
    ItoERatio,
    MaxPressure,
    TriggerPresure
};
enum
{
    InitialVal,
    IncrementVal,
    MinimumVal,
    MaximumVal
};

bool isFloat[NumberOfSetParameters] = {false, false, true, false, false}; // Tidal volume and frequency are not floats, and IE as float to display 1:.
#pragma endregion clinicalParameters

/* Start Setup */
void setup() {
    Serial.begin(9600);
    Serial.println("Setup...");
    /* Initialise the arrays of LCD, LED and button objects, and switch the LEDs off. */
    for (int i = 0; i < NumberOfDisplays; i++) {
        arrayOfDisplays[i].init(LCD_PIN_CLK, DisplayPins[i]);
        arrayOfDisplays[i].setBrightness(LCDbrightness, true);
        arrayOfDisplays[i].showNumberDecEx(8888, false);
    }
    for (int i = 0; i < NumberOfSetParameters; i++) {
        arrayOfSetParameterLEDs[i].init(SetParameterLEDPins[i], false);
        arrayOfSetParameterLEDs[i].off();
    }
    for (int i = 0; i < NumberOfModeLEDs; i++) {
        arrayOfModeLEDs[i].init(ArrayOfModeLEDPins[i], false);
        arrayOfModeLEDs[i].off();
    }
    for (int i = 0; i < numberOfAlarms; i++) {
        arrayOfAlarmLEDs[i].init(ArrayOfAlarmLEDPins[i], false);
        arrayOfAlarmLEDs[i].off();
    }
    for (int i = 0; i < NumberOfButtons; i++ ) {
        arrayOfButtons[i]->attachClick( CallWhenClicked, &isButtonClicked[i] );
        arrayOfButtons[i]->attachLongPressStart( CallWhenPressed, &isButtonPressed[i] );
        arrayOfButtons[i]->attachLongPressStop( CallWhenUnpressed, &isButtonPressed[i] );
    }

    showAllAlarms();
    /* Setup I2C */
    Wire.begin(DEVICE);
    Wire.onRequest(requestEvent);
    Wire.onReceive(receiveEvent);
    DisplayWaterfall(TimeForInit);
    clearAllAlarms();
    /* Set parameters from DefaultHigh on startup, but do not start in default high mode */ 
    SetDefaultParameters(ventilationMode, DefaultMedium);
    arrayOfDisplays[TriggerPresure].setSegments(OffSegments);

} // End of Setup

void loop() {
    /* Update Button States */ 
    for (int i = 0; i < NumberOfButtons; i++) {
        arrayOfButtons[i]->tick();
    }

    if (operatingMode != RunMode ) { // Only change default settings if not running 
    /*  Default settings state machine */
        switch (defaultSetting) {
            case DefaultHigh:
                arrayOfModeLEDs[DefaultHighLED].on();
                arrayOfModeLEDs[DefaultLowLED].off();
                arrayOfModeLEDs[DefaultMediumLED].off();
                if (isButtonClicked[DefaultButton]) { newDefaultSetting = DefaultMedium; }
                break;
            case DefaultMedium:
                arrayOfModeLEDs[DefaultHighLED].off();
                arrayOfModeLEDs[DefaultMediumLED].on();
                arrayOfModeLEDs[DefaultLowLED].off();
                if (isButtonClicked[DefaultButton]) { newDefaultSetting = DefaultLow; }
                break;
            case DefaultLow:
                arrayOfModeLEDs[DefaultHighLED].off();
                arrayOfModeLEDs[DefaultMediumLED].off();
                arrayOfModeLEDs[DefaultLowLED].on();
                if (isButtonClicked[DefaultButton]) { newDefaultSetting = NoDefault; }
                break;
            case NoDefault:
                arrayOfModeLEDs[DefaultHighLED].off();
                arrayOfModeLEDs[DefaultMediumLED].off();
                arrayOfModeLEDs[DefaultLowLED].off();
                if (isButtonClicked[DefaultButton]) { newDefaultSetting = DefaultHigh; }
                break;
        }; // End of default settings state machine
    
        /* If the default state changes, set the defaults (if required) before switching */ 
        if ( newDefaultSetting != defaultSetting) {
            ClearButtons(newDefaultSetting, defaultSetting);
            defaultSetting = newDefaultSetting;
            SetDefaultParameters(ventilationMode, defaultSetting); // Display and set the chosen default values. 
            Serial.println();
            Serial.print("New default mode = ");
            Serial.print(defaultSetting);
            Serial.println();
        }
     }
    /* If we are in Run mode, then switch off all default lights after a setting is changed  */
    else if ( newDefaultSetting == NoDefault ) { 
        defaultSetting = NoDefault;
        arrayOfModeLEDs[DefaultHighLED].off();
        arrayOfModeLEDs[DefaultMediumLED].off();
        arrayOfModeLEDs[DefaultLowLED].off();
    }

    /*  Interface Mode state machine **/
	switch (interfaceMode) {
        case Locked:
//          Start here, revert here if idle timeout, or if ventilation is confirmed. 
            if (justChangedInterfaceMode) {
                justChangedInterfaceMode = false;
//              Turn off all display LEDs
                ClearSetParameterLEDs();
            }

//          React to button press
            if (isButtonClicked[SelectButton] || isButtonPressed[SelectButton]) {
//              Select button is pressed to unlock -> mode SELECTING
                newInterfaceMode = Selecting;
            }
            break;

        /* Select Parameter to change **/
        case Selecting:
            if (justChangedInterfaceMode) { // If switched to this mode
                justChangedInterfaceMode = false;
                timeSinceIdle = millis();
//              Turn off all display LEDs and turn on the one of the target Param
                ClearSetParameterLEDs();
                arrayOfSetParameterLEDs[targetParameterIndex].on();
            }

            /* Get target parameter **/
            encoderOutput = mainEncoder.read();
            if ( mainEncoder.read() != 0 ) { // Then the target param has been changed ...
                mainEncoder.write(0);       // Reset the encoder 
                timeSinceIdle = millis();   // Reset lock timer
                /* Change the target parameter and scroll the parameter LEDs */
                arrayOfSetParameterLEDs[targetParameterIndex].off(); 
                targetParameterIndex = targetParameterIndex + encoderSelectingDirection*encoderOutput / stepsPerDedent ;  
            /*  Create hard stops at each end of the parameter list 
                If in PC / setup mode, do not allow selection of VT. */
                if ( ventilationMode == PressureControlMode || ventilationMode == PressureControlSetup) { 
                    if ( targetParameterIndex < 1 ) { targetParameterIndex = 1; }   // Hard stop at bottom of list
                }
                else if (targetParameterIndex < 0 ) {targetParameterIndex = 0;} // Allow all values if not in VC modes 
                if ( targetParameterIndex > NumberOfSetParameters - 1 ) {targetParameterIndex = NumberOfSetParameters - 1;} // Hard stop at top of list.
                arrayOfSetParameterLEDs[targetParameterIndex].on();
            }

            /* React to button  */
            if (isButtonClicked[SelectButton] || isButtonPressed[SelectButton]) {
//              this target param is to be set -> mode SETTING
                setParameterIndex = targetParameterIndex;
                newInterfaceMode = Setting;
            } else if ( IsTimeToLock(timeSinceIdle) ) {
//              Lock interface (long idle time or lock button) -> mode LOCKED
                newInterfaceMode = Locked;
            }
            break;

        /* Set a param value **/
        case Setting:
//          If switched to this mode
            if (justChangedInterfaceMode) { 
                justChangedInterfaceMode = false;
            }
            ClearSetParameterLEDs(); // 
            arrayOfSetParameterLEDs[int(setParameterIndex)].blink(120); // Blink LED corresponding to Param to change.

            encoderOutput = mainEncoder.read();
            if ( mainEncoder.read() != 0 ) { // Then the target param has been changed ...
                mainEncoder.write(0);       // Reset the encoder 
                timeSinceIdle = millis();   // reset lock timer 
                int numberOfIncrements =  (LookupSetParameter[setParameterIndex][isInPCMode][MaximumVal]  // ...
                                  - LookupSetParameter[setParameterIndex][isInPCMode][MinimumVal])  // ...
                                  / LookupSetParameter[setParameterIndex][isInPCMode][IncrementVal] + 1;
                    //  Find the current index.
                currentIndex = ( targetParameterValues[setParameterIndex] - LookupSetParameter[setParameterIndex][isInPCMode][MinimumVal] )  // ...
                                    / LookupSetParameter[setParameterIndex][isInPCMode][IncrementVal];
                targetIndex = ( encoderSettingDirection*encoderOutput / stepsPerDedent + currentIndex ) ;
                if (targetIndex < 0 ) {targetIndex = 0;}
                if (targetIndex >= numberOfIncrements) {targetIndex = numberOfIncrements - 1;}
//              Calculate new Value according to parameter range (defined by ventilation mode) and target index. 
                targetParameterValues[setParameterIndex] = LookupSetParameter[setParameterIndex][isInPCMode][MinimumVal]
                                                        + LookupSetParameter[setParameterIndex][isInPCMode][IncrementVal] * targetIndex;
    //          Show current value
                arrayOfDisplays[setParameterIndex].clear();
                arrayOfDisplays[setParameterIndex].showNumberDecEx(targetParameterValues[setParameterIndex], isFloat[setParameterIndex]);
                if ( targetParameterValues[TriggerPresure] == 0 ) { arrayOfDisplays[TriggerPresure].setSegments(OffSegments); }
            }

//          React to button press
            if (isButtonClicked[SelectButton]) {
//              Set the param value to the new one and display it -> mode SELECTING
                setParameterValues[setParameterIndex] = targetParameterValues[setParameterIndex];
                newInterfaceMode = Selecting;
                newDefaultSetting = NoDefault; // Switch off all default lights.  
            } else if (isButtonPressed[SelectButton]) {
//              Abort changing of value -> mode SELECTING
                targetParameterValues[setParameterIndex] = setParameterValues[setParameterIndex] ;
                newInterfaceMode = Selecting;
            } else if ( IsTimeToLock(timeSinceIdle) ) {
//              Lock interface (long idle time) -> mode LOCKED
                newInterfaceMode = Locked;
//              Reset and display the old value. 
                targetParameterValues[setParameterIndex] = setParameterValues[setParameterIndex];
            }
            break;
    }; // End of Interface Mode state machine

    /*  If the interface mode changes, reset everything before switching */
    if (newInterfaceMode != interfaceMode ) {
        ClearButtons(newInterfaceMode, interfaceMode);
        ClearSetParameterLEDs();
        if (interfaceMode == Setting) {     // Only show set value if we are changing interface mode, to avoid this when changing ventilation mode
            arrayOfDisplays[setParameterIndex].clear();
            arrayOfDisplays[setParameterIndex].showNumberDecEx(setParameterValues[setParameterIndex], isFloat[setParameterIndex]);
        }
        interfaceMode = newInterfaceMode;
        justChangedInterfaceMode = true;
        mainEncoder.write(0);
        timeSinceIdle = millis();
        if ( setParameterValues[TriggerPresure] == 0 ) { arrayOfDisplays[TriggerPresure].setSegments(OffSegments); }
    }

    /*  Operating Mode state machine */
    switch (operatingMode) {

        case RunMode:
//          Only OnLED is on
            arrayOfModeLEDs[RunLED].on();
            arrayOfModeLEDs[PauseLED].off();
//          New operating mode: PAUSE if StartButton is pressed,  otherwise stay in RUN
            newOperatingMode = isButtonPressed[StartButton]? PauseMode : RunMode;
            break;

        case PauseMode:
//          Only PauseLED is on.
            arrayOfModeLEDs[RunLED].off();
            arrayOfModeLEDs[PauseLED].on();
//          New operating mode: Run if Startbutton is Pressed
            newOperatingMode = isButtonPressed[StartButton]? RunMode : PauseMode ;
//          Reset button Presses
            break;
    } // End of operating mode state machine

    /*  If operating mode has changed, clear buttons before switching. */
    if (newOperatingMode != operatingMode ) {
        ClearButtons(newOperatingMode, operatingMode); 
        operatingMode = newOperatingMode; 
    }

    /* Ventilation Mode state machine */
    switch(ventilationMode) {
//      React to button press
        case VolumeControlSetup:
            if (justChangedVentilationMode) {            
                justChangedVentilationMode = false;  
                arrayOfModeLEDs[PCLed].off();
                newInterfaceMode = Setting;                                   
                justChangedInterfaceMode = true;
                setParameterIndex = targetParameterIndex = MaxPressure;         // Prompt user to change max pressure 
                targetParameterValues[MaxPressure] = 35;                        // Suggest a target max pressure but do not change set value
                arrayOfDisplays[MaxPressure].clear();                           // Show the value
                arrayOfDisplays[MaxPressure].showNumberDecEx(targetParameterValues[MaxPressure]);
                arrayOfDisplays[TidalVolume].clear();                           // Erase Tv value from display
                arrayOfDisplays[TidalVolume].showNumberDecEx(setParameterValues[TidalVolume]);
            }
            arrayOfModeLEDs[VCLed].blink(100);
            if (operatingMode != RunMode) { newVentilationMode = VolumeControlMode; }
            else { 
                if ( isButtonClicked[ModeButton] ) { 
                    newVentilationMode = VolumeControlMode;             // If user confirms vent mode change, update set value
                    setParameterValues[setParameterIndex] = targetParameterValues[setParameterIndex];
                    }
                if ( isButtonPressed[ModeButton] ) {
                    newVentilationMode = PressureControlMode;           // If user cancels vent mode change, reset and print the old set value. 
                    targetParameterValues[MaxPressure] = setParameterValues[MaxPressure];
                    arrayOfDisplays[MaxPressure].clear();                           // Show the value
                    arrayOfDisplays[MaxPressure].showNumberDecEx(targetParameterValues[MaxPressure]);
                }
                // newVentilationMode = isButtonClicked[ModeButton]? VolumeControlMode : isButtonPressed[ModeButton]? PressureControlMode : VolumeControlSetup ;
            }
        break;

        case VolumeControlMode:
            if (justChangedVentilationMode) {   
                justChangedVentilationMode = false;         
                arrayOfModeLEDs[VCLed].on();
                arrayOfModeLEDs[PCLed].off();
                arrayOfDisplays[TidalVolume].clear();
                arrayOfDisplays[TidalVolume].showNumberDecEx(setParameterValues[TidalVolume]);
            }
            newVentilationMode = ( isButtonClicked[ModeButton] || isButtonPressed[ModeButton])? PressureControlSetup : VolumeControlMode ; 
//          Ventilation Mode button is pressed, mode ->  Pressure Control        
        break;

        case PressureControlSetup:
            if (justChangedVentilationMode) {            
                justChangedVentilationMode = false;
                arrayOfModeLEDs[VCLed].off();
                newInterfaceMode = Setting;
                justChangedInterfaceMode = true;
                setParameterIndex = targetParameterIndex = MaxPressure;
                targetParameterValues[MaxPressure] = 15;
                arrayOfDisplays[MaxPressure].clear();
                arrayOfDisplays[MaxPressure].showNumberDecEx(targetParameterValues[MaxPressure]);
            }
            arrayOfModeLEDs[PCLed].blink(100);
            arrayOfDisplays[TidalVolume].setSegments( nullSegments );  
            if (operatingMode != RunMode) { newVentilationMode = PressureControlMode; }
            else {
                if ( isButtonClicked[ModeButton] ) { 
                    newVentilationMode = PressureControlMode;             // If user confirms vent mode change, update set value
                    setParameterValues[setParameterIndex] = targetParameterValues[setParameterIndex];
                    }
                if ( isButtonPressed[ModeButton] ) {
                    newVentilationMode = VolumeControlMode;           // If user cancels vent mode change, reset and print the old set value. 
                    targetParameterValues[MaxPressure] = setParameterValues[MaxPressure];
                    arrayOfDisplays[MaxPressure].clear();                           // Show the value
                    arrayOfDisplays[MaxPressure].showNumberDecEx(targetParameterValues[MaxPressure]);
                }
                // newVentilationMode = isButtonClicked[ModeButton]? PressureControlMode : isButtonPressed[ModeButton]? VolumeControlMode : PressureControlSetup ;
            }
        break;

        case PressureControlMode:
            if (justChangedVentilationMode) {
                justChangedVentilationMode = false;
                arrayOfModeLEDs[PCLed].on();
                arrayOfModeLEDs[VCLed].off();
                setParameterValues[MaxPressure] = targetParameterValues[MaxPressure];
            }    
            arrayOfDisplays[TidalVolume].setSegments( nullSegments ) ;
            newVentilationMode = ( isButtonClicked[ModeButton] || isButtonPressed[ModeButton] )? VolumeControlSetup : PressureControlMode ;
        break;
    }

    /* If ventilation mode has changed */
    if (newVentilationMode != ventilationMode ) {
        ClearButtons(newVentilationMode, ventilationMode);
        ventilationMode = newVentilationMode;
        isInPCMode = ventilationMode/2 ; // This is 0 if in VC mode or VC setup, 1 if in PC or PC setup. Used as index for lookup table. 
        justChangedVentilationMode = true;
        // timeSinceIdle = millis();
        Serial.println();
        Serial.print("New ventilation mode = ");
        Serial.print(ventilationMode);
        Serial.println();
    }

//  Display Values from Ventilator
    DisplayReceivedParameterValues();

//  Fill up an array of 8 bit values to send over I2C
    dataToSend[0] = uint8_t(operatingMode); // 0: RunMode, 1: PauseMode
    dataToSend[1] = uint8_t(ventilationMode); // 0: VolumeControlMode, 1:VolumeControlSetup, 2: PressureControlMode, 3: PressureControlSetup
    dataToSend[2] = uint8_t( isButtonClicked[MuteButton] || isButtonPressed[MuteButton] ); // if muted
    for (int i = 0; i < NumberOfSetParameters ; i++) {
        dataToSend[i+3] = setParameterValues[i] ;
        dataToSend[3] = setParameterValues[0]/10;   // Tidal volume needs to be scaled
    }

} // End of Loop

void ClearButtons(int new_mode, int old_mode) {
    /**
    * Set all button array states to false to avoid presses or clicks
    * carrying through state machine changes.
    * Compare the new and old modes, to enable printing the new mode just once. 
    */

    if (new_mode != old_mode) {
        for (int i = 0; i < NumberOfButtons; i++) {
            isButtonPressed[i] = isButtonClicked[i] = false;
        }
    }
}

void ClearSetParameterLEDs() {
        /**
    * Switch off all the LEDs next to each set prameter display.
    */
    for (int i = 0; i < NumberOfSetParameters; i++ )
        arrayOfSetParameterLEDs[i].off();
}

void CallWhenClicked(void *inButton) {
    /**
    * Set the button array state of using pointers
    */

    bool *i;
    i = (bool*)inButton;
    *i = true;
    Serial.println("Clicked");
    // timeSinceIdle = millis();
}

void CallWhenPressed(void *inButton) {
    /**
    * Set the button array state of using pointers
    */

    bool* i;
    i = (bool*)inButton;
    *i = true;
    Serial.println("Pressed");
}

void CallWhenUnpressed(void *inButton){
    /**
    * Set the button array state of using pointers
    */

    bool* i; i = (bool*)inButton;
    *i = false;
}

void SetDefaultParameters( int ventilationMode, int defaultSetting ) {
    /**
    * Display and set each of the setable parameters to default values, according to the current ventilation mode and default mode setting. 
    * If no default mode is selected then display the setparametes. 
    */
    for (int i = isInPCMode; i < NumberOfSetParameters - 1; i++) { // Update all except P_trig, and Tv if in PC mode
        arrayOfDisplays[i].clear();
        if ( defaultSetting == NoDefault ) {
            // targetParameterValues[i] = setParameterValues[i] = DefaultParameters[i][isInPCMode][defaultSetting];
            arrayOfDisplays[i].showNumberDecEx( setParameterValues[i] , isFloat[i] );
        }
        else {
            targetParameterValues[i] = setParameterValues[i] = DefaultParameters[i][isInPCMode][defaultSetting];
            arrayOfDisplays[i].showNumberDecEx( setParameterValues[i] , isFloat[i]);
        } 
        if (isInPCMode == 1) { arrayOfDisplays[TidalVolume].setSegments(nullSegments); }
        arrayOfDisplays[TriggerPresure].setSegments(OffSegments);
    }
}

void DisplayReceivedParameterValues() {
    /**
    * Display the array of received parameters on the lower 3 displays.
    */

    arrayOfDisplays[AchievedVolume].showNumberDecEx(receivedParameterValues[0], false);
    arrayOfDisplays[AchievedPIP].showNumberDecEx(round(receivedParameterValues[1]), true);
    arrayOfDisplays[AchievedPEEP].showNumberDecEx(receivedParameterValues[2], true);
}

void showAlarms() {
    /*
     * Indicate if alarm occurred.
     */
    for ( int i = 0; i < numberOfAlarms; i++) {
        isAlarmActive[i] = bool( receivedParameterValues[i + 3] ); //  The recieved alarm status are index 3-7 of the received data. 
        if ( isAlarmActive[i] ) {
            if ( isAlarmActive[4] ) { arrayOfAlarmLEDs[4].on(); }
            else {arrayOfAlarmLEDs[i].blink(120); } 
            } //  If true blink
        else arrayOfAlarmLEDs[i].off(); //  Otherwise switch off. 
    }
}

void showAllAlarms() {
    for ( int i = 0; i < numberOfAlarms; i++) {
        arrayOfAlarmLEDs[i].on();
    }
}
void clearAllAlarms() {
    for ( int i = 0; i < numberOfAlarms; i++) {
        arrayOfAlarmLEDs[i].off();
    }
}

bool IsTimeToLock(unsigned long timeSinceIdle) {
/*
    Simply return true if it is time to lock the interface.
*/

    return (millis() - timeSinceIdle > MaxTimeSinceIdle);
}

void DisplayWaterfall(unsigned long TimeForInit) {
    /**
    * Display a moving pattern of hyphens on the displays and LEDs for the given number of millis.
    * Clear them all at the end.
    */
    int8_t counter = -1;
    uint8_t hyphens[] = {0, SEG_A, SEG_G, SEG_D};
    arrayOfSetParameterLEDs[0].on();
    arrayOfModeLEDs[0].off();
    unsigned long timer = millis();

//  Display Waterfall
    while (millis() - timer < TimeForInit) {
        for (TM1637Display Display : arrayOfDisplays) {
            Display.setSegments(hyphens);
        }
        for (int i = 0; i < 4; i++) {
            hyphens[i] = hyphens[ (i+1)%4 ];
        }
        arrayOfSetParameterLEDs[counter%NumberOfSetParameters].off();
        arrayOfModeLEDs[counter%NumberOfModeLEDs].off();
        // arrayOfAlarmLEDs[counter%numberOfAlarms].off();
        counter++;
        arrayOfSetParameterLEDs[counter%NumberOfSetParameters].on();
        arrayOfModeLEDs[counter%NumberOfModeLEDs].on();
        // arrayOfAlarmLEDs[counter%numberOfAlarms].on();
    }

//  Turn off LEDs
    arrayOfSetParameterLEDs[counter%NumberOfSetParameters].off();
    arrayOfModeLEDs[counter%NumberOfModeLEDs].off();
    // arrayOfAlarmLEDs[counter%numberOfAlarms].off();
    for (int i = 0; i < NumberOfDisplays; i++) {
        arrayOfDisplays[i].clear();
    }
}

void requestEvent() {
    /**
    * Send the parameter values when requested.
    */

    uint8_t* dataToSendptr;
    dataToSendptr = dataToSend;  // 
    // Write the values as 8 bit unsigned ( 0 - 255 )
    Wire.write(dataToSendptr, sizeof(dataToSend));
    // Mute button only survives one request 
    isButtonClicked[MuteButton] = isButtonPressed[MuteButton] = 0 ; 

}

void receiveEvent(int numberOfBytes) {
    /**
    * Read the received bytes from I2C.
    */
    unsigned long i = 0;
    uint8_t* receivedParamValsPtr = (uint8_t*) receivedParameterValues;
    while (Wire.available() && i < sizeof(receivedParameterValues) ) {
        *receivedParamValsPtr++ = Wire.read();
        i++;
    }

    //  Show Ventilator alarms
    showAlarms();
}