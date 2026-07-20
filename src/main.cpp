#include <Arduino.h>
#include <SmartRC_CC1101.h>
#include <SPI.h>
#include <config.h>

#define CS D3
#define MOSI D10
#define SCK D8
#define MISO D9
#define GDO0 D2

SmartRC_CC1101 radio;

enum FanMode {
    FAN_TIMER_1H    = 0,
    FAN_TIMER_4H    = 1,
    FAN_SPEED_2     = 2,
    FAN_SPEED_3     = 3,
    FAN_SPEED_4     = 4,
    FAN_SPEED_5     = 5,
    FAN_SPEED_6     = 6,
    FAN_SPEED_7     = 7,
    FAN_DIR         = 9,
    LIGHT_POWER     = 10,
    LIGHT_BRIGHTEN  = 12,
    LIGHT_DIM       = 13,
    FAN_TIMER_8H    = 14,
    FAN_SPEED_MIXED = 15,
    FAN_POWER       = 17
};

struct Pulse
{
  bool level;
  uint16_t duration;
};

const Pulse ZERO[] =
{ 
  {HIGH, 387}, 
  {LOW, 801} 
};
const Pulse ONE[] = 
{ 
  {HIGH, 935}, 
  {LOW, 230} 
};
const Pulse END[] = 
{ 
  {HIGH, 347}, 
  {LOW, 4326} 
};

Pulse frame[66];

void buildFrame(FanMode fan_mode, uint16_t fan_id)
{ 
  uint8_t pos = 0;

  // command (fan mode dec repeated twice)
  uint16_t command = (fan_mode << 8) | fan_mode;

  for (int i = 15; i >= 0; --i)
  {
    bool bit = (command >> i) & 1;

    if (bit == LOW) {
      frame[pos++] = ZERO[0];
      frame[pos++] = ZERO[1];
    } else {
      frame[pos++] = ONE[0];
      frame[pos++] = ONE[1];
    }
  }

  // fan id
  for (int j = 15; j >= 0; --j)
  {
    bool bit = (fan_id >> j) & 1;

    if (bit == LOW) {
      frame[pos++] = ZERO[0];
      frame[pos++] = ZERO[1];
    } else {
      frame[pos++] = ONE[0];
      frame[pos++] = ONE[1];
    }
  }

  // gap between frames
  frame[pos++] = END[0];
  frame[pos++] = END[1];
}

void transmitPulse(Pulse p)
{
  digitalWrite(GDO0, p.level);
  delayMicroseconds(p.duration);
}

void sendFrame()
{
  for (unsigned int i = 0; i < 66; i++)
  {
    transmitPulse(frame[i]);
  }

  digitalWrite(GDO0, 0);
}

void setup()
{ 
  delay(2000);
  Serial.begin(115200);

  pinMode(GDO0, OUTPUT);
  digitalWrite(GDO0, 0);

  SPI.begin(SCK, MISO, MOSI, CS);

  radio.setSpiPin(SCK, MISO, MOSI, CS);
  radio.setGDO0(GDO0);

  radio.Init();

  radio.setCCMode(0);
  radio.setModulation(2);
  radio.setMHZ(433.92);

  radio.SpiWriteReg(CC1101_PKTCTRL0, 0x32);
  radio.SpiWriteReg(CC1101_IOCFG0, 0x0D);
  radio.SpiWriteReg(CC1101_PKTCTRL1, 0x00);

  radio.SpiWriteReg(CC1101_MDMCFG4, 0xF8);
  radio.SpiWriteReg(CC1101_MDMCFG3, 0x83);

  radio.SetTx();

  pinMode(GDO0, OUTPUT);

  // buildFrame()

}

void loop()
{
  radio.SetTx();
  delayMicroseconds(100);

  // send 5 copies of the frame
  for(int i = 0; i < 5; i++)
  {
    sendFrame();
  }

  delay(10000);
}