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

// Pattern types supported:
enum  pattern { 
	NONE, 
	RAINBOW_CYCLE, 
	THEATER_CHASE, 
	COLOR_WIPE, 
	SCANNER, 
	FADE, 
	BLINK 
};
// Patern directions supported:
enum  direction { FORWARD, REVERSE };

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
	SETPATTERN,
	SETBRIGHTNESSALL
};

// Attach a new CmdMessenger object to the  default Serial port
CmdMessenger cmdMessenger = CmdMessenger(Serial);

// Attach handler functions to recognized commands
// UNCOMMENT THESE AS THEY'RE IMPLEMENTED
void attachCommandCallbacks() {
	cmdMessenger.attach(CMDunknownCommand); // default on unrecognized
	cmdMessenger.attach(SETCOLORALL, CMDsetColorAll);
	cmdMessenger.attach(SETCOLORSINGLE, CMDsetColorSingle);
	cmdMessenger.attach(SETCOLORRANGE, CMDsetColorRange);
	cmdMessenger.attach(SETPATTERN, CMDsetPattern);
	cmdMessenger.attach(SETBRIGHTNESSALL, CMDsetBrightnessAll);
}

// called on any command received that doesn't match above handlers.
void CMDunknownCommand() {
	cmdMessenger.sendCmd(CMDERROR, F("Command unknown / no attached callback."));
}

// sets all the colors on the strip to a defined color passed in the command argument
// Expected command would be in the form: "1,0xffeedd"
// cmdMessenger param1: color
void CMDsetColorAll() {
	if (cmdMessenger.available()) {
		Strip.ColorSet(cmdMessenger.readInt32Arg());
	} else {
		cmdMessenger.sendCmd(CMDERROR, F("Missing color parameter."));
	}
}

// sets a single LED to designated color
// Expected command would be in the form: "2,2,0xffeedd"
// cmdMessenger param1 LED index
// cmdMessenger param2 Color
void CMDsetColorSingle() {
	uint16_t led;
	uint32_t color;
	boolean ok = false;
	if (cmdMessenger.available()) {
		led = cmdMessenger.readInt16Arg();
		if (cmdMessenger.available()) {
			color = cmdMessenger.readInt32Arg();
			ok = true;
		}
	}
	if (ok) {
		Strip.ColorSet(color, led);
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
	uint16_t startLED;
	uint16_t num;
	uint32_t color;
	boolean ok = false;
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
		Strip.ColorSet(color, startLED, num);
	} else {
		cmdMessenger.sendCmd(CMDERROR, F("Missing color range parameter(s)."));
	}
}

