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
#define PIXEL_COUNT  19
#define NEOPIXEL_PIN 2  // TODO ditto
#define BUTTON_PIN   9  // TODO ditto

// Interaction Defines
#define LONG_PRESS_MS 			1000
#define DOUBLE_TAP_RELEASE_MS 	300

// LED defines
#define MIDDLE_RING_ANGLE 	60
#define OUTER_RING_ANGLE  	30
#define INNER_RING_LEDS 	1
#define MIDDLE_RING_LEDS 	6
#define OUTER_RING_LEDS 	12
#define JEWEL_LEDS 			(INNER_RING_LEDS + MIDDLE_RING_LEDS)
#define OUTER_RING_OFFSET 	4

// Math Defines
#define BRIGHTNESS_AMP 		50.0
#define BRIGHTNESS_OFFSET 	50.0
#define BRIGHTNESS_FREQ 	1.0/1000
#define BRIGHTNESS_MAX 		100

// Breathing Defines
#define BREATHING_STEP_MS 	20

// Wave Defines
#define WAVE_STEP_MS 		20

// Colour Defines
#define INTERPOLATE_MS 		75

#define NEXT_COLOUR(val) (colour_t(((uint8_t)val) + 1))
#define NEXT_MODE(val) (light_mode_t(((uint8_t)val) + 1))

#define LINERP(x, y, a, f) (((((uint16_t)x) * (f- a)) + (((uint16_t)y) * a)) / f)

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

typedef enum {
	INNER,
	MIDDLE,
	OUTER,
} led_ring_t;

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
	pixels.show();
}

/**
 * Sets LED colour based on physical location
 */
void set_led(led_ring_t ring, uint16_t angle, uint32_t colour) {
	angle %= 360;
	if (ring == INNER) {
		pixels.setPixelColor(0, colour);
	} else if (ring == MIDDLE) {
		pixels.setPixelColor(INNER_RING_LEDS + (angle / MIDDLE_RING_ANGLE), colour);
	} else {
		uint8_t index = JEWEL_LEDS + (angle / OUTER_RING_ANGLE) + OUTER_RING_OFFSET;
		if (index >= PIXEL_COUNT) {
			index -= OUTER_RING_LEDS;
		}
		pixels.setPixelColor(index, colour);
	}
}

void set_ring(led_ring_t ring, uint32_t colour) {
	uint16_t step = ring == MIDDLE ? MIDDLE_RING_ANGLE : OUTER_RING_ANGLE;
	if (ring == INNER) {
		set_led(ring, 0, colour);
	}
	for (int i = 0; i < 360; i += step) {
		set_led(ring, i, colour);
	}
}

/**
 * Fetches colour given current state
 */
struct rgb get_colour(colour_mode_t mode, uint32_t state = millis() / INTERPOLATE_MS) {
	if (current_state.colour < CYCLING) {
		return colour_lookup[current_state.colour];
	} else if (current_state.colour == CYCLING) {
		if (mode == INTERPOLATE) {
		  // interpolate x * a and y * (1 - a)
		  struct rgb c1, c2, new_colour;
		  uint32_t colour_index, colour_phase;
		  colour_index = (state / 100) % CYCLING;
		  colour_phase = state % 100;

		  if (colour_index == 0) {
			  c1 = colour_lookup[CYCLING - 1];
			  c2 = colour_lookup[colour_index];
		  } else {
			  c1 = colour_lookup[colour_index - 1];
			  c2 = colour_lookup[colour_index];
		  }

		  new_colour.r = LINERP(c1.r, c2.r, colour_phase, 100);
		  new_colour.g = LINERP(c1.g, c2.g, colour_phase, 100);
		  new_colour.b = LINERP(c1.b, c2.b, colour_phase, 100);

		  return new_colour;
		} else {
		  // should be step
		  state %= CYCLING;
		  return colour_lookup[state];
		}
	} else {
		// Should not happen
		reset_state();
		return colour_lookup[PINK];
	}
}

