/*
   Sound Logic 2 Demo

   Uses Arduino Uno, AD5242 1 Meg Digital Pot, MT8816 crosspoint switches 
   Routes 555 audio through the switch matrix, connecting the pulses optionally 
   through NAND gates, a binary counter, and off board audio effects etc 
   and then to a final audio output.

   555 control voltage manipulation is also possible to help control the 555 pitch
   along with a 1 Meg digital pot to set the main frequency.

   16x16 Crosspoint Switch
 Uses two MT8816 8x16 switches, which are controlled over I2C
 by an MCP23017 GPIO expander and the Adafruit library

 Adafruit_MCP23017 library pin numbers to use for MCP23017 pins
 mcp23017 pin    pin name       library pin #
     21            GPA0             0
     22            GPA1             1
     23            GPA2             2
     24            GPA3             3
     25            GPA4             4
     26            GPA5             5
     27            GPA6             6
     28            GPA7             7
      1            GPB0             8
      2            GPB1             9
      3            GPB2             10
      4            GPB3             11
      5            GPB4             12
      6            GPB5             13
      7            GPB6             14
      8            GPB7             15


   Library used:  AD524x by Rob Tillaart
                  Adafruit MCP23017 
                
   Tested with Arduino IDE 2.3.4
               AD524x 0.5.1
               Adafruit MCP23017 2.3.2
               

   Gadget Reboot
   https://www.youtube.com/@gadgetreboot
*/

// AD5242 digital pot library
#include "AD524X.h"
AD524X AD01(0x2C);  //  address pins AD0 & AD1 = GND

// MCP23017 gpio expander library
#include <Adafruit_MCP23X17.h>
Adafruit_MCP23X17 mcp;  // mcp23017 object to communicate with
#define addr 0x20       // mcp23017 address when all address pins are grounded
#define closed 1        // crosspoint junction is connected when mt8816 data = 1
#define opened 0        // crosspoint junction is disconnected when mt8816 data = 0

// assign mcp23017 gpio pins to mt8816 pin functions based on schematic wiring
#define pinCS0 15  // chip select
#define pinCS1 0
#define pinReset 12
#define pinStrobe 14
#define pinData 13
#define pinAY0 5  // y address
#define pinAY1 6
#define pinAY2 7
#define pinAX0 1  // x address
#define pinAX1 2
#define pinAX2 3
#define pinAX3 4

void setup() {
  Serial.begin(115200);
  Serial.println();

  Wire.begin();
  Wire.setClock(400000);

  //i2c_scanner();  // check for I2C device addresses if needed

  // init digital pot
  AD01.begin();
  byte val = 1;
  AD01.write(1, val);  // set pot #1 wiper to val=1

  // init gpio expander
  if (!mcp.begin_I2C(addr)) {
    Serial.println("Can't initialize mcp23017");
    while (1)
      ;
  }

  // configure gpio expander output pins for mt8816 control
  mcp.pinMode(pinReset, OUTPUT);
  mcp.pinMode(pinStrobe, OUTPUT);
  mcp.pinMode(pinData, OUTPUT);
  mcp.pinMode(pinAY0, OUTPUT);
  mcp.pinMode(pinAY1, OUTPUT);
  mcp.pinMode(pinAY2, OUTPUT);
  mcp.pinMode(pinAX0, OUTPUT);
  mcp.pinMode(pinAX1, OUTPUT);
  mcp.pinMode(pinAX2, OUTPUT);
  mcp.pinMode(pinAX3, OUTPUT);
  mcp.pinMode(pinCS0, OUTPUT);
  mcp.pinMode(pinCS1, OUTPUT);

  // disable mt8816 chips (set chip selects low)
  mcp.digitalWrite(pinCS0, LOW);
  mcp.digitalWrite(pinCS1, LOW);

  resetMatrix();  // reset entire matrix (disconnect all node junctions)
}

void loop() {
  // configure matrix for various demo sounds
  demoSound1();  // pulsed tone with freq. sweeping up and down
  demoSound2();  // two tones alternating with freq. sweeping up and down
  demoSound3();  // same as demo 1 with 555 CV
  demoSound4();  // same as demo 2 with 555 CV
 // demoSound5();  // random 555 freq pot movements
}

