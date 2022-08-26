#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>

//External libraries
#include <Adafruit_GFX.h>     //Adafruit GFX Library  by Adafruit             https://github.com/adafruit/Adafruit-GFX-Library          (Avaliable in Arduino IDE)
#include <Adafruit_SSD1306.h> //Adafruit SSD1306      by Adafruit             https://github.com/adafruit/Adafruit_SSD1306              (Avaliable in Arduino IDE)
#include <MIDI.h>             //Midi Library          by Forty Seven Effects  https://github.com/FortySevenEffects/arduino_midi_library (Avaliable in Arduino IDE)
#include <Encoder.h>          //Encoder               by Paul Stoffregen      https://www.pjrc.com/teensy/td_libs_Encoder.html          (Avaliable in Arduino IDE)
#include <AceButton.h>        //AceButton             by Brian T. Parks       https://github.com/bxparks/AceButton                      (Avaliable in Arduino IDE)
using namespace ace_button;
//NOTE: You'll totally need to remove the data from the Adafruit SSD1306 "splash.h" file if you want this to fit on a leonardo style device :P (Just set widths and heights to 0 and empty the arrays)

//Synth configuration
const byte voiceCount = 6; //Number of voices. Counts from IIC addr 0x00 upwards.
const byte sampleCount = 32; //Number of samples per waveform. Must be same for all voices!
const byte envelopeCount = 5; //Number of steps in envelope. Must be same for all voices!
const byte channelCount = 16; //Number of channels. Standard midi has 16 channels.

//I/O configuration
const byte ROT_ENC_PINS[2][3] = {{6, 5, 4}, {8, 7, 9}};
const byte SCREEN_ADDR = 0x3C;
const byte SCREEN_WIDTH = 128;
const byte SCREEN_HEIGHT = 64;

//I²C commands. See planning doc for reference
const byte COMMAND_NOTE_ON = 0x01;
const byte COMMAND_NOTE_OFF = 0x02;
const byte COMMAND_DATA_SAVE = 0x03;
const byte COMMAND_DATA_LOAD = 0x04;
const byte COMMAND_PITCH_BEND = 0x05;
const byte COMMAND_REG_SAMPLE = 0x10;
const byte COMMAND_REG_ENVELOPE = 0x30;

//Enums and consts
enum Envelope {ATTACK, DECAY, SUSTAIN, RELEASE, PBSPAN};
enum InPos {LEFT, RIGHT};
enum Screen {SPLASH, HOME, MENU, CHASS, ENVELOPE, WAVSUBMEN, WAVCUSTOM};
enum Waveforms {SINE, SQUARE, SAW, TRIANGLE, NOISE, CUSTOM};

