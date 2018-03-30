#include <SoftwareSerial.h>
#include "Adafruit_Soundboard.h"
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

/**********************************************************************************************************************
 * PIN Assignment
**********************************************************************************************************************/

// Sound Board PIN 2 - 5
#define SOUND_BOARD_RX_PIN    2
#define SOUND_BOARD_TX_PIN    3
#define SOUND_BOARD_RS_PIN    4
#define SOUND_BOARD_ACT_PIN   5

// LED PINs 6 - 8
#define BARREL_LED_PIN        6
#define CYCLOTRON_LED_PIN     7
#define POWER_CELL_LED_PIN    8

// Switch PINs 9 - 11
#define POWER_SWITCH_PIN      11
#define TRIGGER_SWITCH_PIN    12
#define SAFETY_SWITCH_PIN     13


/**********************************************************************************************************************
 * Power Cell Globals
**********************************************************************************************************************/

#define POWER_CELL_NUMPIXELS      16

unsigned long power_cells_last_event_time = 0;
int last_power_switch_state = LOW;

int power_cells_delay = 40;
int power_cells_index = 0;
int power_cells_brightness = 20;

Adafruit_NeoPixel power_cells = Adafruit_NeoPixel(
                                  POWER_CELL_NUMPIXELS,
                                  POWER_CELL_LED_PIN,
                                  NEO_GRB + NEO_KHZ800
                                );

/**********************************************************************************************************************
 * Cyclotron Globals
**********************************************************************************************************************/

#define CYCLOTRON_NUMPIXELS       4

unsigned long cyclotron_cells_last_event_time = 0;

int cyclotron_cells_delay = 600;
int cyclotron_cells_index = 0;
int current_cyclotron_cell = 0;
int cyclotron_brightness = 20;

Adafruit_NeoPixel cyclotron = Adafruit_NeoPixel(
                                CYCLOTRON_NUMPIXELS,
                                CYCLOTRON_LED_PIN,
                                NEO_GRB + NEO_KHZ800
                              );

/**********************************************************************************************************************
 * Trigger/Barrel Globals
**********************************************************************************************************************/

#define BARREL_NUMPIXELS      4

unsigned long barrel_last_event_time = 0;
unsigned long firing_event_time = 0;
int last_trigger_state = LOW;

int barrel_delay = 20; // This is set by the random number generator.
int barrel_brightness = 20;

uint32_t barrel_color;

Adafruit_NeoPixel barrel = Adafruit_NeoPixel(
                                  BARREL_NUMPIXELS,
                                  BARREL_LED_PIN,
                                  NEO_GRB + NEO_KHZ800
                                );

/**********************************************************************************************************************
 * Soundboard Globals
**********************************************************************************************************************/

SoftwareSerial softSerial = SoftwareSerial(SOUND_BOARD_TX_PIN, SOUND_BOARD_RX_PIN);
Adafruit_Soundboard soundBoard = Adafruit_Soundboard(&softSerial, NULL, SOUND_BOARD_RS_PIN);

boolean soundBoardEnabled = true;
uint32_t track_current_position, track_length;

char startupTrack[] =   "T00     WAV";
char shutdownTrack[] =  "T01     WAV";
char idleTrack[] =      "T02     WAV";
char firing[] =         "T03     WAV";
char firingDone[] =     "T04     WAV";

char bursthead[] =      "BURSTHDRWAV";
char burstloop[] =      "BURSTLP WAV";
char bursttail[] =      "BURSTT  WAV";
char powerdown[] =      "POWERDN WAV";
char powerhum[] =       "POWERHUMWAV";
char powerup[] =        "POWERUP WAV";

/**********************************************************************************************************************
 * Misc Globals
**********************************************************************************************************************/

boolean hasPoweredUp = false;
boolean shutdownComlete = true;
boolean isFiringStream = false;

/**********************************************************************************************************************
 * End Of Globals
**********************************************************************************************************************/

