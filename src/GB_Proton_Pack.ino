#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

/**********************************************************************************************************************
 * Power Cell Globals
**********************************************************************************************************************/

#define POWER_CELL_PIN            5
#define POWER_CELL_NUMPIXELS      13

unsigned long power_cells_last_event_time = 0;

int power_cells_delay = 40;
int power_cells_index = 0;
int power_cells_brightness = 250;

Adafruit_NeoPixel power_cells = Adafruit_NeoPixel(
                                  POWER_CELL_NUMPIXELS,
                                  POWER_CELL_PIN,
                                  NEO_GRB + NEO_KHZ800
                                );

/**********************************************************************************************************************
 * Cyclotron Globals
**********************************************************************************************************************/

#define CYCLOTRON_PIN             6
#define CYCLOTRON_NUMPIXELS       4

unsigned long cyclotron_cells_last_event_time = 0;

int cyclotron_cells_delay = 600;
int cyclotron_cells_index = 0;
int current_cyclotron_cell = 0;
int cyclotron_brightness = 250;

Adafruit_NeoPixel cyclotron = Adafruit_NeoPixel(
                                CYCLOTRON_NUMPIXELS,
                                CYCLOTRON_PIN,
                                NEO_GRB + NEO_KHZ800
                              );

/**********************************************************************************************************************
 * Trigger Globals
**********************************************************************************************************************/

#define BARREL_PIN            2
#define BARREL_NUMPIXELS      1

unsigned long barrel_last_event_time = 0;

int barrel_delay = 20; // This is set by the random number generator.
int barrel_brightness = 250;
boolean barrel_on = false;

uint32_t barrel_color;

Adafruit_NeoPixel barrel = Adafruit_NeoPixel(
                                  BARREL_NUMPIXELS,
                                  BARREL_PIN,
                                  NEO_GRB + NEO_KHZ800
                                );

void setup() {
//  Serial.begin(9600);
//  Serial.println("");
  power_cells.begin();
  power_cells.setBrightness(power_cells_brightness);
  power_cells.show();

  cyclotron.begin();
  cyclotron.setBrightness(cyclotron_brightness);
  cyclotron.show();

  barrel.begin();
  barrel.setBrightness(barrel_brightness);
  barrel.show();
}

void loop() {

  unsigned long current_time = millis();

  if (current_time - power_cells_last_event_time >= power_cells_delay) {
    power_cells_last_event_time = millis();
    power_cells_index++;
    
    if (power_cells_index < POWER_CELL_NUMPIXELS) {
      power_cells.setPixelColor(power_cells_index, power_cells.Color(0, 0, 150));
    } else {
      power_cells_index = 0;
      for (int i = 0; i < POWER_CELL_NUMPIXELS; i++) {
        power_cells.setPixelColor(i, power_cells.Color(0, 0, 0));
      }
    }
    power_cells.show();
  }

  if (current_time - cyclotron_cells_last_event_time >= cyclotron_cells_delay) {
    cyclotron_cells_last_event_time = millis();
    int previous_cell_index = cyclotron_cells_index;

    cyclotron_cells_index = ++cyclotron_cells_index % CYCLOTRON_NUMPIXELS;

    cyclotron.setPixelColor(previous_cell_index, cyclotron.Color(0, 0, 0));
    cyclotron.setPixelColor(cyclotron_cells_index, cyclotron.Color(150, 0, 0));
    cyclotron.show();
  }

  // This will need to be behind the trigger button code.
  if (current_time - barrel_last_event_time >= barrel_delay) {
    barrel_last_event_time = millis();
    if (barrel_on) {
      barrel_on = false;
      barrel_color = barrel.Color(255, 255, 255);
    } else {
      barrel_on = true;
      barrel_color = barrel.Color(0, 0, 0);
    }
    for (int i = 0; i < BARREL_NUMPIXELS; i++) {
      barrel.setPixelColor(i, barrel_color); 
    }
    
    barrel.show();
    barrel_delay = random(20, 80);
  }
}