//Bitmap illustrations
enum Bitmaps {MAE, HEART, BOLT, PENTAGRAM};
const byte bitmapWidths[] = {40, 23, 18, 15};
const byte bitmapHeights[] = {29, 22, 46, 15};
const byte bitmapX[] = {43, 14, 90, 22};
const byte bitmapY[] = {17, 9, 9, 41};
// 'mae', 40x29px
const unsigned char bitmap1 [] PROGMEM = {
  0xf8, 0x00, 0x07, 0x00, 0x1f, 0x8e, 0x00, 0x7d, 0xc0, 0x71, 0xc3, 0x81, 0xc1, 0x41, 0xc1, 0x60,
  0xff, 0x00, 0x7f, 0x03, 0x60, 0x60, 0x00, 0x06, 0x02, 0x40, 0x00, 0x00, 0x00, 0x02, 0x70, 0x00,
  0x00, 0x00, 0x06, 0x38, 0x00, 0x00, 0x00, 0x04, 0x60, 0x00, 0x00, 0x00, 0x04, 0x40, 0xfc, 0x00,
  0x3f, 0x04, 0x63, 0x87, 0x00, 0xe1, 0xc6, 0x4f, 0x03, 0xc3, 0xc0, 0xf2, 0xdf, 0x03, 0xe7, 0xc0,
  0xfb, 0x8f, 0x03, 0xc3, 0xc0, 0xf1, 0x83, 0x87, 0x00, 0xe1, 0xc1, 0x80, 0xfc, 0x00, 0x3f, 0x01,
  0x80, 0x00, 0x3c, 0x00, 0x01, 0xc0, 0x10, 0x3c, 0x08, 0x03, 0x40, 0x0e, 0x18, 0x70, 0x02, 0x60,
  0x41, 0x3c, 0x81, 0x06, 0x20, 0x38, 0x24, 0x1e, 0x04, 0x30, 0x01, 0x24, 0x80, 0x0c, 0x1c, 0x06,
  0x66, 0x60, 0x38, 0x06, 0x08, 0x42, 0x10, 0x60, 0x03, 0x80, 0x42, 0x09, 0xc0, 0x00, 0xe0, 0x00,
  0x07, 0x00, 0x00, 0x3e, 0x00, 0x7c, 0x00, 0x00, 0x03, 0x00, 0xc0, 0x00, 0x00, 0x01, 0xff, 0x80,
  0x00
};
// 'heart', 23x22px
const unsigned char bitmap2 [] PROGMEM = {
  0x1e, 0x00, 0x00, 0x33, 0x80, 0x00, 0x40, 0xc3, 0xf8, 0xcc, 0x6e, 0x0c, 0x98, 0x38, 0x06, 0x90,
  0x11, 0xe2, 0x80, 0x30, 0x32, 0x80, 0x70, 0x12, 0xc0, 0x58, 0x12, 0x40, 0xc8, 0x02, 0x40, 0x88,
  0x04, 0x60, 0x8c, 0x0c, 0x21, 0xfc, 0x08, 0x31, 0x06, 0x38, 0x13, 0x02, 0xe0, 0x1a, 0x03, 0x80,
  0x0a, 0x07, 0x00, 0x0e, 0x0c, 0x00, 0x06, 0x18, 0x00, 0x03, 0x30, 0x00, 0x01, 0xa0, 0x00, 0x00,
  0xe0, 0x00
};
// 'bolt', 18x46px
const unsigned char bitmap3 [] PROGMEM = {
  0x00, 0x0e, 0x00, 0x00, 0x1f, 0x80, 0x00, 0x3c, 0xc0, 0x00, 0x7c, 0xc0, 0x00, 0xf8, 0x40, 0x01,
  0xf0, 0x40, 0x03, 0xe0, 0xc0, 0x03, 0xc0, 0x80, 0x07, 0x81, 0x80, 0x0f, 0x83, 0x00, 0x1f, 0x02,
  0x00, 0x1e, 0x06, 0x00, 0x3c, 0x0c, 0x00, 0x78, 0x18, 0x00, 0x70, 0x30, 0x00, 0xf0, 0x60, 0x00,
  0xe0, 0xc0, 0x00, 0xc0, 0x60, 0x00, 0x40, 0x30, 0x00, 0x60, 0x1c, 0x00, 0x30, 0x07, 0x00, 0x18,
  0x01, 0x80, 0x0c, 0x00, 0x40, 0x06, 0x00, 0x40, 0x03, 0x00, 0x40, 0x01, 0x80, 0x40, 0x00, 0xc0,
  0xc0, 0x00, 0xc0, 0x80, 0x01, 0xc1, 0x80, 0x01, 0xc3, 0x00, 0x03, 0x82, 0x00, 0x07, 0x06, 0x00,
  0x07, 0x0c, 0x00, 0x0e, 0x18, 0x00, 0x0c, 0x10, 0x00, 0x1c, 0x30, 0x00, 0x18, 0x60, 0x00, 0x38,
  0xc0, 0x00, 0x31, 0x80, 0x00, 0x73, 0x00, 0x00, 0x62, 0x00, 0x00, 0x66, 0x00, 0x00, 0xcc, 0x00,
  0x00, 0xd8, 0x00, 0x00, 0xb0, 0x00, 0x00, 0xe0, 0x00, 0x00
};
// 'pentagram', 15x15px
const unsigned char bitmap4 [] PROGMEM = {
  0x0f, 0xc0, 0x10, 0x70, 0x60, 0x48, 0x40, 0xcc, 0xbd, 0x46, 0x93, 0xc2, 0x8c, 0x62, 0x88, 0x5e,
  0x8c, 0x72, 0x97, 0xc2, 0xbe, 0x42, 0x41, 0x46, 0x40, 0xc4, 0x30, 0xd8, 0x0f, 0xe0
};

const unsigned char * bitmaps [] = {bitmap1, bitmap2, bitmap3, bitmap4};

