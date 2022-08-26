#include <Wire.h>
#include <EEPROM.h>

const byte DEVICE_ID = 1;

//Data consts
const unsigned int timerCompareValues[] = {61124, 57736, 54465, 51439, 48543, 45829, 43252, 40815, 38520, 36363, 34316, 32403, 30580, 28867, 27247, 25706, 24271, 22903, 21625, 20407, 19259, 18181, 17158, 16196, 15290, 14429, 13619, 12856, 12135, 11454, 10810, 10203, 9631, 9090, 8580, 8097, 7643, 7214, 6809, 6427, 6066, 5726, 5404, 5101, 4815, 4544, 4289, 4049, 3821, 3607, 3404, 3213, 3033, 2863, 2702, 2550, 2407, 2272, 2144, 2024, 1910, 1803, 1702, 1606, 1516, 1431, 1350, 1275, 1203, 1135, 1072, 1011, 955, 901, 850, 803, 757, 715, 675, 637, 601, 567, 535, 505, 477, 450, 425, 401, 378, 357, 337, 318, 300, 283, 267, 252, 238, 224, 212, 200, 189, 178, 168, 158, 149, 141, 133, 126, 118, 112, 105, 99, 94, 88, 83, 79, 74, 70, 66, 62, 59, 55, 52, 49, 46, 44, 41, 39};
const byte sampleCount = 32;
const byte envelopeCount = 5;

//Const consts
const byte COMMAND_NOTE_ON = 0x01;
const byte COMMAND_NOTE_OFF = 0x02;
const byte COMMAND_DATA_SAVE = 0x03;
const byte COMMAND_DATA_LOAD = 0x04;
const byte COMMAND_PITCH_BEND = 0x05;
const byte COMMAND_REG_SAMPLE_1 = 0x10;
const byte COMMAND_REG_SAMPLE_2 = 0x20;
const byte COMMAND_REG_ENVELOPE = 0x30;

enum Envelope {ATTACK, DECAY, SUSTAIN, RELEASE, PBSPAN};

const byte RW_SAMPLE = 0;
const byte RW_ENVELOPE = 1;

byte samples[sampleCount];
unsigned long envelope[envelopeCount];

byte rwReg = RW_SAMPLE;
byte rwPos = 0;
bool isPlaying = false;
unsigned long lastChangeTime;
byte samplePosition;
byte outputSample;
unsigned long volume = 0xff;
unsigned long envelopeModifier;
unsigned long lastEnvelopeModifier;
byte serialCounter = 0;
byte currentNote = 0xff;
float currentPitchBend = 0;

void setup() {
  //Serial.begin(115200);
  Wire.begin(DEVICE_ID);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

  DDRD = 0xFF;
  loadData();
  //saveDefaults();
}

void loop() {
  //TODO Clean up this mess
  unsigned long sample = (unsigned long) samples[samplePosition];
  if (isPlaying) {
    //(x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    if (millis() < (lastChangeTime + envelope[ATTACK])) {
      //During Attack, map from 0 to max
      unsigned long timeSinceLastChange = millis() - lastChangeTime;
      unsigned long maxTime = envelope[ATTACK];
      envelopeModifier = (255ul * timeSinceLastChange) / maxTime;
    } else if (millis() < (lastChangeTime + envelope[ATTACK] + envelope[DECAY])) {
      //During Decay, map from max to sustain
      unsigned long timeSinceLastEnv = millis() - lastChangeTime - envelope[ATTACK];
      unsigned long maxTime = envelope[DECAY];
      unsigned long invSustain = 255ul - envelope[SUSTAIN];
      envelopeModifier = 255ul - ((invSustain * timeSinceLastEnv) / maxTime);
    } else {
      //After decay while still playing, stay at sustain value
      envelopeModifier = envelope[SUSTAIN];
    }
    lastEnvelopeModifier = envelopeModifier;
  } else if (millis() < lastChangeTime + envelope[RELEASE]) {
    //During release, map from note end val to min
    unsigned long timeUntilEnd = envelope[RELEASE] - (millis() - lastChangeTime);
    unsigned long maxTime = envelope[RELEASE];
    envelopeModifier = (lastEnvelopeModifier * timeUntilEnd) / maxTime;
  } else {
    envelopeModifier = 0ul;
  }

  sample = (sample * volume * envelopeModifier) / 65025ul; // 255^2 = 65025
  unsigned long sampleOffset = (255ul - (volume * envelopeModifier / 255ul)) / 2ul;
  byte outByte = (byte) (sample + sampleOffset);
  PORTD = outByte;
}

