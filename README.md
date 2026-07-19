# Fanco Flow DC Ceiling Fan

Using CC1101 receiver recording raw pulse durations of fan remote, sequences can be decoded as they are fixed. The only security feature is a 16 bit id.

- Period for 1 bit is about 1200 microseconds.
- Zero is high short low long
- One is high long low short

```
FAN_POWER       : 0001 0001
FAN_SPEED_1     : 0000 0010
FAN_SPEED_2     : 0000 0011
FAN_SPEED_3     : 0000 0100
FAN_SPEED_4     : 0000 0101
FAN_SPEED_5     : 0000 0110
FAN_SPEED_6     : 0000 0111
FAN_SPEED_MIXED : 0000 1111
FAN_TIMER_1H    : 0000 0000
FAN_TIMER_4H    : 0000 0001
FAN_DIR         : 0000 1001
FAN_TIMER_8H    : 0000 1110
LIGHT_BRIGHTEN  : 0000 1100
LIGHT_POWER     : 0000 1010
LIGHT_DIM       : 0000 1101
```

Format of a payload:
- 16 bit command
  - single byte repeated twice
- 16 bit fan identification