const byte FONT_WIDTH = 6;
const byte FONT_HEIGHT = 8;
const byte ENC_DIVIDER = 4;
const char *menuTitles[] = {"Channel Assign", "Set Envelope+", "Set waveform"};
const char *envelopeInitials = "ADSRP";
const char *waveFormNames[] = {"Sine", "Square", "Saw", "Triangle", "Noise", "Custom"};
const byte waveFormNameLenghts[] = {4, 6, 3, 8, 5, 6};
const byte waveformCount = 6;
const Screen menuScreens[] = {CHASS, ENVELOPE, WAVSUBMEN};
const byte menuCount = 3;

//Globals
byte samples[sampleCount];
unsigned long envelope[envelopeCount];
byte assignedChannel[voiceCount];
byte playingNote[voiceCount];
int encVals[2];
byte recentScreenSwitch = false;
Screen currentScreen = SPLASH;
byte selectedChannel = 0;
bool staticHome = true;

//Inits
MIDI_CREATE_DEFAULT_INSTANCE();
Encoder leftEncoder(ROT_ENC_PINS[LEFT][0], ROT_ENC_PINS[LEFT][1]);
Encoder rightEncoder(ROT_ENC_PINS[RIGHT][0], ROT_ENC_PINS[RIGHT][1]);
AceButton leftButton(ROT_ENC_PINS[LEFT][2]);
AceButton rightButton(ROT_ENC_PINS[RIGHT][2]);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, 4);

//Handles
void handleNoteOn(byte inChannel, byte inNote, byte inVelocity)
{
  inVelocity = inVelocity * 2;
  inChannel -= 1;
  noteOn(getFirstVoice(inChannel, true), inNote, inVelocity);
  if (!staticHome) {
    redrawScreen();
  }
}

void handleNoteOff(byte inChannel, byte inNote, byte inVelocity)
{
  inChannel -= 1;
  noteOff(getPlayingVoice(inChannel, inNote));
  if (!staticHome) {
    redrawScreen();
  }
}

void handleAfterTouchPoly(byte inChannel, byte inNote, byte inVelocity) {
  inVelocity = inVelocity * 2;
  inChannel -= 1;
  noteOn(getPlayingVoice(inChannel, inNote), inNote, inVelocity);
  if (!staticHome) {
    redrawScreen();
  }
}

void handlePitchBend(byte channel, int bend) {
  channel -= 1;
  pitchBend(channel, bend);
}

void handleSystemReset() {
  for (byte i = 0; i < voiceCount; i++) {
    noteOff(i);
  }
  for (byte i = 0; i < channelCount; i++) {
    pitchBend(i, 0);
  }
}

void handleButtonEvent(AceButton* buttonRef, uint8_t eventType, uint8_t /* buttonState */) {
  if (eventType == AceButton::kEventPressed) {
    InPos button = getButton(buttonRef);
    switch (currentScreen) {
      case HOME:
        switch (button) {
          case RIGHT:
            changeScreen(MENU);
            break;
          case LEFT:
            //Turn off all voices
            for (byte i = 0; i < voiceCount; i++) {
              noteOff(i);
            }//Switch which home to display
            staticHome = !staticHome;
            break;
        }
        break;
      case MENU:
        switch (button) {
          case RIGHT:
            selectedChannel = encVals[RIGHT];
            changeScreen(menuScreens[encVals[LEFT]]);
            break;
          case LEFT:
            changeScreen(HOME);
            break;
        }
        break;
      case CHASS:
        switch (button) {
          case RIGHT:
            //Enter
            saveChAss();
            changeScreen(MENU);
            break;
          case LEFT:
            //Cancel
            loadChAss();
            changeScreen(MENU);
            break;
        }
        break;
      case ENVELOPE:
        switch (button) {
          case RIGHT:
            //Enter
            triggerChannelSave(selectedChannel);
            changeScreen(MENU);
            break;
          case LEFT:
            //Cancel
            triggerChannelLoad(selectedChannel);
            changeScreen(MENU);
            break;
        }
        break;
      case WAVSUBMEN:
        switch (button) {
          case RIGHT:
            if (encVals[LEFT] == CUSTOM) {
              changeScreen(WAVCUSTOM);
            } else {
              triggerChannelSave(selectedChannel);
              changeScreen(MENU);
            }
            //Enter
            break;
          case LEFT:
            //Cancel
            triggerChannelLoad(selectedChannel);
            changeScreen(MENU);
            break;
        }
        break;
      case WAVCUSTOM:
        switch (button) {
          case RIGHT:
            //Enter
            triggerChannelSave(selectedChannel);
            changeScreen(MENU);
            break;
          case LEFT:
            //Cancel
            triggerChannelLoad(selectedChannel);
            changeScreen(WAVSUBMEN);
            break;
        }
        break;
    }
    redrawScreen();
  }
}

