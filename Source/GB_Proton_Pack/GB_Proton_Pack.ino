/**********************************************************************************************************************
 * Ghostbuster Proton Pack Lights and Sounds.
 * ver. 2.4 April 2018
 * Platform: Arduino UNO, Adafruit Audio FX Sound Board - WAV/OGG Trigger with 16MB Flash
**********************************************************************************************************************/
#include <SoftwareSerial.h>
#include "Adafruit_Soundboard.h"
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

/**********************************************************************************************************************
 * PIN Assignment
 * 
 * Soundboard
 * 
 * LEDs
 *      Barrel
 *      Power Cells
 *      Cyclotron
 *      Bar graph
 *      Hat
 *      Vent
 *      
 * Sound Frequency Analysis
 *      Requires 2 analog pins
 * 
 * Toggle switches : 3 on the neutrino wand.
 * 
 * Push Button switches : 2 also on the neutrino wand.
 * 
**********************************************************************************************************************/

/**********************************************************************************************************************
 * Soundboard PINs 2 - 5 and A5 for Frequency Input
**********************************************************************************************************************/
#define SOUND_BOARD_TX_PIN 2
#define SOUND_BOARD_RX_PIN 3
#define SOUND_BOARD_RS_PIN 4
#define SOUND_BOARD_ACT_PIN 5

// This is for the Color organ, I wanted to have the pack respond to the music when played.
#define SOUND_FREQUENCY_ACT_PIN A5 

/**********************************************************************************************************************
 * LED PINs 6 - 11
**********************************************************************************************************************/
#define POWER_CELL_LED_PIN 6
#define BARREL_LED_PIN 7
#define CYCLOTRON_LED_PIN 8
#define BARGRAPH_LED_PIN 9
#define HAT_LED_PIN 10
#define VENT_LED_PIN 11

/**********************************************************************************************************************
 * Switch PINs 14 - 18
**********************************************************************************************************************/
#define ACTIVATE_SWITCH_PIN 14
#define BARGRAPH_ONE_SWITCH_PIN 15
#define BARGRAPH_TWO_SWITCH_PIN 16

#define INTENSITY_BUTTON_PIN 17
#define TRIGGER_BUTTON_PIN 18

/**********************************************************************************************************************
 * Setup Globals
**********************************************************************************************************************/

// Event time tracking.
unsigned long power_cells_last_event_time = 0;
unsigned long cyclotron_cells_last_event_time = 0;
unsigned long barrel_last_event_time = 0;
unsigned long blast_event_time = 0;

// PIN States
int default_intensity_state = HIGH;
int default_trigger_state = HIGH;

int last_activate_switch_state = LOW;
int last_bargraph_one_switch_state = LOW;
int last_bargraph_two_switch_state = LOW;

int last_intensity_button_state = default_intensity_state;
int last_trigger_button_state = default_trigger_state;

boolean isPowerOn = false;
boolean isSafetyOn = false;
boolean isSecondarySafetyOn = false;

boolean isSongPlaying = false;

// Delays
int power_cells_idle_delay = 50;
int power_cells_firing_delay = 20;
int cyclotron_cells_idle_delay = 600;
int cyclotron_cells_firing_delay = 200;

// The value of this variable gets randomized.
int barrel_led_flash_delay = 20;

int current_power_cell_index = 0;
int current_cyclotron_cell_index = 0;

int power_cells_brightness = 50;
int cyclotron_brightness = 20;
int barrel_brightness = 250;

uint32_t barrel_color;

/**********************************************************************************************************************
 * Setup WS2812B Objects
**********************************************************************************************************************/

#define POWER_CELL_NUMPIXELS 16
#define CYCLOTRON_NUMPIXELS 4
#define BARREL_NUMPIXELS 6

Adafruit_NeoPixel power_cells = Adafruit_NeoPixel(
    POWER_CELL_NUMPIXELS,
    POWER_CELL_LED_PIN,
    NEO_GRB + NEO_KHZ800
);

Adafruit_NeoPixel cyclotron = Adafruit_NeoPixel(
    CYCLOTRON_NUMPIXELS,
    CYCLOTRON_LED_PIN,
    NEO_GRB + NEO_KHZ800
);

Adafruit_NeoPixel barrel = Adafruit_NeoPixel(
    BARREL_NUMPIXELS,
    BARREL_LED_PIN,
    NEO_GRB + NEO_KHZ800
);

/**********************************************************************************************************************
 * Soundboard Globals
**********************************************************************************************************************/

