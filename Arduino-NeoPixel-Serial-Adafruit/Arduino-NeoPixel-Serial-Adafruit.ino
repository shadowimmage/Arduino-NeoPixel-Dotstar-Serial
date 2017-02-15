/* Arduino-NeoPixel-Serial-Adafruit.ino
 * Chase Sawyer, Feb. 2017
 * Uses the Adafruit NeoPixel library to try and get around bugs in the NeoPixel strip 
 * that were experienced using the FastLED library.
 * Changed control Scheme - Host machine has an abstract command set that it can call 
 * to the Arduino controller, and it can then do all the bookkeeping on the back end.
 *
 * Makes heavy use of Adafruit's guide on multitasking in Arduino
 * https://learn.adafruit.com/multi-tasking-the-arduino-part-3
 * 
 */

#include <CmdMessenger.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define DATA_PIN 6
#define NUM_LEDS 60
#define BAUD_RATE 115200

// Pattern types supported:
enum  pattern {
	NONE, 
	RAINBOW_CYCLE,
	THEATER_CHASE,
	COLOR_WIPE, 
	SCANNER, 
	FADE,
    STATIC
};
// Patern directions supported:
enum  direction { FORWARD, REVERSE };

boolean UNBLOCKED = true;

// NeoPattern Class - derived from the Adafruit_NeoPixel class
class NeoPatterns : public Adafruit_NeoPixel
{
    public:

    // Member Variables:  
    pattern  ActivePattern;  // which pattern is running
    direction Direction;     // direction to run the pattern
    
    unsigned long Interval;   // milliseconds between updates
    unsigned long lastUpdate; // last update of position
    
    uint32_t Color1, Color2;  // What colors are in use
    uint16_t TotalSteps;  // total number of steps in the pattern
    uint16_t Index;  // current step within the pattern
    
    void (*OnComplete)();  // Callback on completion of pattern
    
    // Constructor - calls base-class constructor to initialize strip
    // pixels : Number of pixels in this instance
    // pin : data pin to write out
    // type : see Adafruit_NeoPixel library for LED type Declaration
    // callback : callback function to run each time the pattern ends
    NeoPatterns(uint16_t pixels, uint8_t pin, uint8_t type, void (*callback)())
    :Adafruit_NeoPixel(pixels, pin, type)
    {
        OnComplete = callback;
    }
    
    // Update the pattern
    void Update()
    {
        if((millis() - lastUpdate) > Interval) // time to update
        {
            lastUpdate = millis();
            switch(ActivePattern)
            {
                case RAINBOW_CYCLE:
                    RainbowCycleUpdate();
                    break;
                case THEATER_CHASE:
                    TheaterChaseUpdate();
                    break;
                case COLOR_WIPE:
                    ColorWipeUpdate();
                    break;
                case SCANNER:
                    ScannerUpdate();
                    break;
                case FADE:
                    FadeUpdate();
                    break;
                case STATIC:
                    StaticPatternUpdate();
                default:
                    break;
            }
        }
    }
  
    // Increment the Index and reset at the end
    void Increment()
    {
        if (Direction == FORWARD)
        {
           Index++;
           if (Index >= TotalSteps)
            {
                Index = 0;
                if (OnComplete != NULL)
                {
                    OnComplete(); // call the comlpetion callback
                }
            }
        }
        else // Direction == REVERSE
        {
            --Index;
            if (Index <= 0)
            {
                Index = TotalSteps-1;
                if (OnComplete != NULL)
                {
                    OnComplete(); // call the comlpetion callback
                }
            }
        }
    }
    
    // Reverse pattern direction
    void Reverse()
    {
        if (Direction == FORWARD)
        {
            Direction = REVERSE;
            Index = TotalSteps-1;
        }
        else
        {
            Direction = FORWARD;
            Index = 0;
        }
    }

    // display a static pattern, and calls the OnComplete handler.
    // Interval controls how often this gets checked
    void StaticPattern(uint8_t interval) {
    	ActivePattern = STATIC;
    	Interval = interval;
        TotalSteps = 1;
        Index = 0;
    	show();
    }

    // doesn't do much; increments for the callback function to eventually execute.
    void StaticPatternUpdate() {
        Increment();
    }
    
    // Initialize for a RainbowCycle
    void RainbowCycle(uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = RAINBOW_CYCLE;
        Interval = interval;
        TotalSteps = 255;
        Index = 0;
        Direction = dir;
    }
    