void checkEncoders() {
  bool shouldHandleEncoders = false;
  int leftRead = leftEncoder.read();
  if (leftRead < -3 || leftRead > 3) {
    shouldHandleEncoders = true;
  }
  int rightRead = rightEncoder.read();
  if (rightRead < -3 || rightRead > 3) {
    shouldHandleEncoders = true;
  }
  if (shouldHandleEncoders) {
    handleEncoderChange(leftRead, rightRead);
    redrawScreen();
    leftEncoder.write(0);
    rightEncoder.write(0);
  }
}

void handleEncoderChange(int leftChange, int rightChange) {
  bool movingInto;
  switch (currentScreen) {
    case MENU:
      if (recentScreenSwitch) {
        encVals[LEFT] = 0;
        encVals[RIGHT] = 0;
        recentScreenSwitch = false;
      }
      encVals[LEFT] = wrapInt(encVals[LEFT] + (leftChange / ENC_DIVIDER), 0, menuCount);
      encVals[RIGHT] = wrapInt(encVals[RIGHT] + (rightChange / ENC_DIVIDER), 0, channelCount);
      break;
    case CHASS:
      movingInto = leftChange != 0;
      if (recentScreenSwitch) {
        encVals[LEFT] = 0;
        encVals[RIGHT] = 0;
        movingInto = true;
        recentScreenSwitch = false;
      }
      if (movingInto) {
        encVals[LEFT] = wrapInt(encVals[LEFT] + (leftChange / ENC_DIVIDER), 0, voiceCount);
        encVals[RIGHT] = assignedChannel[encVals[LEFT]];
      }

      encVals[RIGHT] = wrapInt(encVals[RIGHT] + (rightChange / ENC_DIVIDER), 0, channelCount);
      assignedChannel[encVals[LEFT]] = encVals[RIGHT];
      break;
    case ENVELOPE:
      movingInto = (leftChange != 0);
      if (recentScreenSwitch) {
        encVals[LEFT] = 0;
        encVals[RIGHT] = 0;
        getVoiEnvelope(getFirstVoice(selectedChannel, false));
        movingInto = true;
        recentScreenSwitch = false;
      }
      if (movingInto) {
        encVals[LEFT] = wrapInt(encVals[LEFT] + (leftChange / ENC_DIVIDER), 0, envelopeCount);
        encVals[RIGHT] = envelope[encVals[LEFT]];
      }
      if (encVals[LEFT] == SUSTAIN) {
        encVals[RIGHT] = wrapInt(encVals[RIGHT] + rightChange / (ENC_DIVIDER / 2), 0, 0xFF + 1);
      } else if (encVals[LEFT] == PBSPAN) {
        encVals[RIGHT] = wrapInt(encVals[RIGHT] + rightChange / ENC_DIVIDER, 0, 127 + 1);
      } else {
        encVals[RIGHT] = wrapInt(encVals[RIGHT] + rightChange * 4, 0, 8000);
      }
      envelope[encVals[LEFT]] = encVals[RIGHT];
      setChannelEnvelope(selectedChannel);
      break;
    case WAVSUBMEN:
      if (recentScreenSwitch) {
        encVals[LEFT] = CUSTOM;
      }
      encVals[LEFT] = wrapInt(encVals[LEFT] + (leftChange / ENC_DIVIDER), 0, waveformCount);
      switch (encVals[LEFT]) {
        case SINE:
          encVals[RIGHT] = 0;
          generateSinWave();
          break;
        case SQUARE:
          if (recentScreenSwitch) {
            encVals[RIGHT] = 5;
          }
          encVals[RIGHT] = wrapInt(encVals[RIGHT] + (rightChange / ENC_DIVIDER), 0, 10 + 1);
          generateSquareWave(float(encVals[RIGHT]) / 10.0);
          break;
        case SAW:
          if (recentScreenSwitch) {
            encVals[RIGHT] = 0;
          }
          encVals[RIGHT] = wrapInt(encVals[RIGHT] + (rightChange / ENC_DIVIDER), 0, 1 + 1);
          generateSawtoothWave(encVals[RIGHT] == 0);
          break;
        case TRIANGLE:
          generateTriangleWave();
          break;
        case NOISE:
          generateNoiseWave();
          break;
        case CUSTOM:
          getVoiWaveform(getFirstVoice(selectedChannel, false));
          break;
      }
      setChannelWaveform(selectedChannel);
      if (recentScreenSwitch) {
        recentScreenSwitch = false;
      }
      break;
    case WAVCUSTOM:
      movingInto = (leftChange != 0);
      if (recentScreenSwitch) {
        encVals[LEFT] = 0;
        encVals[RIGHT] = 0;
        movingInto = true;
        recentScreenSwitch = false;
      }
      if (movingInto) {
        encVals[LEFT] = wrapInt(encVals[LEFT] + (leftChange / ENC_DIVIDER), 0, sampleCount);
        encVals[RIGHT] = samples[encVals[LEFT]];
      }
      encVals[RIGHT] = wrapInt(encVals[RIGHT] + rightChange * 2, 0, 0xFF + 1);
      samples[encVals[LEFT]] = encVals[RIGHT];
      setChannelWaveform(selectedChannel);
      break;
  }
}