void setup() {
  Serial.begin(115200);
  Serial.println("Ghostbusters Proton Pack Initializing...");
  Serial.println("Lead Engineer: Egon Spengler");
  Serial.println("---------------------------------------------------------------------");

  softSerial.begin(9600);

  if (!soundBoard.reset()) {
    soundBoardEnabled = false;
    Serial.println("Soundboard failed to initialize.");
  }

  power_cells.begin();
  power_cells.setBrightness(power_cells_brightness);
  power_cells.show();

  cyclotron.begin();
  cyclotron.setBrightness(cyclotron_brightness);
  cyclotron.show();

  barrel.begin();
  barrel.setBrightness(barrel_brightness);
  barrel.show();

  pinMode(POWER_SWITCH_PIN, INPUT_PULLUP);
  pinMode(TRIGGER_SWITCH_PIN, INPUT_PULLUP);
  pinMode(SOUND_BOARD_ACT_PIN, INPUT);
}

void loop() {
  unsigned long current_time = millis();
  int power_switch_state = digitalRead(POWER_SWITCH_PIN);

  boolean isTrackPlaying = ! (boolean) digitalRead(SOUND_BOARD_ACT_PIN);

  if (last_power_switch_state != power_switch_state) {
    Serial.print("Main power is");
    if (power_switch_state == HIGH) {
      Serial.println(" on.");
      hasPoweredUp = true;
      playAudio(powerup, isTrackPlaying);
    } else {
      Serial.println(" off.");
      hasPoweredUp = false;
      playAudio(powerdown, isTrackPlaying);
      for (int i = 0; i < POWER_CELL_NUMPIXELS; i++) {
        power_cells.setPixelColor(i, power_cells.Color(0, 0, 0));
      }
      for (int i = 0; i < CYCLOTRON_NUMPIXELS; i++) {
        cyclotron.setPixelColor(i, cyclotron.Color(0, 0, 0));
      }
      for (int i = 0; i < BARREL_NUMPIXELS; i++) {
        barrel.setPixelColor(i, barrel.Color(0, 0, 0));
      }
      power_cells.show();
      cyclotron.show();
      barrel.show();
    }
    last_power_switch_state = power_switch_state;
  }

  if (hasPoweredUp && !isTrackPlaying && !isFiringStream) {
    playAudio(powerhum, isTrackPlaying);
  }

  // If the power switch is not on, we do nothing.
  if (power_switch_state == HIGH) {
    int trigger_state = digitalRead(TRIGGER_SWITCH_PIN);
    if (last_trigger_state != trigger_state) {
      if (trigger_state == HIGH) {
        firing_event_time = millis();
        if (!isTrackPlaying && !isFiringStream) {
          isFiringStream = true;
          playAudio(bursthead, isTrackPlaying);
        }
        Serial.println("Firing proton stream.");
      } else {
        Serial.print("Proton stream fired for ");
        Serial.print(current_time - firing_event_time);
        Serial.println(" milliseconds.");
      }
      last_trigger_state = trigger_state;
    }
    
    if (current_time - power_cells_last_event_time >= power_cells_delay) {
      power_cells_last_event_time = millis();
      if (power_cells_index < POWER_CELL_NUMPIXELS) {
        // Turn Power Cell LEDs on sequentially.
        power_cells.setPixelColor(power_cells_index, power_cells.Color(0, 0, 150));
        power_cells_index++;
      } else {
        // Turn off all Power Cell LEDs.
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
  
    if (trigger_state == HIGH) {
      if (!isFiringStream) {
        soundBoard.stop();
      }
      if (!isTrackPlaying && isFiringStream) {
        isFiringStream = true;
        playAudio(burstloop, isTrackPlaying);
      }
      if (current_time - barrel_last_event_time >= barrel_delay) {
        barrel_last_event_time = millis();
        if (barrel.getPixelColor(0) == 0) {
          barrel_color = barrel.Color(255, 255, 255);
        } else {
          barrel_color = barrel.Color(0, 0, 0);
        }
        for (int i = 0; i < BARREL_NUMPIXELS; i++) {
          barrel.setPixelColor(i, barrel_color);
        }
        
        barrel.show();
        // We need a random number to ensure that the barrel flashing looks realistic. This range works nicely.
        barrel_delay = random(20, 80);
      }
    } else {
      if (isFiringStream) {
        isFiringStream = false;
        playAudio(bursttail, true);
      }
      for (int i = 0; i < BARREL_NUMPIXELS; i++) {
        barrel.setPixelColor(i, barrel.Color(0, 0, 0)); 
      }
      barrel.show();
    }
  }
}

void playAudio(char* trackname, boolean isTrackPlaying) {
  if (isTrackPlaying) {
    soundBoard.stop();
  }

  if (soundBoard.playTrack(trackname)) {
    soundBoard.unpause();
  }
}