// set up a pulsed tone (on/off cadence) by
// connecting two different binary counter Q outputs
// to a NAND gate.
// the gate output will be a tone that turns on and off
// at a rate based on the slower Q clock rate
void demoSound1() {
  resetMatrix();  // open all matrix junction contacts

  // define the new 16x16 matrix junction connection nodes
  setJunction(3, 10, closed);   // x3 (4040 clk in) connects to y10 (555 output pulses)
  setJunction(5, 0, closed);    // x5 (4040 Q2 output) connects to y0 (NAND1 input a)
  setJunction(6, 1, closed);    // x6 (4040 Q6 output) connects to y1 (NAND1 input b)
  setJunction(15, 8, closed);   // x15 (NAND1 output) connects to y8 (aux FX output to external effect input)
  setJunction(11, 15, closed);  // x11 (input from external effect) connects to y15 (main output to audio amp)

  potSweepUpDown();  // move the 555 pot wiper to max and back to min
}

// set up two alternating tones by
// mapping two Q outputs (two different tones)
// to separate NAND gates in such a way that
// one tone sounds, then the second, and it repeats
void demoSound2() {
  resetMatrix();  // open all matrix junction contacts

  // define the new 16x16 matrix junction connection nodes
  setJunction(3, 10, closed);  // x3 (4040 clk in) connects to y10 (555 output pulses)
  setJunction(5, 0, closed);   // x5 (4040 Q2 output) connects to y0 (NAND1 input a)   wire up the first NAND
  setJunction(6, 1, closed);   // x6 (4040 Q6 output) connects to y1 (NAND1 input b)
  setJunction(15, 6, closed);  // x15 (NAND1 output) connects to y6 (NAND4 input a)

  setJunction(4, 2, closed);   // x4 (4040 Q1 output) connects to y2 (NAND2 input a)   wire up NAND 2 and 3
  setJunction(6, 4, closed);   // x4 (4040 Q6 output) connects to y4 (NAND3 input a)
  setJunction(6, 5, closed);   // x4 (4040 Q6 output) connects to y5 (NAND3 input b)
  setJunction(13, 3, closed);  // x13 (NAND3 output) connects to y3 (NAND2 input b)

  setJunction(14, 7, closed);   // x14 (NAND2 output) connects to y7 (NAND4 input b)   finish connecting to NAND 4 and final output
  setJunction(12, 8, closed);   // x12 (NAND4 output) connects to y8 (aux FX output to external effect input)
  setJunction(11, 15, closed);  // x11 (input from external effect) connects to y15 (main output to audio amp)

  potSweepUpDown();  // move the 555 pot wiper to max and back to min
}

// same as demo 1 but with 555 control voltage effects
void demoSound3() {
  resetMatrix();  // open all matrix junction contacts

  // define the new 16x16 matrix junction connection nodes
  // connect X0 to X7 by using the unused Y14 line as an internal bus wire
  // this routes the slow freq. Q11 pulses to the 555 control voltage RC network
  // to modulate the 555 overall freq. based on the Q11 pulse rate
  setJunction(0, 14, closed);   // x0 connects to y14
  setJunction(7, 14, closed);   // x7 connects to y14
  setJunction(3, 10, closed);   // x3 (4040 clk in) connects to y10 (555 output pulses)
  setJunction(5, 0, closed);    // x5 (4040 Q2 output) connects to y0 (NAND1 input a)
  setJunction(6, 1, closed);    // x6 (4040 Q6 output) connects to y1 (NAND1 input b)
  setJunction(15, 8, closed);   // x15 (NAND1 output) connects to y8 (aux FX output to external effect input)
  setJunction(11, 15, closed);  // x11 (input from external effect) connects to y15 (main output to audio amp)

  potSweepUpDown();  // move the 555 pot wiper to max and back to min
}