//Setup and loop
void setup() {
  Wire.begin();
  //Setup display
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDR);
  display.setTextColor(WHITE);
  redrawScreen();

  //Setup rotary encoder
  pinMode(ROT_ENC_PINS[LEFT][2], INPUT_PULLUP);
  pinMode(ROT_ENC_PINS[RIGHT][2], INPUT_PULLUP);

  ButtonConfig* buttonConfig = leftButton.getButtonConfig();
  buttonConfig->setEventHandler(handleButtonEvent);

  buttonConfig = rightButton.getButtonConfig();
  buttonConfig->setEventHandler(handleButtonEvent);

  //Setup synth + midi
  randomSeed(analogRead(0));
  loadChAss();
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandleAfterTouchPoly(handleAfterTouchPoly);
  MIDI.setHandlePitchBend(handlePitchBend);
  MIDI.setHandleSystemReset(handleSystemReset);
  MIDI.begin();

  //Wait for voices to come online
  delay(1000);
  //TODO consider adding pretty expanding line thingy?

  changeScreen(HOME);
  redrawScreen();
}

void loop() {
  MIDI.read();
  leftButton.check();
  rightButton.check();
  checkEncoders();
}

//Draws
void redrawScreen() {
  display.clearDisplay();
  switch (currentScreen) {
    case SPLASH:
      drawSplash();
      break;
    case HOME:
      drawHome();
      break;
    case MENU:
      drawMenu();
      break;
    case CHASS:
      drawChannelAssign();
      break;
    case ENVELOPE:
      drawEnvelope();
      break;
    case WAVSUBMEN:
      drawWaveformSubmenu();
      break;
    case WAVCUSTOM:
      drawWaveformCustom();
      break;
  }
  display.display();
}
void drawSplash() {
  display.setTextSize(2);
  display.setCursor((SCREEN_WIDTH - (2 * FONT_WIDTH * 5)) / 2, (SCREEN_HEIGHT / 2) - (2 * FONT_HEIGHT) - 2);
  display.print("MOSSY");
  display.setTextSize(1);
  display.setCursor((SCREEN_WIDTH - (FONT_WIDTH * 9)) / 2, SCREEN_HEIGHT / 2 + 4);
  display.print("WaveKitty");
  display.drawFastHLine(50, SCREEN_HEIGHT / 2, SCREEN_WIDTH - (50 * 2), WHITE);
}
void drawHome() {
  if (staticHome) {
    //Draw kitty and bolt and all kinda cool stuff
    for (byte i = 0; i < 4; i++) {
      display.drawBitmap(bitmapX[i], bitmapY[i], bitmaps[i], bitmapWidths[i], bitmapHeights[i], WHITE);
    }
  } else {
    display.setCursor((SCREEN_WIDTH - (FONT_WIDTH * 9)) / 2, 0);
    display.print("WaveKitty");
    for (byte i = 0; i < voiceCount; i++) {
      byte x = 8 + (i % 2) * 9 * FONT_WIDTH;
      byte y = 11 + (i / 2) * FONT_HEIGHT;
      display.setCursor(x, y);
      display.print(i, HEX);
      display.print("/");
      display.print(assignedChannel[i], HEX);
      display.print(": ");
      if (playingNote[i] != 0xFF) {
        display.print(playingNote[i], HEX);
      }
    }
    display.drawBitmap(110, 9, bitmaps[BOLT], bitmapWidths[BOLT], bitmapHeights[BOLT], WHITE);
  }
}
void drawMenu() {
  display.setCursor((SCREEN_WIDTH - (FONT_WIDTH * 4)) / 2, 0);
  display.print("Menu");
  drawCancelEnter();
  display.setCursor(0, 16);
  display.print(" -  ");
  display.println(menuTitles[wrapInt(encVals[LEFT] - 1, 0, menuCount)]);
  display.print("[");
  display.print(encVals[RIGHT], HEX);
  display.print("] ");
  display.println(menuTitles[encVals[LEFT]]);
  display.print(" -  ");
  display.println(menuTitles[wrapInt(encVals[LEFT] + 1, 0, menuCount)]);
  display.drawBitmap(53, 42, bitmaps[HEART], bitmapWidths[HEART], bitmapHeights[HEART], WHITE);
}
void drawChannelAssign() {
  display.setCursor((SCREEN_WIDTH - (FONT_WIDTH * 14)) / 2, 0);
  display.print(menuTitles[0]); //Channel Assign - 14
  display.setTextSize(2);
  display.setCursor(((SCREEN_WIDTH / 2) - (2 * FONT_WIDTH * 2)) / 2, (SCREEN_HEIGHT / 2) - (2 * FONT_HEIGHT) - 4);
  display.print("Vo");
  display.setCursor(((SCREEN_WIDTH / 2) - (2 * FONT_WIDTH * 2)) / 2, (SCREEN_HEIGHT / 2) + 4);
  display.print(encVals[LEFT], HEX);
  display.setCursor((SCREEN_WIDTH / 2) + (((SCREEN_WIDTH / 2) - (2 * FONT_WIDTH * 2)) / 2), (SCREEN_HEIGHT / 2) - (2 * FONT_HEIGHT) - 4);
  display.print("Ch");
  display.setCursor((SCREEN_WIDTH / 2) + (((SCREEN_WIDTH / 2) - (2 * FONT_WIDTH * 2)) / 2), (SCREEN_HEIGHT / 2) + 4);
  display.print(encVals[RIGHT], HEX);
  display.setTextSize(1);
  drawCancelEnter();
  display.drawBitmap(55, 9, bitmaps[BOLT], bitmapWidths[BOLT], bitmapHeights[BOLT], WHITE);
}
void drawEnvelope() {
  display.setCursor(0, 0);
  display.print(selectedChannel, HEX);
  display.setCursor((SCREEN_WIDTH - (FONT_WIDTH * 12)) / 2, 0);
  display.print(menuTitles[1]); //Set Envelope
  display.setTextSize(2);
  display.setCursor(8, SCREEN_HEIGHT - (FONT_HEIGHT + 2 * FONT_HEIGHT + 4));
  display.print(envelopeInitials[encVals[LEFT]]);
  display.print(": ");
  display.print(encVals[RIGHT]);
  display.setTextSize(1);
  drawCancelEnter();
  //Illustration begins at y 9, ends at 37, is 28 heigh
  //Illustration begins at x 8, ends at 119, is 111 wide
  //Assumes times dont exceed 2 seconds
  int attackLength = map(envelope[ATTACK], 0, 2000, 0, 30);
  int decayLength = map(envelope[DECAY], 0, 2000, 0, 30);
  int sustainHeight = map(envelope[SUSTAIN], 0, 0xFF, 37, 9);
  int releaseLength = map(envelope[RELEASE], 0, 2000, 0, 30);
  display.drawLine(8, 37, 8 + attackLength, 9, WHITE); //Attack
  display.drawLine(8 + attackLength, 9, 8 + attackLength + decayLength, sustainHeight, WHITE); //Decay
  display.drawLine(8 + attackLength + decayLength, sustainHeight, 119 - releaseLength, sustainHeight, WHITE); //Sustain
  display.drawLine(119 - releaseLength, sustainHeight, 119, 37, WHITE); //Release
}
void drawWaveformSubmenu() {
  display.setCursor(0, 0);
  display.print(selectedChannel, HEX);
  display.setCursor((SCREEN_WIDTH - (FONT_WIDTH * 12)) / 2, 0);
  display.print(menuTitles[2]); //Set Waveform - 12
  drawCancelEnter();
  display.setTextSize(2);
  display.setCursor((SCREEN_WIDTH - (2 * FONT_WIDTH * waveFormNameLenghts[encVals[LEFT]])) / 2, SCREEN_HEIGHT - (FONT_HEIGHT + 2 * FONT_HEIGHT + 4));
  display.print(waveFormNames[encVals[LEFT]]);
  display.setTextSize(1);

  for (byte i = 0; i < sampleCount; i++) {
    int yTop = map(samples[i], 0, 0xFF, 0, 28);
    int invYTop =  37 - yTop;
    display.fillRect(i * 4, invYTop, 4, yTop, WHITE);
  }
}
void drawWaveformCustom() {
  for (byte i = 0; i < sampleCount; i++) {
    int yTop = samples[i] / 4;
    int invYTop =  SCREEN_HEIGHT - yTop;
    int sampleRemainder = samples[i] % 4;
    display.fillRect(i * 4, invYTop, 4, yTop, WHITE);
    if (sampleRemainder != 0) {
      if (samples[i] < samples[i - 1]) {
        display.drawFastHLine(i * 4, invYTop - 1, sampleRemainder, WHITE);
      } else {
        display.drawFastHLine((i * 4) + 3, invYTop - 1, -sampleRemainder, WHITE);
      }
    }
  }
  display.fillRect(encVals[LEFT] * 4, 0, 4, SCREEN_HEIGHT, INVERSE);
}
void drawCancelEnter() {
  display.setCursor(8, SCREEN_HEIGHT - FONT_HEIGHT);
  display.print("Cancel");
  display.setCursor(SCREEN_WIDTH - 8 - (FONT_WIDTH * 5), SCREEN_HEIGHT - FONT_HEIGHT);
  display.print("Enter");
  display.drawBitmap(113, 0, bitmaps[PENTAGRAM], bitmapWidths[PENTAGRAM], bitmapHeights[PENTAGRAM], WHITE);
}

