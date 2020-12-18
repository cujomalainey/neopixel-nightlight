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

// HW defines
#define PIXEL_COUNT  19 // TODO double check
#define NEOPIXEL_PIN 7  // TODO ditto
#define BUTTON_PIN   4  // TODO ditto

// Interaction Defines
#define LONG_PRESS_MS 1000
#define DOUBLE_TAP_RELEASE_MS 1000

// Defines
#define BRIGHTNESS_AMP 50.0
#define BRIGHTNESS_OFFSET 50.0
#define BRIGHTNESS_FREQ 1.0/1000

#define NEXT_COLOUR(val) (colour_t(((uint8_t)val) + 1))
#define NEXT_MODE(val) (light_mode_t(((uint8_t)val) + 1))

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
	SOLID = 0,
	BREATHING = 1,
	CHASING = 2,
	WAVE = 3,
	MODE_COUNT = 4,
} light_mode_t;

typedef enum {
	STEP,
	INTERPOLATE,
} colour_mode_t;

struct state {
	uint8_t brightness; // (1-100%) treated as upper limit
	colour_t colour;
	light_mode_t mode;
	bool on;
	bool long_press;
	uint32_t last_release;
};

struct rgb {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

const struct rgb colour_lookup[COLOUR_COUNT] = {
	{255, 0,   100}, // Pink
	{255, 0,   0  }, // Red
	{255, 59,  0  }, // Orange
	{255, 175, 0  }, // Yellow
	{0,   255, 0  }, // Green
	{0,   255, 255}, // Cyan
	{0,   0,   255}, // Blue
	{200, 0,   255}, // Purple
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

Adafruit_NeoPixel pixels(PIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
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

/**
 * turn off LEDs
 */
void turn_off() {
	pixels.clear();
}

/**
 * Fetches colour given current state
 */
struct rgb get_colour(colour_mode_t mode, uint32_t state = millis()) {
	if (current_state.colour < CYCLING) {
		return colour_lookup[current_state.colour];
	}
}

/**
 * Solid static colour
 */
void run_solid() {
	struct rgb colour = get_colour(INTERPOLATE);
	for(int i = 0; i < PIXEL_COUNT; i++) { // For each pixel...
		pixels.setPixelColor(i, pixels.Color(colour.r, colour.g, colour.b));

		pixels.show();   // Send the updated pixel colors to the hardware.
	}
}

/**
 * sine wave brightness
 */
void run_breating() {
	static uint16_t breathing_level = 0;
	// TODO
}

/**
 * rotating pulse with tail
 */
void run_chasing() {
	static uint16_t chasing_angle = 0;
	// TODO
}

/**
 * pulse from center out
 */
void run_wave() {
	static uint16_t pulse_age = 0;
	// TODO
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
		current_state.colour = NEXT_COLOUR(current_state.colour);
		if (current_state.colour >= COLOUR_COUNT) {
			current_state.mode = NEXT_MODE(current_state.mode);
			current_state.colour = PINK;
			if (current_state.mode >= MODE_COUNT) {
				current_state.mode = SOLID;
			}
		}
	}
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

void setup() {
	btn.begin();
	pixels.begin();
}

void loop() {
	// handle_button();
	// if (current_state.on && !current_state.long_press) {
	// 	handle_animations();
	// }
	run_solid();
}