    // Update the Rainbow Cycle Pattern
    void RainbowCycleUpdate()
    {
        for(int i=0; i< numPixels(); i++)
        {
            setPixelColor(i, Wheel(((i * 256 / numPixels()) + Index) & 255));
        }
        show();
        Increment();
    }

    // Initialize for a Theater Chase
    void TheaterChase(uint32_t color1, uint32_t color2, uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = THEATER_CHASE;
        Interval = interval;
        TotalSteps = numPixels();
        Color1 = color1;
        Color2 = color2;
        Index = 0;
        Direction = dir;
   }
    
    // Update the Theater Chase Pattern
    void TheaterChaseUpdate()
    {
        for(int i=0; i< numPixels(); i++)
        {
            if ((i + Index) % 3 == 0)
            {
                setPixelColor(i, Color1);
            }
            else
            {
                setPixelColor(i, Color2);
            }
        }
        show();
        Increment();
    }

    // Initialize for a ColorWipe
    void ColorWipe(uint32_t color, uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = COLOR_WIPE;
        Interval = interval;
        TotalSteps = numPixels();
        Color1 = color;
        Index = 0;
        Direction = dir;
    }
    
    // Update the Color Wipe Pattern
    void ColorWipeUpdate()
    {
        setPixelColor(Index, Color1);
        show();
        Increment();
    }
    
    // Initialize for a SCANNNER
    void Scanner(uint32_t color1, uint8_t interval)
    {
        ActivePattern = SCANNER;
        Interval = interval;
        TotalSteps = (numPixels() - 1) * 2;
        Color1 = color1;
        Index = 0;
    }

    // Update the Scanner Pattern
    void ScannerUpdate()
    { 
        for (int i = 0; i < numPixels(); i++)
        {
            if (i == Index)  // Scan Pixel to the right
            {
                 setPixelColor(i, Color1);
            }
            else if (i == TotalSteps - Index) // Scan Pixel to the left
            {
                 setPixelColor(i, Color1);
            }
            else // Fading tail
            {
                 setPixelColor(i, DimColor(getPixelColor(i)));
            }
        }
        show();
        Increment();
    }
    
    // Initialize for a Fade
    void Fade(uint32_t color1, uint32_t color2, uint16_t steps, uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = FADE;
        Interval = interval;
        TotalSteps = steps;
        Color1 = color1;
        Color2 = color2;
        Index = 0;
        Direction = dir;
    }
    
    // Update the Fade Pattern
    void FadeUpdate()
    {
        // Calculate linear interpolation between Color1 and Color2
        // Optimise order of operations to minimize truncation error
        uint8_t red = ((Red(Color1) * (TotalSteps - Index)) + (Red(Color2) * Index)) / TotalSteps;
        uint8_t green = ((Green(Color1) * (TotalSteps - Index)) + (Green(Color2) * Index)) / TotalSteps;
        uint8_t blue = ((Blue(Color1) * (TotalSteps - Index)) + (Blue(Color2) * Index)) / TotalSteps;
        
        ColorSet(Color(red, green, blue));
        show();
        Increment();
    }

    // Update the Interval for any running pattern.
    void UpdateInterval(uint32_t interval) {
    	Interval = interval;
    }
   
    // Calculate 50% dimmed version of a color (used by ScannerUpdate)
    uint32_t DimColor(uint32_t color)
    {
        // Shift R, G and B components one bit to the right
        uint32_t dimColor = Color(Red(color) >> 1, Green(color) >> 1, Blue(color) >> 1);
        return dimColor;
    }

    // Set all pixels to a color (synchronously)
    void ColorSet(uint32_t color)
    {
        for (int i = 0; i < numPixels(); i++)
        {
            setPixelColor(i, color);
        }
        show();
    }

    // Set a single pixel to a color
    void ColorSet(uint32_t color, uint16_t index) {
    	setPixelColor(index, color);
    	show();
    }

    // Set a range from stIndex to num pixels to a color
    void ColorSet(uint32_t color, uint16_t stIndex, uint16_t num) {
    	for (int i = stIndex; i < num; i++) {
    		setPixelColor(i, color);
    	}
    	show();
    }

    // Returns the Red component of a 32-bit color
    uint8_t Red(uint32_t color)
    {
        return (color >> 16) & 0xFF;
    }

    // Returns the Green component of a 32-bit color
    uint8_t Green(uint32_t color)
    {
        return (color >> 8) & 0xFF;
    }

    // Returns the Blue component of a 32-bit color
    uint8_t Blue(uint32_t color)
    {
        return color & 0xFF;
    }
    