//Command senders

void noteOn(byte adress, byte note, byte volume) {
  Wire.beginTransmission(adress);
  Wire.write(COMMAND_NOTE_ON);
  Wire.write(note);
  Wire.write(volume);
  Wire.endTransmission();
  playingNote[adress] = note;
}

void noteOff(byte adress) {
  Wire.beginTransmission(adress);
  Wire.write(COMMAND_NOTE_OFF);
  Wire.endTransmission();
  playingNote[adress] = 0xFF;
}

void setVoiWaveform(byte adress) {
  for (byte i = 0; i < sampleCount; i++) {
    Wire.beginTransmission(adress);
    Wire.write(COMMAND_REG_SAMPLE + i);
    Wire.write(samples[i]);
    Wire.endTransmission();
  }
}

void setChannelWaveform(byte channel) {
  for (byte i = 0; i < voiceCount; i++) {
    if (assignedChannel[i] == channel) {
      setVoiWaveform(i);
    }
  }
}

void getVoiWaveform(byte adress) {
  Wire.beginTransmission(adress);
  Wire.write(COMMAND_REG_SAMPLE);
  Wire.endTransmission();
  for (byte i = 0; i < sampleCount; i++) {
    Wire.requestFrom(adress, 1);
    samples[i] = Wire.read();
  }
}