// same as demo 2 but with 555 control voltage effects
void demoSound4() {
  resetMatrix();  // open all matrix junction contacts

  // define the new 16x16 matrix junction connection nodes
  // connect X0 to X7 by using the unused Y14 line as an internal bus wire
  // this routes the slow freq. Q11 pulses to the 555 control voltage RC network
  // to modulate the 555 overall freq. based on the Q11 pulse rate
  setJunction(0, 14, closed);  // x0 connects to y14
  setJunction(7, 14, closed);  // x7 connects to y14

  setJunction(3, 10, closed);  // x3 (4040 clk in) connects to y10 (555 output pulses)
  setJunction(5, 0, closed);   // x5 (4040 Q2 output) connects to y0 (NAND1 input a)   wire up the first NAND
  setJunction(6, 1, closed);   // x6 (4040 Q6 output) connects to y1 (NAND1 input b)
  setJunction(15, 6, closed);  // x15 (NAND1 output) connects to y6 (NAND4 input a)

  setJunction(4, 2, closed);   // x4 (4040 Q1 output) connects to y2 (NAND2 input a)   wire up NAND 2 and 3
  setJunction(6, 4, closed);   // x4 (4040 Q6 output) connects to y4 (NAND3 input a)
  setJunction(6, 5, closed);   // x4 (4040 Q6 output) connects to y5 (NAND3 input b)
  setJunction(13, 3, closed);  // x13 (NAND3 output) connects to y3 (NAND2 input b)

  setJunction(14, 7, closed);   // x14 (NAND2 output) connects to y7 (NAND4 input b)   finish connecting to NAND 4 and final output
  setJunction(12, 8, closed);   // x12 (NAND4 output) connects to y8 (aux FX output to external effect input)
  setJunction(11, 15, closed);  // x11 (input from external effect) connects to y15 (main output to audio amp)

  potSweepUpDown();  // move the 555 pot wiper to max and back to min
}

// move the pot wiper to random positions to create various tones
void demoSound5() {
  randomSeed(analogRead(0));
  for (int i = 0; i <= 25; i++) {
    int val = random(200);
    AD01.write(1, val);
    delay(50);
  }
}

void potSweepUpDown() {
  // increment pot wiper from end to end
  Serial.println("Moving pot wiper to adjust pitch");
  for (int val = 0; val <= 200; val++) {
    AD01.write(1, val);
    delay(20);
  }
  for (int val = 200; val >= 1; val--) {
    AD01.write(1, val);
    delay(20);
  }
}

// reset all junction connections
void resetMatrix() {
  mcp.digitalWrite(pinReset, HIGH);
  delay(100);
  mcp.digitalWrite(pinReset, LOW);
}

// based on mt8816 library https://github.com/DatanoiseTV/MT8816/
// modified to support two mt8816's arranged as a larger 16x16 matrix
// close or open the contacts at the x,y node of the 16x16 switch
void setJunction(uint8_t x, uint8_t y, bool state) {

  // the x addresses are not sequential as shown in the address
  // decoder table of the data sheet
  // this evaluation will map to the correct address for x.
  // eg. to access x position 12, the address is ax3..ax0  = 0110  = 6 decimal
  if (x == 12) x = 6;
  else if (x == 13) x = 7;
  else if ((x >= 6) && (x <= 11)) x += 2;

  // enable chip select of the mt8816 being accessed
  // based on which y line is being referenced
  if ((y >= 0) && (y <= 7))
    mcp.digitalWrite(pinCS0, HIGH);
  else
    mcp.digitalWrite(pinCS1, HIGH);

  // set the address pins corresponding to the x,y coordinates of the targeted junction
  if (bitRead(x, 0)) mcp.digitalWrite(pinAX0, HIGH);
  else mcp.digitalWrite(pinAX0, LOW);
  if (bitRead(x, 1)) mcp.digitalWrite(pinAX1, HIGH);
  else mcp.digitalWrite(pinAX1, LOW);
  if (bitRead(x, 2)) mcp.digitalWrite(pinAX2, HIGH);
  else mcp.digitalWrite(pinAX2, LOW);
  if (bitRead(x, 3)) mcp.digitalWrite(pinAX3, HIGH);
  else mcp.digitalWrite(pinAX3, LOW);
  if (bitRead(y, 0)) mcp.digitalWrite(pinAY0, HIGH);
  else mcp.digitalWrite(pinAY0, LOW);
  if (bitRead(y, 1)) mcp.digitalWrite(pinAY1, HIGH);
  else mcp.digitalWrite(pinAY1, LOW);
  if (bitRead(y, 2)) mcp.digitalWrite(pinAY2, HIGH);
  else mcp.digitalWrite(pinAY2, LOW);

  // strobe the junction control data into the mt8816 chip that is enabled by chip select
  mcp.digitalWrite(pinData, state);
  mcp.digitalWrite(pinStrobe, HIGH);
  delay(1);
  mcp.digitalWrite(pinStrobe, LOW);

  // disable chip selects
  mcp.digitalWrite(pinCS0, LOW);
  mcp.digitalWrite(pinCS1, LOW);
}

void i2c_scanner() {
  byte error, address;
  int nDevices;
  Serial.println("Scanning...");
  nDevices = 0;
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
      nDevices++;
    } else if (error == 4) {
      Serial.print("Unknow error at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  } else {
    Serial.println("done\n");
  }
  delay(500);
}
