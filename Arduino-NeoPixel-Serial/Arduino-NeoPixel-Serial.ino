/* Serial-NeoPixel-Control.ino
 * Chase Sawyer, February 2017
 * Uses Adafruit's Neopixel librar
 *
 * BUGGY: Beyond about 15 neopixels, this control doesn't behave as expected.
 *  - Possibly could be that some data is getting lost in transit
 *  - or data isn't loading into the buffer correctly
 *  - or the transmission over the one wire data line isn't behaving properly
 */




#include "FastLED.h"
#define NUM_LEDS 60
#define DATA_PIN 6

CRGB leds[NUM_LEDS];

char cmd_buffer[240];
char cmd_size;

void setup() {
	Serial.begin(115200);
	delay(1000);
	FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
}

void loop() {
	for (int i = 0; i < NUM_LEDS; i++) {
		leds[i] = CRGB::Black;
	}
	FastLED.show();
	delay(30);
	// ask for data
	Serial.print("1");
	Serial.flush();
	delay(8);
	if (Serial.available() > 0) {
		cmd_size = Serial.read();
		if (cmd_size) {
			Serial.readBytes(cmd_buffer, cmd_size);
			for (int i = 0; i < NUM_LEDS; i++) {
				leds[i].setRGB( cmd_buffer[(i*4)+1],
								cmd_buffer[(i*4)+2],
								cmd_buffer[(i*4)+3] );
			}
			FastLED.show();
		}
	}
	delay(1000);
}