// Set the LEDs to a certain pre-defined pattern (from the NeoPatterns class)
// Expected command would be in the form: "4,2"
// cmdMessenger param1: pattern index as defined by enum pattern {}
// cmdMessenger param2+: varaibles for the Pattern to execute
// Basic format:
// 		if the pattern matches, go to that pattern # and 
// 		Check through for the needed additional parameters
// 		If all the parameters are present (and thus set to local variables) 
// 		Then pass those along to the pattern function,
// 		otherwise send an error back to the computer.
void CMDsetPattern() {
	uint16_t pattern;
	if (cmdMessenger.available()) {
		pattern = cmdMessenger.readInt16Arg();
		switch(pattern) {
			case RAINBOW_CYCLE: {
				uint8_t interval;
				boolean ok = false;
				if (cmdMessenger.available()) {
					interval = cmdMessenger.readInt16Arg();
					ok = true;
				}
				if (ok) {
					Strip.RainbowCycle(interval);
				} else {
					cmdMessenger.sendCmd(CMDERROR, F("Insufficient Params - Rainbow Cycle needs 1."));
				}
			}
				break;
			case THEATER_CHASE: {
				uint32_t color1;
				uint32_t color2;
				uint8_t interval;
				boolean ok = false;
				if (cmdMessenger.available()) {
					color1 = cmdMessenger.readInt32Arg();
					if (cmdMessenger.available()) {
						color2 = cmdMessenger.readInt32Arg();
						if (cmdMessenger.available()) {
							interval = cmdMessenger.readInt16Arg();
							ok = true;
						}
					}
				}
				if (ok) {
					Strip.TheaterChase(color1, color2, interval);
				} else {
					cmdMessenger.sendCmd(CMDERROR, F("Insufficient Params - Theater Chase needs 3."));
				}
			}
				break;
			case COLOR_WIPE: {
				uint32_t color;
				uint8_t interval;
				boolean ok = false;
				if (cmdMessenger.available()) {
					color = cmdMessenger.readInt32Arg();
					if (cmdMessenger.available()) {
						interval = cmdMessenger.readInt16Arg();
						ok = true;
					}
				}
				if (ok) {
					Strip.ColorWipe(color, interval);
				} else {
					cmdMessenger.sendCmd(CMDERROR, F("Insufficient Params - Color Wipe needs 2."));
				}
			}
				break;
			case SCANNER: {
				uint32_t color;
				uint8_t interval;
				boolean ok = false;
				if (cmdMessenger.available()) {
					color = cmdMessenger.readInt32Arg();
					if (cmdMessenger.available()) {
						interval = cmdMessenger.readInt16Arg();
						ok = true;
					}
				}
				if (ok) {
					Strip.Scanner(color, interval);
				} else {
					cmdMessenger.sendCmd(CMDERROR, F("Insufficient Params - Scanner needs 2."));
				}
			}
				break;
			case FADE: {
				uint32_t color1;
				uint32_t color2;
				uint16_t steps;
				uint8_t interval;
				boolean ok = false;
				if (cmdMessenger.available()) {
					color1 = cmdMessenger.readInt32Arg();
					if (cmdMessenger.available()) {
						color2 = cmdMessenger.readInt32Arg();
						if (cmdMessenger.available()) {
							steps = cmdMessenger.readInt16Arg();
							if (cmdMessenger.available()) {
								interval = cmdMessenger.readInt16Arg();
								ok = true;
							}
						}
					}
				}
				if (ok) {
					Strip.Fade(color1, color2, steps, interval);
				} else {
					cmdMessenger.sendCmd(CMDERROR, F("Insufficient Params - Fade needs 4."));
				}
			}
				break;
			default:
				cmdMessenger.sendCmd(CMDERROR, F("No matching pattern index."));
				break;
		}
	} else {
		cmdMessenger.sendCmd(CMDERROR, F("Missing pattern parameter."));
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

// Initialize everything and prepare to start
void setup() {
	Serial.begin(115200);
	Strip.begin();

    // Kick off a pattern
    // Ring1.TheaterChase(Ring1.Color(255,255,0), Ring1.Color(0,0,50), 100);
    // Ring2.RainbowCycle(3);
    // Ring2.Color1 = Ring1.Color1;
    // Stick.Scanner(Ring1.Color(255,0,0), 55);

    attachCommandCallbacks();

    Strip.RainbowCycle(3);
}

// Main loop
void loop() {
	// Process incomming serial data, and perform callbacks
	cmdMessenger.feedinSerialData();

    // Update the LEDs.
    // Ring1.Update();
    // Ring2.Update();
    Strip.Update();
    

    //ORIGINAL DEMO CODE WITH BUTTONS:
    // Switch patterns on a button press:
    // if (digitalRead(8) == LOW) // Button #1 pressed
    // {
    //     // Switch Ring1 to FASE pattern
    //     Ring1.ActivePattern = FADE;
    //     Ring1.Interval = 20;
    //     // Speed up the rainbow on Ring2
    //     Ring2.Interval = 0;
    //     // Set stick to all red
    //     Stick.ColorSet(Stick.Color(255, 0, 0));
    // }
    // else if (digitalRead(9) == LOW) // Button #2 pressed
    // {
    //     // Switch to alternating color wipes on Rings1 and 2
    //     Ring1.ActivePattern = COLOR_WIPE;
    //     Ring2.ActivePattern = COLOR_WIPE;
    //     Ring2.TotalSteps = Ring2.numPixels();
    //     // And update tbe stick
    //     Stick.Update();
    // }
    // else // Back to normal operation
    // {
    //     // Restore all pattern parameters to normal values
    //     Ring1.ActivePattern = THEATER_CHASE;
    //     Ring1.Interval = 100;
    //     Ring2.ActivePattern = RAINBOW_CYCLE;
    //     Ring2.TotalSteps = 255;
    //     Ring2.Interval = min(10, Ring2.Interval);
    //     // And update tbe stick
    //     Stick.Update();
    // }    
}

//------------------------------------------------------------
//Completion Routines - get called on completion of a pattern
//------------------------------------------------------------

// Ring1 Completion Callback
// void Ring1Complete()
// {
//     if (digitalRead(9) == LOW)  // Button #2 pressed
//     {
//         // Alternate color-wipe patterns with Ring2
//         Ring2.Interval = 40;
//         Ring1.Color1 = Ring1.Wheel(random(255));
//         Ring1.Interval = 20000;
//     }
//     else  // Retrn to normal
//     {
//       Ring1.Reverse();
//     }
// }

// Ring 2 Completion Callback
// void Ring2Complete()
// {
//     if (digitalRead(9) == LOW)  // Button #2 pressed
//     {
//         // Alternate color-wipe patterns with Ring1
//         Ring1.Interval = 20;
//         Ring2.Color1 = Ring2.Wheel(random(255));
//         Ring2.Interval = 20000;
//     }
//     else  // Retrn to normal
//     {
//         Ring2.RainbowCycle(random(0,10));
//     }
// }

// Stick Completion Callback
// void StickComplete()
// {
//     // Random color change for next scan
//     Stick.Color1 = Stick.Wheel(random(255));
// }

// Strip Completion Callback
void StripComplete() {
	// do something?
	;
}