void setVoiEnvelope(byte adress) {
  for (byte i = 0; i < envelopeCount; i++) {
    Wire.beginTransmission(adress);
    Wire.write(COMMAND_REG_ENVELOPE + i);
    for (byte j = 0; j < 4; j++) {
      Wire.write(envelope[i] >> (j * 8));
    }
    Wire.endTransmission();
  }
}
void setChannelEnvelope(byte channel) {
  for (byte i = 0; i < voiceCount; i++) {
    if (assignedChannel[i] == channel) {
      setVoiEnvelope(i);
    }
  }
}

void getVoiEnvelope(byte adress) {
  Wire.beginTransmission(adress);
  Wire.write(COMMAND_REG_ENVELOPE);
  Wire.endTransmission();
  for (byte i = 0; i < envelopeCount; i++) {
    Wire.requestFrom(adress, 4);
    byte inputData [4];
    Wire.readBytes(inputData, 4);
    unsigned long inputLong = getUnsLongFromByte(inputData);
    envelope[i] = inputLong;
  }
}

void triggerVoiSave(byte adress) {
  Wire.beginTransmission(adress);
  Wire.write(COMMAND_DATA_SAVE);
  Wire.endTransmission();
}

void triggerChannelSave(byte channel) {
  for (byte i = 0; i < voiceCount; i++) {
    if (assignedChannel[i] == channel) {
      triggerVoiSave(i);
    }
  }
}

