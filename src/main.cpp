#include <Arduino.h>
#include <SmartRC_CC1101.h>
#include <SPI.h>

#define CS D3
#define MOSI D10
#define SCK D8
#define MISO D9
#define GDO0 D2

SmartRC_CC1101 radio;

struct Pulse
{
  bool level;
  uint16_t duration;
};

// command = fan setting
// fan id  = different per fan
// must be 16 bits ea
const uint16_t command = 0b0000001000000010;
const uint16_t fanId   = 0b0000000000000000;

const Pulse zero[] =
{ 
  {HIGH, 387}, 
  {LOW, 801} 
};
const Pulse one[] = 
{ 
  {HIGH, 935}, 
  {LOW, 230} 
};
const Pulse end[] = 
{ 
  {HIGH, 347}, 
  {LOW, 4326} 
};

Pulse frame[66];

void buildFrame(uint16_t commandBits, uint16_t idBits)
{ 
  uint16_t framePos = 0;

  // command
  for (int i = 15; i >= 0; --i)
  {
    bool bit = (commandBits >> i) & 1;

    if (bit == LOW) {
      frame[framePos++] = zero[0];
      frame[framePos++] = zero[1];
    } else {
      frame[framePos++] = one[0];
      frame[framePos++] = one[1];
    }
  }

  // fan id
  for (int j = 15; j >= 0; --j)
  {
    bool bit = (idBits >> j) & 1;

    if (bit == LOW) {
      frame[framePos++] = zero[0];
      frame[framePos++] = zero[1];
    } else {
      frame[framePos++] = one[0];
      frame[framePos++] = one[1];
    }
  }

  // gap between frames
  frame[framePos++] = end[0];
  frame[framePos++] = end[1];
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

  //---

  buildFrame(command, fanId);

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