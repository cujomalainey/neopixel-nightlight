/**
 * Simple single button control for Neopixel
 *
 * Short press: change mode/on
 * Long press: change brightness
 * Double tap: off
 */
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <JC_Button.h>

#define PIXEL_COUNT  19 // TODO double check
#define NEOPIXEL_PIN 3  // TODO ditto
#define BUTTON_PIN   4  // TODO ditto

#define LONG_PRESS_MS 1000
#define DOUBLE_TAP_RELEASE_MS 1000

typedef enum {
	PINK,
	RED,
	ORANGE,
	YELLOW,
	GREEN,
	CYAN,
	BLUE,
	PURPLE,
	CYCLING,
	COLOUR_COUNT,
} colour_t;

typedef enum {
	SOLID,
	BREATHING,
	CHASING,
	WAVE,
	MODE_COUNT,
} light_mode_t;

struct state {
	uint8_t brightness; // (1-100%) treated as upper limit
	colour_t colour;
	light_mode_t mode;
	bool on;
	bool long_press;
	uint32_t last_release;
};

struct rgb {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
};

const struct rgb colour_lookup[COLOUR_COUNT] = {
	{255, 192, 203}, // Pink
	{255, 0,   0  }, // Red
	{255, 69,  0  }, // Orange
	{255, 255, 0  }, // Yellow
	{0,   255, 0  }, // Green
	{0,   255, 255}, // Cyan
	{0,   0,   255}, // Blue
	{128, 0,   128}, // Purple
	{}
};

struct state current_state = {
	.brightness 	= 100,
	.colour     	= PINK,
	.mode       	= SOLID,
	.on				= true,
	.long_press 	= false,
	.last_release 	= 0,
};

struct state previous_state = current_state;

Adafruit_NeoPixel strip(PIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
Button btn(BUTTON_PIN);

void reset_state() {
	current_state = {
		.brightness 	= 100,
		.colour     	= PINK,
		.mode       	= SOLID,
		.on         	= false,
		.long_press 	= false,
		.last_release 	= 0,
	};
	previous_state = current_state;
}

void handle_button() {
	bool rollover;
	btn.read();
	if (!current_state.on && btn.isPressed()) {
		current_state.on = true;
	}
	if (btn.pressedFor(LONG_PRESS_MS)) {
		current_state.brightness = BRIGHTNESS_AMP*sin(BRIGHTNESS_FREQ*millis()) + BRIGHTNESS_OFFSET;
		run_solid();
		current_state.long_press = true;
	} else if (btn.wasReleased()) {
		if (current_state.long_press) {
			// Returning after long press, dont update release timer
			// to avoid double press trigger
			current_state.long_press = false;
			return;
		}

		rollover = millis() < current_state.last_release; // check for millis rollover ~50d
		if (!rollover && (millis() - current_state.last_release) < DOUBLE_TAP_RELEASE_MS) {
			// double tap to turn off and undo next mode
			turn_off();
			current_state = previous_state;
			current_state.on = false;
			return;
		}

		// single press, next mode
		current_state.last_release = btn.lastChange();
		previous_state = current_state;
		current_state.colour++;
		if (current_state.colour >= COLOUR_COUNT) {
			current_state.mode++;
			current_state.colour = PINK;
			if (current_state.mode >= MODE_COUNT) {
				current_state.mode = SOLID;
			}
		}
	}
}

/**
 * turn off LEDs
 */
void turn_off() {
}

/**
 * Solid static colour
 */
void run_solid() {
	// TODO
}

/**
 * sine wave brightness
 */
void run_breating() {
	static breathing_level = 0;
	// TODO
}

/**
 * rotating pulse with tail
 */
void run_chasing() {
	static chasing_angle = 0;
	// TODO
}

/**
 * pulse from center out
 */
void run_wave() {
	static pulse_age = 0;
	// TODO
}

void handle_animations() {
	switch (current_state.mode) {
	case SOLID:
		run_solid();
		break;
	case BREATHING:
		run_breating();
		break;
	case CHASING:
		run_chasing();
		break;
	case WAVE:
		run_wave();
		break;
	default:
		// We shouldnt be able to get here, but it we do, lets reset
		reset_state();
		break;
	}
}

void setup(){
}

void loop(){
	handle_button();
	if (current_state.on && !current_state.long_press) {
		handle_animations();
	}
}