    // Input a value 0 to 255 to get a color value.
    // The colours are a transition r - g - b - back to r.
    uint32_t Wheel(byte WheelPos)
    {
        WheelPos = 255 - WheelPos;
        if(WheelPos < 85)
        {
            return Color(255 - WheelPos * 3, 0, WheelPos * 3);
        }
        else if(WheelPos < 170)
        {
            WheelPos -= 85;
            return Color(0, WheelPos * 3, 255 - WheelPos * 3);
        }
        else
        {
            WheelPos -= 170;
            return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
        }
    }
};

//Only using one strip of LEDs
// void Ring1Complete();
// void Ring2Complete();
// void StickComplete();
void StripComplete();

// Define some NeoPatterns for the two rings and the stick
//  as well as some completion routines
// NeoPatterns Ring1(24, 5, NEO_GRB + NEO_KHZ800, &Ring1Complete);
// NeoPatterns Ring2(16, 6, NEO_GRB + NEO_KHZ800, &Ring2Complete);
// NeoPatterns Stick(16, 7, NEO_GRB + NEO_KHZ800, &StickComplete);
NeoPatterns Strip(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800, &StripComplete);

// Commands recognized
enum  commands {
	CMDERROR,
	SETCOLORALL,
	SETCOLORSINGLE,
	SETCOLORRANGE,
	SETPATTERNRAINBOW,
	SETPATTERNTHEATER,
	SETPATTERNWIPE,
	SETPATTERNSCANNER,
	SETPATTERNFADE,
	SETBRIGHTNESSALL,
	ARDUINOBUSY,
	NOCOMMAND
};

// Attach a new CmdMessenger object to the  default Serial port
CmdMessenger cmdMessenger = CmdMessenger(Serial,',',';','/');

// Attach handler functions to recognized commands
void attachCommandCallbacks() {
	cmdMessenger.attach(SETCOLORALL, CMDsetColorAll);
	cmdMessenger.attach(SETCOLORSINGLE, CMDsetColorSingle);
	cmdMessenger.attach(SETCOLORRANGE, CMDsetColorRange);
    cmdMessenger.attach(SETPATTERNRAINBOW, CMDsetPatternRainbow);
    cmdMessenger.attach(SETPATTERNTHEATER, CMDsetPatternTheater);
    cmdMessenger.attach(SETPATTERNWIPE, CMDsetPatternWipe);
    cmdMessenger.attach(SETPATTERNSCANNER, CMDsetPatternScanner);
	cmdMessenger.attach(SETPATTERNFADE, CMDsetPatternFade);
	cmdMessenger.attach(SETBRIGHTNESSALL, CMDsetBrightnessAll);
	cmdMessenger.attach(ARDUINOBUSY, CMDarduinoBusy);
	cmdMessenger.attach(NOCOMMAND, CMDnoCommand);
    cmdMessenger.attach(CMDunknownCommand); // default on unrecognized
}

// called on any command received that doesn't match above handlers.
void CMDunknownCommand() {
	cmdMessenger.sendCmd(CMDERROR, F("Command unknown / no attached callback."));
}

// sets all the colors on the strip to a defined color passed in the command argument
// Expected command would be in the form: "1,0xffeedd"
// cmdMessenger param1: color
void CMDsetColorAll() {
    uint32_t color = cmdMessenger.readBinArg<uint32_t>();
    while (cmdMessenger.available()) {
    	cmdMessenger.readBinArg<byte>();
    }
    if (color != NULL) {
        Strip.ColorSet(color);
        Strip.StaticPattern(50);
        cmdMessenger.sendBinCmd(CMDERROR, color);
        Serial.flush();
    } else {
        cmdMessenger.sendCmd(CMDERROR, F("Missing color parameter."));
    }
    UNBLOCKED = true;
}
// void CMDsetColorAll() {
//     uint32_t color = cmdMessenger.readBinArg<uint32_t>();
//     while (cmdMessenger.available()) {
//     	cmdMessenger.readBinArg<byte>();
//     }
//     // if (color != NULL) {
//     //     Strip.ActivePattern = STATIC;
//     //     Strip.ColorSet(color);
//     cmdMessenger.sendBinCmd(CMDERROR, color);
//     Serial.flush();
//     // } else {
//     //     cmdMessenger.sendCmd(CMDERROR, F("Missing color parameter."));
//     //     Serial.flush();
//     // }
//     UNBLOCKED = true;
// }