uint32_t get_scaled_colour(struct rgb colour, uint8_t brightness=current_state.brightness) {
	uint16_t red = colour.r;
	uint16_t green = colour.g;
	uint16_t blue = colour.b;

	red *= brightness;
	green *= brightness;
	blue *= brightness;

	red /= BRIGHTNESS_MAX;
	green /= BRIGHTNESS_MAX;
	blue /= BRIGHTNESS_MAX;

	return pixels.Color(red, green, blue);
}

/**
 * Solid static colour
 */
void run_solid() {
	uint32_t led_colour = get_scaled_colour(get_colour(INTERPOLATE));
	for(int i = 0; i < PIXEL_COUNT; i++) { // For each pixel...
		pixels.setPixelColor(i, led_colour);
		pixels.show();
	}
}

/**
 * sine wave brightness
 */
void run_breating() {
	uint32_t breathing_level = millis() / BREATHING_STEP_MS;
	uint32_t led_colour;
	uint32_t colour_state;

	colour_state = breathing_level / 200;
	breathing_level %= 200;
	if (breathing_level > 100) {
		breathing_level = 200 - breathing_level;
	}
	breathing_level = map(breathing_level, 0, 100, 0, current_state.brightness);
	led_colour = get_scaled_colour(get_colour(STEP, colour_state), breathing_level);

	for(int i = 0; i < PIXEL_COUNT; i++) { // For each pixel...
		pixels.setPixelColor(i, led_colour);
		pixels.show();
	}
}

/**
 * rotating pulse with tail
 */
void run_chasing() {
	uint32_t chasing_angle = (millis() / BREATHING_STEP_MS) % 360;
	uint32_t led_colour = get_scaled_colour(get_colour(INTERPOLATE), 100);
	if (chasing_angle > 300) {
		// TODO handle loop around
	}
	set_led(INNER, 0, led_colour);
	set_led(MIDDLE, 0, led_colour);
	set_led(OUTER, 0, led_colour);
	set_led(OUTER, 30, pixels.Color(0,50,0));
	set_led(OUTER, 330, pixels.Color(0,0,50));
	pixels.show();
}

/**
 * pulse from center out
 */
void run_wave() {
	uint32_t wave_state = (millis() / WAVE_STEP_MS) % 400;
	uint32_t wave_phase = wave_state / 100;
	uint32_t wave_brightness = wave_state % 100;
	struct rgb led_colour = get_colour(INTERPOLATE);
	uint32_t c1, c2;
	wave_brightness = map(wave_brightness, 0, 100, 0, current_state.brightness);

	switch (wave_phase) {
	case 0:
		// increasing inner ring
		c1 = get_scaled_colour(led_colour, wave_brightness);
		set_led(INNER, 0, c1);
		break;
	case 1:
		// decreasing inner to increasing middle
		c1 = get_scaled_colour(led_colour, wave_brightness);
		c2 = get_scaled_colour(led_colour, current_state.brightness - wave_brightness);
		set_ring(MIDDLE, c1);
		set_ring(INNER, c2);
		break;
	case 2:
		// decreasing middle to increasing outer
		c1 = get_scaled_colour(led_colour, wave_brightness);
		c2 = get_scaled_colour(led_colour, current_state.brightness - wave_brightness);
		set_ring(OUTER, c1);
		set_ring(MIDDLE, c2);
		break;
	case 3:
		// decreasing outer
		c1 = get_scaled_colour(led_colour, current_state.brightness - wave_brightness);
		set_ring(OUTER, c1);
		break;
	}
	pixels.show();
}

void handle_button() {
	bool rollover;
	btn.read();
	if (!current_state.on && btn.wasReleased()) {
		current_state.on = true;
		return;
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
			pixels.clear();
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
		pixels.clear();
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
	handle_button();
	if (current_state.on && !current_state.long_press) {
		handle_animations();
	}
}

