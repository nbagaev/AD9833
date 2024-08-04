#define CS  9 //MCP41010 CS PIN
#define FSY 10 //AD9833 FSYNC PIN
#define DAT 11 //AD9833 SDATA PIN
#define CLK 13 //AD9833 SCLK PIN

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
#define WRITE_FREQ0 0x4000 //writing to FREQ0 register
#define WRITE_FREQ1 0x8000 //writing to FREQ1 register

//Phase Register Bits
#define WRITE_PHASE0 0xC000 //writing to PHASE0 register
#define WRITE_PHASE1 0xE000 //writing to PHASE1 register
/////////////AD9833 

unsigned long freq = 400;
uint16_t freq_MSB, freq_LSB;
uint16_t form = SINUS;

uint8_t pot_value;

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
          freq = atof (buf);
          if((freq >= 0.0) && (freq < 5000000)) 
          {
            form = SINUS;
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
          freq = atof (buf);
          if((freq >= 0.0) && (freq <= 5000000))
          {
            form = TRIANGLE;
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
          freq = atof (buf);
          if((freq >= 0.0) && (freq <= 5000000))
          {
            form=SQUARE;      
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
          pot_value = atof (buf);
          update_MCP41010(WRITE_DATA | P0 | pot_value);
          Serial.print("Potentiometer ");
          Serial.println(pot_value);          
        }
      }   
    }
  }
}

void prepare_freq()
{
  long freq_reg;
  freq_reg = (freq * 268435456.0) / MCLK; //2^28 = 268435456 
  freq_MSB = (uint16_t)((freq_reg) >> 14);//frequency registers are 28 bits wide, get 14 MSB
  freq_LSB = (uint16_t)(freq_reg & 0x3FFF);//get 14 LSB; 3FFF = 0011 1111 1111 1111
  freq_LSB |= WRITE_FREQ0;
  freq_MSB |= WRITE_FREQ0;
}

void update_freq()
{
  update_AD9833(B28 | RESET);
  prepare_freq();
  update_AD9833(freq_LSB); 
  update_AD9833(freq_MSB); 
  update_AD9833(form);   
}

void init_AD9833()
{
  update_AD9833(B28 | RESET);
  prepare_freq();
  update_AD9833(freq_LSB);
  update_AD9833(freq_MSB);
  update_AD9833(WRITE_PHASE0); 
  update_AD9833(EXIT_RESET | form); 
}

void update_AD9833(uint16_t data)
{
  digitalWrite(FSY, 0);
  write_word(data);
  digitalWrite(FSY, 1);
}

void update_MCP41010(uint16_t data)
{
  digitalWrite(CS, 0);
  write_word(data);
  digitalWrite(CS, 1);
}

void write_word(uint16_t data)
{
  for (int i = 15; i >= 0; i--)
  {
    digitalWrite(DAT, bitRead(data, i)); 
    digitalWrite(CLK, 0);
    digitalWrite(CLK, 1);
  }
}