SoftwareSerial softSerial = SoftwareSerial(SOUND_BOARD_TX_PIN, SOUND_BOARD_RX_PIN);
Adafruit_Soundboard soundBoard = Adafruit_Soundboard( & softSerial, NULL, SOUND_BOARD_RS_PIN);

uint16_t default_sound_volume = 180;
boolean sountboard_volume_adjusted = false;

boolean soundBoardEnabled = true;
uint32_t track_current_position, track_length;

char bursthead[]    =  "BURSTHDRWAV";
char burstloop[]    =  "BURSTLP WAV";
char bursttail[]    =  "BURSTT  WAV";
char powerdown[]    =  "POWERDN WAV";
char powerhum[]     =  "POWERHUMWAV";
char powerup[]      =  "POWERUP WAV";

char *songs[] = {"GBTHEME OGG", "CLEANINGOGG"};

/**********************************************************************************************************************
 * Misc Globals
**********************************************************************************************************************/

boolean isProtonPackActive = false;
boolean isNeutrinoFiring = false;
boolean isAudioPlaying = false;

/**********************************************************************************************************************
 * End Of Globals
**********************************************************************************************************************/

void setup() {
    Serial.begin(115200);
    Serial.println("Ghostbusters II Proton Pack");
    Serial.println("v2.4 April 2018");
    Serial.println("Engineer: Egon Spengler");
    repeatChar('-', 70);

    softSerial.begin(9600);

    if (soundBoard.reset()) {
        uint16_t current_volume = soundBoard.volDown();
        while (current_volume > default_sound_volume) {
            current_volume = soundBoard.volDown();
        }
        Serial.print("Volume set to: "); 
        Serial.println(current_volume);
    } else {
        soundBoardEnabled = false;
        Serial.println("Soundboard failed to initialize.");
    }

    repeatChar('-', 70);

    power_cells.begin();
    power_cells.setBrightness(power_cells_brightness);
    power_cells.show();

    cyclotron.begin();
    cyclotron.setBrightness(cyclotron_brightness);
    cyclotron.show();

    barrel.begin();
    barrel.setBrightness(barrel_brightness);
    barrel.show();

    pinMode(ACTIVATE_SWITCH_PIN, INPUT_PULLUP);
    pinMode(BARGRAPH_ONE_SWITCH_PIN, INPUT_PULLUP);
    pinMode(BARGRAPH_TWO_SWITCH_PIN, INPUT_PULLUP);

    pinMode(INTENSITY_BUTTON_PIN, INPUT_PULLUP);
    pinMode(TRIGGER_BUTTON_PIN, INPUT_PULLUP);

    pinMode(SOUND_BOARD_ACT_PIN, INPUT);
}

void loop() {
    unsigned long current_time = millis();

    isAudioPlaying = !(boolean) digitalRead(SOUND_BOARD_ACT_PIN);

    isPowerOn = isActivateSwitchActive();
    isSafetyOn = isBargraphOneSwitchActive();
    isSecondarySafetyOn = isBargraphTwoSwitchActive();

    boolean isIntensityButtonOn = isIntensityButtonActive();
    boolean isTriggerButtonOn = isTriggerButtonActive();

    if (isPowerOn) {
        if (!isAudioPlaying && !isNeutrinoFiring) {
            playAudio(powerhum, false);
        }

        // Set the speed of the power cells.
        int power_cells_speed = isTriggerButtonOn ? power_cells_firing_delay : power_cells_idle_delay;

        if (current_time - power_cells_last_event_time >= power_cells_speed) {
            power_cells_last_event_time = millis();

            if (current_power_cell_index < POWER_CELL_NUMPIXELS) {
                // Turn Power Cell LEDs on sequentially.
                power_cells.setPixelColor(current_power_cell_index, power_cells.Color(0, 0, 150));
                power_cells.show();
                current_power_cell_index++;
            } else {
                turnPowerCellsOff();
            }
        }

        int cyclotron_cells_speed = isTriggerButtonOn ? cyclotron_cells_firing_delay : cyclotron_cells_idle_delay;

        if (current_time - cyclotron_cells_last_event_time >= cyclotron_cells_speed) {
            cyclotron_cells_last_event_time = millis();

            int previous_cyclotron_cell_index = current_cyclotron_cell_index;
            current_cyclotron_cell_index = ++current_cyclotron_cell_index % CYCLOTRON_NUMPIXELS;

            cyclotron.setPixelColor(previous_cyclotron_cell_index, cyclotron.Color(0, 0, 0));
            cyclotron.setPixelColor(current_cyclotron_cell_index, cyclotron.Color(150, 0, 0));
            cyclotron.show();
        }

        if (isTriggerButtonOn && !isSafetyOn && !isSecondarySafetyOn) {
            if (!isAudioPlaying) {
                playAudio(burstloop, false);
            }

            if (current_time - barrel_last_event_time >= barrel_led_flash_delay) {
                barrel_last_event_time = millis();

                if (barrel.getPixelColor(0) == 0) { // Are the LEDs off?
                    barrel_color = barrel.Color(255, 255, 255); // Turn them on.
                } else {
                    barrel_color = barrel.Color(0, 0, 0); // Turn them off.
                }

                setBarrelColor(barrel_color);
                barrel_led_flash_delay = random(20, 80);
            }
        }
    }
}

