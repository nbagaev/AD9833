#define CS  6 //MCP41010 CS PIN
#define FSY 7 //AD9833 FSYNC PIN
#define DAT 8 //AD9833 MCP41010 SDATA PIN
#define CLK 9 //AD9833 MCP41010 SCLK PIN

#define MCLK 25000000.0 //AD9833 25MHz master clock

/////////////MCP41010
//Data is clocked into the SI pin on the rising edge of the clock.
//Control Register Bits
#define SHUTDOWN 0x2000 //Shutdown
#define WRITE_DATA 0x1000 //Write Data
#define P0 0x100 //Potentiometer 0 selected.
/////////////MCP41010

/////////////AD9833 
//Data is clocked into the AD9833 on each falling edge of SCLK..
//Control Register Bits
#define B28         0x2000 // Two write operations are required to load a complete word into either of the frequency registers
#define HLB         0x1000 // This control bit allows the user to continuously load the MSBs or LSBs of a frequency register while ignoring the remaining 14 bits.
#define FSELECT     0x800  // The FSELECT bit defines whether the FREQ0 register or the FREQ1 register is used in the phase accumulator
#define PSELECT     0x400  // The PSELECT bit defines whether the PHASE0 register or the PHASE1 register data is added to the output of the phase accumulator.
#define RESET       0x100  // resets internal registers to 0
#define EXIT_RESET  0x0    // disable reset 
#define SLEEP1      0x80   // MCLK clock is disabled
#define SLEEP12     0x40   // powers down the on-chip DAC
#define OPBITEN     0x20   // the MSB (or MSB/2) of the DAC data is connected to the VOUT pin
#define DIV2        0x8    // used in association with OPBITEN. the MSB of the DAC data is passed directly to the VOUT pin
#define MODE        0x2    // used in association with OPBITEN. triangle output, otherwise sinus

#define SINUS       (B28 | 0x0)
#define TRIANGLE    (B28 | MODE)
#define SQUARE      (B28 | OPBITEN | DIV2)

//Frequency Register Bits
#define FREQ_REG_0 0x4000 //writing to FREQ0 register
#define FREQ_REG_1 0x8000 //writing to FREQ1 register

//Phase Register Bits
#define PHASE_REG_0 0xC000 //writing to PHASE0 register
#define PHASE_REG_1 0xE000 //writing to PHASE1 register
/////////////AD9833 

unsigned long freq = 400;
uint16_t form = SINUS;

void setup()
{
  pinMode(CS, OUTPUT);
  pinMode(FSY, OUTPUT);
  pinMode(DAT, OUTPUT);
  pinMode(CLK, OUTPUT);
  digitalWrite(FSY, 1);
  digitalWrite(CS, 1);
  digitalWrite(DAT, 0);  
  digitalWrite(CLK, 1);
 
  Serial.begin(115200);
  Serial.setTimeout(100);//faster reaction, default 1000 

  init_AD9833();

  Serial.println(" type s1..s5000000 for sinus 1..5000000 Hz");
  Serial.println(" type t1..t5000000 for triangle 1..5000000 Hz");
  Serial.println(" type r1..s5000000 for square 1..5000000 Hz");
  Serial.println(" type p0..p255 for potentiometer");
}

void loop()
{
  uint8_t ser, len;
  static char buf[10];
  
  if (Serial.available() > 0)
  {
    ser = Serial.read();
    memset(buf, '\0', sizeof(buf));
    len = Serial.readBytes(buf,12);
    switch (ser)
    {
      case 's': //sinus
      {        
        if((len>0) && (len<11))
        {                  
          freq = strtoul(buf, NULL, 10);
          if((freq >= 0.0) && (freq < 5000000)) 
          {
            form = SINUS;
            update_form();
            update_freq();         
            Serial.print("Sinus ");
            Serial.println(freq);            
          }
        }      
      }
      break;
      case 't': //triangle
      {       
        if((len>0) && (len<11))
        {
          freq = strtoul(buf, NULL, 10);
          if((freq >= 0.0) && (freq <= 5000000))
          {
            form = TRIANGLE;
            update_form();
            update_freq();            
            Serial.print("Triangle ");
            Serial.println(freq);
          }
        }
      }
      break;
      case 'r': //rectangle
      {      
        if((len>0) && (len<11))
        {               
          freq = strtoul(buf, NULL, 10);
          if((freq >= 0.0) && (freq <= 5000000))
          {
            form=SQUARE;
            update_form();      
            update_freq();
            Serial.print("Rectangle ");
            Serial.println(freq);            
          }
        }
      }
      break;
      case 'p': //potentiometer
      {      
        if((len>0) && (len<4))
        {                  
          uint8_t pot_value = atoi (buf);
          write_word(WRITE_DATA | P0 | pot_value, DAT, CLK, CS);//update MCP41010
          Serial.print("Potentiometer ");
          Serial.println(pot_value);          
        }
      }   
    }
  }
}

void write_word(uint16_t data, uint8_t sda, uint8_t sclk, uint8_t cs)
{
  digitalWrite(cs, 0);
  for (int i = 15; i >= 0; i--)
  {
    digitalWrite(sda, bitRead(data, i)); 
    digitalWrite(sclk, 0);
    digitalWrite(sclk, 1);
  }
  digitalWrite(cs, 1);
}

void update_form()
{
  write_word(B28 | form, DAT, CLK, FSY);  
}

void update_freq()
{
  long freq_reg = (freq * 268435456.0) / MCLK; //2^28 = 268435456 
  uint16_t freq_MSB = (uint16_t)((freq_reg) >> 14);//frequency registers are 28 bits wide, get 14 MSB
  uint16_t freq_LSB = (uint16_t)(freq_reg & 0x3FFF);//get 14 LSB; 3FFF = 0011 1111 1111 1111
  write_word(freq_LSB | FREQ_REG_0, DAT, CLK, FSY);
  write_word(freq_MSB | FREQ_REG_0, DAT, CLK, FSY);     
}

void init_AD9833()
{
  write_word(B28 | RESET | form, DAT, CLK, FSY);
  update_freq();
  write_word(PHASE_REG_0, DAT, CLK, FSY);
  write_word(B28 | EXIT_RESET, DAT, CLK, FSY);
}