// sets a single LED to designated color
// Expected command would be in the form: "2,2,0xffeedd"
// cmdMessenger param1 LED index
// cmdMessenger param2 Color
void CMDsetColorSingle() {
    boolean ok = false;
	uint16_t led;
	uint32_t color;
	if (cmdMessenger.available()) {
		led = cmdMessenger.readInt16Arg();
		if (cmdMessenger.available()) {
			color = cmdMessenger.readInt32Arg();
			ok = true;
		}
	}
	if (ok) {
        Strip.ActivePattern = STATIC;
		Strip.ColorSet(color, led);
        cmdMessenger.sendCmd(CMDERROR, F("Color Set Single OK."));
	} else {
		cmdMessenger.sendCmd(CMDERROR, F("Missing single color set parameter(s)."));
	}
}

// sets a range of LEDs to designated color
// Expected command would be in the form: "2,0,10,0xffeedd"
// cmdMessenger param1 starting LED index
// cmdMessenger param2 Number of LEDs to color
// cmdMessenger param3 Color
void CMDsetColorRange() {
    boolean ok = false;
	uint16_t startLED;
	uint16_t num;
	uint32_t color;
	if (cmdMessenger.available()) {
		startLED = cmdMessenger.readInt16Arg();
		if (cmdMessenger.available()) {
			num = cmdMessenger.readInt16Arg();
			if (cmdMessenger.available()) {
				color = cmdMessenger.readInt32Arg();
				ok = true;
			}
		}
	}
	if (ok) {
        Strip.ActivePattern = STATIC;
		Strip.ColorSet(color, startLED, num);
        cmdMessenger.sendCmd(CMDERROR, F("Color Set Range OK."));
	} else {
		cmdMessenger.sendCmd(CMDERROR, F("Missing color range parameter(s)."));
	}
}

// Set the LEDs to the Rainbow Pattern
// Expected Command would be in the form: "4,2"
// cmdMessenger param1: interval for the rainbow cycle.
void CMDsetPatternRainbow() {
    uint32_t interval = cmdMessenger.readBinArg<uint32_t>();
    while (cmdMessenger.available()) {
    	cmdMessenger.readBinArg<byte>();
    }
    if (interval != NULL) {
        Strip.RainbowCycle(interval);
        // cmdMessenger.sendCmd(CMDERROR, F("Rainbow Cycle Pattern OK."));
        cmdMessenger.sendBinCmd(CMDERROR,interval);
        Serial.flush();
    } else {
        cmdMessenger.sendCmd(CMDERROR, F("Insufficient Params - Rainbow Cycle needs 1."));
        Serial.flush();
    }
    UNBLOCKED = true;
}
// void CMDsetPatternRainbow() {
//     // boolean ok = false;
//     uint32_t interval = cmdMessenger.readBinArg<uint32_t>();
//     while (cmdMessenger.available()) {
//     	cmdMessenger.readBinArg<byte>();
//     }
//     cmdMessenger.sendBinCmd(CMDERROR, interval);
    
//     // if (interval != NULL) {
//     //     ok = true;
//     // }
//     // if (ok) {
//     //     Strip.RainbowCycle(interval);
//     //     cmdMessenger.sendCmd(CMDERROR, F("Rainbow Cycle Pattern OK."));
//         Serial.flush();
//     // } else {
//     //     cmdMessenger.sendCmd(CMDERROR, F("Insufficient Params - Rainbow Cycle needs 1."));
//     //     Serial.flush();
//     // }
//     UNBLOCKED = true;
// }

// Set the LEDs to a Theater Chase pattern.
// Expected command would be in the form: "5,0xFFEEDD,0xAA1234,40"
// cmdMessenger param1: Color 1
// cmdMessenger param2: Color 2
// cmdMessenger param3: Interval
void CMDsetPatternTheater() {
    boolean ok = false;
    uint32_t color1;
    uint32_t color2;
    uint32_t interval;
    if (cmdMessenger.available()) {
        color1 = cmdMessenger.readInt32Arg();
        if (cmdMessenger.available()) {
            color2 = cmdMessenger.readInt32Arg();
            if (cmdMessenger.available()) {
                interval = cmdMessenger.readInt32Arg();
                ok = true;
            }
        }
    }
    if (ok) {
        Strip.TheaterChase(color1, color2, interval);
        cmdMessenger.sendCmd(CMDERROR, F("Theater Chase pattern OK."));
    } else {
        cmdMessenger.sendCmd(CMDERROR, F("Insufficient Params - Theater Chase needs 3."));
    }
}