/**********************************************************************************************************************
 * Switch/Button State Checks
**********************************************************************************************************************/

boolean isActivateSwitchActive() {
    int this_switch_state = digitalRead(ACTIVATE_SWITCH_PIN);

    if (this_switch_state != last_activate_switch_state) {
        Serial.print("Main power is");
        if (this_switch_state == HIGH) {
            Serial.println(" on.");
            isProtonPackActive = true;
             playAudio(powerup, false);
        } else {
            Serial.println(" off.");
            isProtonPackActive = false;
            allLedsOff();
             playAudio(powerdown, true);
        }
        last_activate_switch_state = this_switch_state;
    }

    return this_switch_state == HIGH;
}

boolean isBargraphOneSwitchActive() {
    int this_switch_state = digitalRead(BARGRAPH_ONE_SWITCH_PIN);

    if (this_switch_state != last_bargraph_one_switch_state) {
        last_bargraph_one_switch_state = this_switch_state;
    }

    return this_switch_state == HIGH;
}

boolean isBargraphTwoSwitchActive() {
    int this_switch_state = digitalRead(BARGRAPH_TWO_SWITCH_PIN);

    if (this_switch_state != last_bargraph_two_switch_state) {
        last_bargraph_two_switch_state = this_switch_state;
    }

    return this_switch_state == HIGH;
}

boolean isIntensityButtonActive() {
    int this_button_state = digitalRead(INTENSITY_BUTTON_PIN);

    if (this_button_state != last_intensity_button_state) {
        last_intensity_button_state = this_button_state;
    }

    return this_button_state == default_intensity_state;
}

boolean isTriggerButtonActive() {
    int this_button_state = digitalRead(TRIGGER_BUTTON_PIN);

    if (isPowerOn && this_button_state != last_trigger_button_state) {
        if (this_button_state == !default_trigger_state && !isSafetyOn && !isSecondarySafetyOn) {
            isNeutrinoFiring = true;
            playAudio(bursthead, true);
        } else if (isSafetyOn && !isSecondarySafetyOn) {
            playSong(0); // Play Ray Parker's "Ghostbusters"
        } else if (!isSafetyOn && isSecondarySafetyOn) {
            playSong(1); // Play The Busboys' "Cleanin' Up the Town"
        } else if (isSafetyOn && isSecondarySafetyOn) {
            playSong(-1); // TDB: Play each song in the song array.
        } else {
            isNeutrinoFiring = false;
            playAudio(bursttail, true);
            turnBarrelOff();
        }
        last_trigger_button_state = this_button_state;
    }

    return this_button_state != default_trigger_state;
}

void allLedsOff() {
    for (int i = 0; i < CYCLOTRON_NUMPIXELS; i++) {
        cyclotron.setPixelColor(i, cyclotron.Color(0, 0, 0));
    }

    turnPowerCellsOff();
    turnBarrelOff();
    cyclotron.show();
}

void turnPowerCellsOff() {
    current_power_cell_index = 0;
    for (int i = 0; i < POWER_CELL_NUMPIXELS; i++) {
        power_cells.setPixelColor(i, power_cells.Color(0, 0, 0));
    }
    power_cells.show();
}

void turnBarrelOff() {
    setBarrelColor(barrel.Color(0, 0, 0));
}

void setBarrelColor(uint32_t barrel_color) {
    for (int i = 0; i < BARREL_NUMPIXELS; i++) {
        barrel.setPixelColor(i, barrel_color);
    }
    barrel.show();
}

void playSong(int songIndex) {
    if (songIndex > -1) {
        playAudio(songs[songIndex], true);
    }
}

void playAudio(char * trackname, boolean forceStop) {
    if (forceStop) {
        soundBoard.stop();
    }

    if (soundBoard.playTrack(trackname)) {
        soundBoard.unpause();
    }
}

void repeatChar(char str, int count) {
    while (count-- > 1)
        Serial.print(str);
    Serial.println(str);
}
