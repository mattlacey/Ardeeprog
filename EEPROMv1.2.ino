
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

// basic scope loop
/*
 * 00000008  4240                                    clr.w   d0
 * 0000000A  103C 0006                               move.b  #6,d0
 * 0000000E  4E71                                    nop
 * 00000010  60F6                                    bra.s   $0008
 * 
 */

byte testROM [] =
{
  0x00, 0x00, 0x00, 0x00,   // SP
  0x00, 0x00, 0x00, 0x10,   // Initial PC
  0x42, 0x40,
  0x10, 0x3C, 0x00, 0x06,
  0x4E, 0x71,
  0x60, 0xF6
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
}

int incoming = 100;

void loop()
{
  if(incoming == 100)
  {
    printMenu();
    incoming = 99;  
  }
  
  if(Serial.available() > 0)
  {
    incoming = Serial.parseInt();

    if (Serial.read() == '\n')
    {
      if(incoming == 1)
      {
        Serial.println("ROM Length:");
        Serial.println(dataLength);
        Serial.println("===========");
        Serial.println("ROM to write");

        for (int i = 0; i < dataLength; i++)
        {
          if(i % 8 == 0)
            Serial.println("");

          if(!testROM[i])
            Serial.print("00");
          else
          {
            if(testROM[i] < 0x10)
              Serial.print("0");

              Serial.print(testROM[i], HEX);
          }

          Serial.print("  ");
        }
      }
      else if(incoming == 2 || incoming == 3)
      {
        Serial.print("Writing ROM in: ");
        timer(3);
        writeROM(incoming == 2);
      }
      else if(incoming == 4)
      {
        Serial.print("Reading ROM in: ");
        timer(3);
        readEEPROM();
      }
      else if(incoming == 9)
      {
        Serial.print("Disable protection in: ");
        timer(5);
        disableWriteProtect();
      }

      Serial.print("\n\n\n");
      incoming = 100;    
    }
  }
}

void printMenu()
{
  Serial.println("Choose an operation:\n--------------------\n1. Display ROM Image\n2. Write EEPROM (EVEN)\n3. Write EEPROM (ODD)\n4. Read EEPROM\n9. Disable Write Protect\n");
}

void timer(int count)
{
  for (int i = count; i; i--)
  {
    Serial.print(i);
    Serial.print(".");
    delay(333);
    Serial.print(".");
    delay(333);
    Serial.print(". ");
    delay(334);
  }

  Serial.println("");
}

void readEEPROM()
{
  if (writeMode)
  {
    setReadMode();
  }

  readROM();

  setAddress(0x0000);
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

void writeROM(bool even)
{
  for (int i = 0; i < dataLength / 2; i ++)
  {
    writeByteToAddress(testROM[(even ? i : i + 1)], i);
    /*
    Serial.print(i);
    Serial.print(": ");
    Serial.println(testROM[(even ? i * 2 : (i * 2) + 1)], HEX);
    */
    
    delay(10);
  }

  Serial.println("Write Complete\n");
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

