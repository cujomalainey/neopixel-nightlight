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
	.brightness = 100,
	.colour     = PINK,
	.mode       = SOLID,
};

struct state previous_state = current_state;

Adafruit_NeoPixel strip(PIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
Button btn(BUTTON_PIN);

void reset_state() {
    current_state = {
        .brightness = 100,
        .colour     = PINK,
        .mode       = SOLID,
    };
    previous_state = current_state;
}

void handle_button() {
    btn.read();
    // TODO
}

void run_solid() {
    // TODO
}

void run_breating() {
    // TODO
}

void run_chasing() {
    // TODO
}

void run_wave() {
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
    handle_animations();
}