void triggerVoiLoad(byte adress) {
  Wire.beginTransmission(adress);
  Wire.write(COMMAND_DATA_LOAD);
  Wire.endTransmission();
}
void triggerChannelLoad(byte channel) {
  for (byte i = 0; i < voiceCount; i++) {
    if (assignedChannel[i] == channel) {
      triggerVoiLoad(i);
    }
  }
}

void pitchBend(byte channel, int bend) {
  for (byte i = 0; i < voiceCount; i++) {
    if (assignedChannel[i] == channel) {
      Wire.beginTransmission(i);
      Wire.write(COMMAND_PITCH_BEND);
      for (byte j = 0; j < 2; j++) {
        Wire.write(bend >> (j * 8));
      }
      Wire.endTransmission();
    }
  }
}

//Wave generators
void generateSinWave() {
  for (byte i = 0; i < sampleCount; i++) {
    float dataPos = (float) i / (float) sampleCount * 3.14 * 2;
    samples[i] = (byte)((sin(dataPos) + 1) * 255.0 / 2.0);
  }
}

void generateSquareWave(float dutyCycle) {
  for (byte i = 0; i < sampleCount; i++) {
    if ( i < ((float)sampleCount * dutyCycle)) {
      samples[i] = 0xFF;
    } else {
      samples[i] = 0x00;
    }
  }
}

void generateSawtoothWave(bool positive) {
  if (positive) {
    for (byte i = 0; i < sampleCount; i++) {
      samples[i] = map(i, 0, sampleCount, 0 , 0xFF);
    }
  } else {
    for (byte i = 0; i < sampleCount; i++) {
      samples[i] = map(i, 0, sampleCount, 0xFF , 0);
    }
  }
}

void generateTriangleWave() {
  for (byte i = 0; i < sampleCount; i++) {
    if (i < sampleCount / 2) {
      samples[i] = map(i, 0, sampleCount / 2, 0 , 0xFF);
    } else {
      samples[i] = map(i, sampleCount / 2, sampleCount, 0xFF , 0);
    }
  }
}

void generateNoiseWave() {
  for (byte i = 0; i < sampleCount; i++) {
    samples[i] = random(0, 0xFF);
  }
}

//Tools

void loadChAss() {
  for (byte i = 0; i < voiceCount; i++) {
    playingNote[i] = 0xFF;
    EEPROM.get(i * sizeof(byte), assignedChannel[i]);
  }
}

void saveChAss() {
  for (byte i = 0; i < voiceCount; i++) {
    EEPROM.put(i * sizeof(byte), assignedChannel[i]);
  }
}

void changeScreen(Screen screen) {
  currentScreen = screen;
  recentScreenSwitch = true;
  handleEncoderChange(0, 0);
}

byte getFirstVoice(byte channel, bool silent) {
  for (byte i = 0; i < voiceCount; i++) {
    if (silent) {
      if (assignedChannel[i] == channel && playingNote[i] == 0xFF) {
        return i;
      }
    } else {
      if (assignedChannel[i] == channel) {
        return i;
      }
    }
  }
  //No voice found, direct to non-existing voice
  return 0xFF;
}

byte getPlayingVoice(byte channel, byte note) {
  for (byte i = 0; i < voiceCount; i++) {
    if (assignedChannel[i] == channel && playingNote[i] == note) {
      return i;
    }
  }
  //No voice found, direct to non-existing voice
  return 0xFF;
}

InPos getButton(AceButton * button) {
  int pin = button->getPin();
  if (pin == ROT_ENC_PINS[LEFT][2]) return LEFT;
  else if (pin == ROT_ENC_PINS[RIGHT][2]) return RIGHT;
  else return NULL;
}

int wrapInt(int value, int minimum, int maximum) {
  //Minimum is inclusive maximum is exclusive
  int range = maximum - minimum;
  while (value >= maximum) {
    value -= range;
  }
  while (value < minimum) {
    value += range;
  }
  return value;
}

unsigned long getUnsLongFromByte(byte in[4]) {
  unsigned long out;
  for (byte i = 0; i < 4; i++) {
    out += (((unsigned long)in[i]) << (i * 8));
  }
  return out;
}