// Set the LEDs to the color wipe pattern.
// Expected command would be in the form: "6,0x003304,45"
// cmdMessenger param1: interval for the pattern to update.
void CMDsetPatternWipe() {
    boolean ok = false;
    uint32_t color;
    uint32_t interval;
    if (cmdMessenger.available()) {
        color = cmdMessenger.readInt32Arg();
        if (cmdMessenger.available()) {
            interval = cmdMessenger.readInt32Arg();
            ok = true;
        }
    }
    if (ok) {
        Strip.ColorWipe(color, interval);
        cmdMessenger.sendCmd(CMDERROR, F("Color Wipe pattern OK."));
    } else {
        cmdMessenger.sendCmd(CMDERROR, F("Insufficient Params - Color Wipe needs 2."));
    }
}

// Set the LEDs to a 'scanner' pattern (think cylon centurion)
// Expected command would be in the form: "7,0xCABCAB,100"
// cmdMessenger param1: color to scan
// cmdMessenger param2: Update interval
void CMDsetPatternScanner() {
    boolean ok = false;
    uint32_t color;
    uint32_t interval;
    if (cmdMessenger.available()) {
        color = cmdMessenger.readInt32Arg();
        if (cmdMessenger.available()) {
            interval = cmdMessenger.readInt32Arg();
            ok = true;
        }
    }
    if (ok) {
        Strip.Scanner(color, interval);
        cmdMessenger.sendCmd(CMDERROR, F("Scanner pattern OK."));
    } else {
        cmdMessenger.sendCmd(CMDERROR, F("Insufficient Params - Scanner needs 2."));
    }
}

// Set the LEDs to a fade pattern
// Expected command would be in the form: "8,0xFFEEDD,0xCAAABA,10,10"
// cmdMessenger param1: color 1
// cmdMessenger param2: color 2
// cmdMessenger param3: steps between
// cmdMessenger param4: Update interval
void CMDsetPatternFade() {
    boolean ok = false;
    uint32_t color1;
    uint32_t color2;
    uint16_t steps;
    uint32_t interval;
    if (cmdMessenger.available()) {
        color1 = cmdMessenger.readInt32Arg();
        if (cmdMessenger.available()) {
            color2 = cmdMessenger.readInt32Arg();
            if (cmdMessenger.available()) {
                steps = cmdMessenger.readInt16Arg();
                if (cmdMessenger.available()) {
                    interval = cmdMessenger.readInt32Arg();
                    ok = true;
                }
            }
        }
    }
    if (ok) {
        Strip.Fade(color1, color2, steps, interval);
        cmdMessenger.sendCmd(CMDERROR, F("Fade pattern OK."));
    } else {
        cmdMessenger.sendCmd(CMDERROR, F("Insufficient Params - Fade needs 4."));
    }
}

// Use internal NeoPixel library brightness function to globally set a max brightness
// for the whole instance. Note: slow - shouldn't be called often.
// Expected command would be in the form: "5,64"
// cmdMessenger param1: brightness level
void CMDsetBrightnessAll() {
	uint8_t brightness;
	boolean ok = false;
	if (cmdMessenger.available()) {
		brightness = cmdMessenger.readInt16Arg();
		ok = true;
	}
	if (ok) {
		Strip.setBrightness(brightness);
	} else {
		cmdMessenger.sendCmd(CMDERROR, F("Brightness needs 1 param."));
	}
}

// placeholder - host shouldn't be replying on this command.
void CMDarduinoBusy() {
	;
}

// default reply from host if there are no updates - clear out input buffer
void CMDnoCommand() {
	while (cmdMessenger.available()) {
		cmdMessenger.readBinArg<byte>();
	}
	UNBLOCKED = true;
}

// Initialize everything and prepare to start
void setup() {
	Serial.begin(BAUD_RATE);

	// set up the strip
	Strip.begin();

    // attach callback command routines
    attachCommandCallbacks();

    // start the rainbow for the default pattern
    Strip.RainbowCycle(3);
}

// Main loop
void loop() {
	// Process incomming serial data, and perform callbacks
    if (UNBLOCKED) {
    	Strip.Update();
    } else {
    	cmdMessenger.feedinSerialData();
    }
}

// Strip Completion Callback
void StripComplete() {
	cmdMessenger.sendBinCmd(ARDUINOBUSY,false);
	Serial.flush();
	UNBLOCKED = false; // block the update method - waiting for host reply.
}