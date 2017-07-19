
#define portOfPin(P)\
  (((P)>=0&&(P)<8)?&PORTD:(((P)>7&&(P)<14)?&PORTB:&PORTC))
#define ddrOfPin(P)\
  (((P)>=0&&(P)<8)?&DDRD:(((P)>7&&(P)<14)?&DDRB:&DDRC))
#define pinOfPin(P)\
  (((P)>=0&&(P)<8)?&PIND:(((P)>7&&(P)<14)?&PINB:&PINC))
#define pinIndex(P)((uint8_t)(P>13?P-14:P&7))
#define pinMask(P)((uint8_t)(1<<pinIndex(P)))


#define pinAsInput(P) *(ddrOfPin(P))&=~pinMask(P)
#define pinAsInputPullUp(P) *(ddrOfPin(P))&=~pinMask(P);digitalHigh(P)
#define pinAsOutput(P) *(ddrOfPin(P))|=pinMask(P)
#define digitalLow(P) *(portOfPin(P))&=~pinMask(P)
#define digitalHigh(P) *(portOfPin(P))|=pinMask(P)
#define isHigh(P)((*(pinOfPin(P))& pinMask(P))>0)
#define isLow(P)((*(pinOfPin(P))& pinMask(P))==0)
#define digitalState(P)((uint8_t)isHigh(P))


// As per hardware board, v 1.0


// PIN definitions for the shift register controls
#define OE_N    2 // Output enable (active low - address chips always grounded)
#define STCP    3 // Storage clock (RCLK)
#define SHCP    4 // Shift clock  (SRCLK)
#define MR_N    5 // Master reset (active low)
#define S_DATA  6 // Serial data


// PIN definition for the EEPROM controls
#define EEPROM_CE_N    8 // Chip Enable
#define EEPROM_OE_N    9 // Output Enable
#define EEPROM_WE_N    10 // Write Enable

byte testROM [] =
{
  0x55, 0xaa, 0x55, 0xaa,
  0x42, 0x42, 0x42, 0x42,
  0x55, 0xaa, 0x55, 0xaa,
  0x42, 0x42, 0x42, 0x42
};

int dataLength = sizeof(testROM);
bool writeMode = false;

void setup()
{
  Serial.begin(9600);

  // Configure pins for the shift registers
  pinMode(MR_N, OUTPUT);
  digitalWrite(MR_N, HIGH);

  pinMode(SHCP, OUTPUT);
  digitalWrite(SHCP, LOW);

  pinMode(STCP, OUTPUT);
  digitalWrite(SHCP, LOW);

  pinMode(OE_N, OUTPUT);
  digitalWrite(OE_N, HIGH);

  pinMode(S_DATA, OUTPUT);


  pinMode(EEPROM_WE_N, OUTPUT);
  pinMode(EEPROM_CE_N, OUTPUT);
  pinMode(EEPROM_OE_N, OUTPUT);

  digitalWrite(EEPROM_OE_N, HIGH);

  Serial.println("ROM Length:");
  Serial.println(dataLength);
  Serial.println("===========");
  Serial.println("ROM to write");

  for (int i = 0; i < dataLength; i++)
  {
    if(i % 4 == 0)
      Serial.println("");

    Serial.print(testROM[i], HEX);
  }

/*
  Serial.println("===========");
  Serial.println("Writing...");

  //Serial.print("Disable protection in: ");
  //timer(5);
  //disableWriteProtect();
*/
  Serial.print("Writing ROM in: ");
  timer(3);
  writeROM();

  Serial.print("Staring read in: ");
  timer(3);

  setReadMode();
}

void loop()
{
  delay(200);
  readEEPROM();
  setAddress(0x0000);
}

void timer(int count)
{
  for (int i = count; i; i--)
  {
    Serial.print(i);
    Serial.print("... ");
    delay(1000);
    Serial.println("");
  }
}

void readEEPROM()
{
  if (writeMode)
  {
    setReadMode();
  }

  readROM();
}

void setReadMode()
{
  // turn off data shift register output
  // address chips are always enabled
  digitalWrite(OE_N, HIGH);

  // setup EEPROM for read, after this toggling CE will read address on shift registers
  digitalWrite(EEPROM_OE_N, LOW);
  digitalWrite(EEPROM_CE_N, LOW);
  digitalWrite(EEPROM_WE_N, HIGH);

  writeMode = false;
}

void setWriteMode()
{
  // turn off data output until we're ready to write
  digitalWrite(OE_N, HIGH);

  // turn off EEPROM output and prepare CE & WE for writing
  digitalWrite(EEPROM_OE_N, HIGH);
  digitalWrite(EEPROM_CE_N, HIGH);
  digitalWrite(EEPROM_WE_N, HIGH);
  
  writeMode = true;
}

// this doesn't actually read to anywhere, just sets
// the io lines on the EEPROM for inspection
byte readROM()
{
  Serial.println("readROM");
  for (int i = 0; i < dataLength; i++)
  {
    setAddress(i);
    delay(10);
  }
}

void writeROM()
{
  for (int i = 0; i < dataLength; i++)
  {
    writeByteToAddress(testROM[i], i);
    delay(10);
  }
}

void setAddress(short address)
{
  Serial.println(address);
  digitalLow(STCP);
  shiftOutFast(0);
  shiftOutFast(address >> 8);
  shiftOutFast(address & 0xff);
  digitalHigh(STCP);
}

void disableWriteProtect()
{
  writeByteToAddress(0xaa, 0x5555);
  writeByteToAddress(0x55, 0x2aaa);
  writeByteToAddress(0x80, 0x5555);
  writeByteToAddress(0xaa, 0x5555);
  writeByteToAddress(0x55, 0x2aaa);
  writeByteToAddress(0x20, 0x5555);
}

void writeByteToAddress(char byte, int address)
{
  digitalHigh(OE_N);
    
  digitalLow(STCP);
  shiftOutFast(byte);
  shiftOutFast(address >> 8);
  shiftOutFast(address & 0xff);
  digitalHigh(STCP);
  
  delayMicroseconds(10);
  
  digitalLow(EEPROM_WE_N);
  digitalLow(EEPROM_CE_N);

  delayMicroseconds(10);
  
  digitalLow(OE_N);

  delayMicroseconds(10);
    
  digitalHigh(EEPROM_CE_N);
  digitalHigh(EEPROM_WE_N);
}

void shiftOutFast(char byte)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (!!(byte & (1 << (7 - i))))
      digitalHigh(S_DATA);
    else
      digitalLow(S_DATA);

    digitalHigh(SHCP);
    digitalLow(SHCP);
  }
}