void receiveEvent(int numBytes) {
  byte command = Wire.read();
  if (command < 0x10) {
    if (command == COMMAND_NOTE_ON) {
      byte readNote = Wire.read();
      byte readVolume = Wire.read();
      startNote(readNote, readVolume);
    } else if (command == COMMAND_NOTE_OFF) {
      stopNote();
    } else if (command == COMMAND_DATA_SAVE) {
      saveData();
    } else if (command == COMMAND_DATA_LOAD) {
      loadData();
    } else if (command == COMMAND_PITCH_BEND) {
      byte lsb = Wire.read();
      byte msb = Wire.read();
      int pitch = (msb << 8) | lsb;
      changePitch(pitch);
    }
  } else {
    byte cmod = command ^ COMMAND_REG_SAMPLE_1;
    if (cmod < 0x10) {
      rwReg = RW_SAMPLE;
      rwPos = cmod;
      while (Wire.available()) {
        samples[rwPos] = Wire.read();
        rwPos ++;
      }
    }
    cmod = command ^ COMMAND_REG_SAMPLE_2;
    if (cmod < 0x10) {
      rwReg = RW_SAMPLE;
      rwPos = 0x10 + cmod;
      while (Wire.available()) {
        samples[rwPos] = Wire.read();
        rwPos ++;
      }
    }
    cmod = command ^ COMMAND_REG_ENVELOPE;
    if (cmod < 0x10) {
      rwReg = RW_ENVELOPE;
      rwPos = cmod;
      while (Wire.available()) {
        byte inputData [4];
        Wire.readBytes(inputData, 4);
        unsigned long inputLong = getUnsLongFromByte(inputData);
        envelope[rwPos] = inputLong;
        rwPos ++;
      }
    }
  }
}

void requestEvent() {
  if (rwReg == RW_SAMPLE) {
    Wire.write(samples[rwPos]);
    rwPos++;
  } else if (rwReg == RW_ENVELOPE) {
    for (byte j = 0; j < 4; j++) {
      Wire.write(envelope[rwPos] >> (j * 8));
    }
    rwPos++;
  }
}

unsigned long getUnsLongFromByte(byte in[4]) {
  unsigned long out;
  for (byte i = 0; i < 4; i++) {
    out += (((unsigned long)in[i]) << (i * 8));
  }
  return out;
}

void startNote(byte noteIndex, byte byteVolume) {
  setInterruptTimer(getInterruptLength(noteIndex, currentPitchBend));
  if (noteIndex != currentNote || ! isPlaying) {
    lastChangeTime = millis();
  }
  volume = (unsigned long) byteVolume;
  currentNote = noteIndex;
  isPlaying = true;
}

void stopNote() {
  isPlaying = false;
  lastChangeTime = millis();
}

void changePitch(int intPitchBend) {
  currentPitchBend = float(intPitchBend) / 8191.0; // From sc of the MIDI library. Sent pitch ranges from -8192 to 8191
  setInterruptTimer(getInterruptLength(currentNote, currentPitchBend));
}

unsigned int getInterruptLength(byte note, float pitchBend) {
  pitchBend = pitchBend * float(envelope[PBSPAN]) / 12.0; 
  pitchBend = pow(2.0, pitchBend); //Actually pitch bend factor
  pitchBend = float(timerCompareValues[note]) / pitchBend; //Now actually interrupt length
  return (unsigned int) pitchBend; 
  
}

void loadData() {
  for (byte i = 0; i < sampleCount; i++) {
    EEPROM.get(i * sizeof(byte), samples[i]);
  }
  int dataOffset = sampleCount * sizeof(byte);
  for (byte i = 0; i < envelopeCount; i++) {
    EEPROM.get(dataOffset + (i * sizeof(long)), envelope[i]);
  }
}

void saveData() {
  for (byte i = 0; i < sampleCount; i++) {
    EEPROM.put(i * sizeof(byte), samples[i]);
  }
  int dataOffset = sampleCount * sizeof(byte);
  for (byte i = 0; i < envelopeCount; i++) {
    EEPROM.put(dataOffset + (i * sizeof(long)), envelope[i]);
  }
}

void saveDefaults() {
  for (byte i = 0; i < sampleCount; i++) {
    samples[i] = 255*i/sampleCount;
  }
  envelope[ATTACK] = 500;
  envelope[DECAY] = 500;
  envelope[SUSTAIN] = 200;
  envelope[RELEASE] = 500;
  envelope[PBSPAN] = 8;
  saveData();
}

void setInterruptTimer(unsigned int compareValue) {
  samplePosition = 0;
  cli();
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  OCR1A = compareValue;
  TCCR1B |= (1 << WGM12); // turn on CTC mode
  TCCR1B |= (1 << CS10); //No prescaler
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei();
}

SIGNAL(TIMER1_COMPA_vect) {
  samplePosition++;
  if (samplePosition >= sampleCount) {
    samplePosition = 0;
  }
}
