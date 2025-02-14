//
//  PolyP II  10.7.3
//
// SEND TARGETED SERIAL PACKET 11520 baud 
// 
// outputs individually controllable sine waves on:
//
// pins: 9, 10, 11 and 3
//
// 
// RANKS [1, 2, 3, 4] TO START
//
// ADD NOTE ON/OFF
//
//  
// <---------------------   FLOAT FREQUENCY MULTIPLY
//
// unsigned int hzToPhaseStep(float hz)                       {
//  float phaseStep= hz *2.0886902978652881446683197032183;           //  (pow(2,16) * frequency) / 31376.6
//  return (unsigned int)phaseStep;
//                                                            }
//
//




// --------------------------------------------------------------------------------------------------------- DECLARES ----------------

#include "m5angle8.h"

M5ANGLE8 MM;                                // M5ANGLE init

byte temp;                                  // Globals
byte n;
byte amplEnv;
byte note;

byte farfTuning = 1;                                                                   // <--------- for later sloppiness

bool busyFlag = false;
bool genBusyFlag;


bool AR =  false;                         // AR  = 0 = FALSE
bool ASR = true;                          // ASR = 1 = TRUE

int midFreq [] = {                                                                     // 60 notes
                                                                                       // midi 33..92 ?   [0..59]
  
                  55 , 58 , 62 , 65 , 69 , 73 , 78 , 82 , 87 ,
                  92 , 98 , 104 , 110 , 117 , 123 , 131 , 139 , 147 , 156 , 165 ,
                  175 , 185 , 196 , 208 , 220 , 233 , 247 , 262 , 277 , 294 , 311 ,
                  330 , 349 , 370 , 392 , 415 , 440 , 466 , 494 , 523 , 554 , 587 ,
                  622 , 659 , 698 , 740 , 784 , 831 , 880 , 932 , 988 , 1047 , 1109 ,
                  1175 , 1245 , 1319 , 1397 , 1480 , 1568 , 1661
                  
                  };





// --------------------------------------------------------------------------------------------------------- TIMERS ------------------

char sine256[256]  __attribute__ ((aligned(256))) = {
0 , 3 , 6 , 9 , 12 , 15 , 18 , 21 , 24 , 27 , 30 , 33 , 36 , 39 , 42 , 45 , 
48 , 51 , 54 , 57 , 59 , 62 , 65 , 67 , 70 , 73 , 75 , 78 , 80 , 82 , 85 , 87 , 
89 , 91 , 94 , 96 , 98 , 100 , 102 , 103 , 105 , 107 , 108 , 110 , 112 , 113 , 114 , 116 , 
117 , 118 , 119 , 120 , 121 , 122 , 123 , 123 , 124 , 125 , 125 , 126 , 126 , 126 , 126 , 126 , 
127 , 126 , 126 , 126 , 126 , 126 , 125 , 125 , 124 , 123 , 123 , 122 , 121 , 120 , 119 , 118 , 
117 , 116 , 114 , 113 , 112 , 110 , 108 , 107 , 105 , 103 , 102 , 100 , 98 , 96 , 94 , 91 , 
89 , 87 , 85 , 82 , 80 , 78 , 75 , 73 , 70 , 67 , 65 , 62 , 59 , 57 , 54 , 51 , 
48 , 45 , 42 , 39 , 36 , 33 , 30 , 27 , 24 , 21 , 18 , 15 , 12 , 9 , 6 , 3 , 
0 , -3 , -6 , -9 , -12 , -15 , -18 , -21 , -24 , -27 , -30 , -33 , -36 , -39 , -42 , -45 , 
-48 , -51 , -54 , -57 , -59 , -62 , -65 , -67 , -70 , -73 , -75 , -78 , -80 , -82 , -85 , -87 , 
-89 , -91 , -94 , -96 , -98 , -100 , -102 , -103 , -105 , -107 , -108 , -110 , -112 , -113 , -114 , -116 , 
-117 , -118 , -119 , -120 , -121 , -122 , -123 , -123 , -124 , -125 , -125 , -126 , -126 , -126 , -126 , -126 , 
-127 , -126 , -126 , -126 , -126 , -126 , -125 , -125 , -124 , -123 , -123 , -122 , -121 , -120 , -119 , -118 , 
-117 , -116 , -114 , -113 , -112 , -110 , -108 , -107 , -105 , -103 , -102 , -100 , -98 , -96 , -94 , -91 , 
-89 , -87 , -85 , -82 , -80 , -78 , -75 , -73 , -70 , -67 , -65 , -62 , -59 , -57 , -54 , -51 , 
-48 , -45 , -42 , -39 , -36 , -33 , -30 , -27 , -24 , -21 , -18 , -15 , -12 , -9 , -6 , -3 
};

volatile char* curWave=sine256;

#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

// nominal frequency of 31372.549

void setupTimers()        {

  TCCR1B = (TCCR1B & 0b11100000) | 0b00001;
  TCCR2B = (TCCR2B & 0b11110000) | 0b0001;
  TCCR1A = (TCCR1A &0b00001100)| 0b10100001;
  TCCR2A = (TCCR2A &0b00001100)| 0b10100001;
  TIMSK2 |= (1<<TOIE2);
  TIMSK1 |= (1<<TOIE1);
  
                           }

                           
// ------------------------------------------------------------------------------------------------------- STRUCTURES -----------

struct OscillatorState            {

   unsigned int phaseStep;
   unsigned int phaseAccu;

   byte volume;
   
   byte attack;                                                   // constrain 0..127
   byte decay;

   byte flags;                                                    // bit 2 AR/ASR
                                                                  // bits 0,1 - envelope state machine
                                                                  //
                                                                  // 00 - IDLE
                                                                  // 01 - ATTACK
                                                                  // 02 - SUSTAIN
                                                                  // 00 - DECAY
                                                                  
                                  };
                                  
                                

struct OscillatorState oscillators[4];                             // 4 oscillators



// ----------------------------------------------------------------------------------------------- TIMER ROUTINES --------------


inline byte getByteLevel(int accumulator)             {
    char valOut=((unsigned int)(accumulator))>>7;
    valOut+=128;
    return (byte)valOut;
                                                      }

ISR(TIMER1_OVF_vect)                                                         {
  oscillators[0].phaseAccu+=oscillators[0].phaseStep;
  int valOut0=curWave[oscillators[0].phaseAccu>>8]*oscillators[0].volume; 
  oscillators[1].phaseAccu+=oscillators[1].phaseStep;
  int valOut1=curWave[oscillators[1].phaseAccu>>8]*oscillators[1].volume;
  OCR1A=getByteLevel(valOut0);
  OCR1B=getByteLevel(valOut1);
                                                                             }

ISR(TIMER2_OVF_vect)                                                           {
  oscillators[2].phaseAccu+=oscillators[2].phaseStep;
  int valOut2=curWave[oscillators[2].phaseAccu>>8]*oscillators[2].volume;  
  oscillators[3].phaseAccu+=oscillators[3].phaseStep;
  int valOut3=curWave[oscillators[3].phaseAccu>>8]*oscillators[3].volume;
  OCR2A=getByteLevel(valOut2); // write to pin 11
  OCR2B=getByteLevel(valOut3); // write pin 
                                                                               }

unsigned int hzToPhaseStep(float hz)                       {
  float phaseStep= hz *2.0886902978652881446683197032183;           //  (pow(2,16) * frequency) / 31376.6
  return (unsigned int)phaseStep;
                                                           }

// ------------------------------------------------------------------------------------------------------- ROUTINES --------------

// ------------------------------- BUSY
//
//
// Toggles pin 12 LED, busy pin on when busy
//
//
//

bool busyP()                                    {

busyFlag   =      !   ( bitRead(oscillators[0].flags, 0)  |  bitRead(oscillators[0].flags, 1)  |               // <------------------------- REWRITE !!!!!
                        bitRead(oscillators[1].flags, 0)  |  bitRead(oscillators[1].flags, 1)  |               //        bool busyP() {} 
                        bitRead(oscillators[2].flags, 0)  |  bitRead(oscillators[2].flags, 1)  |               //        uses genBusyP() {} 
                        bitRead(oscillators[3].flags, 0)  |  bitRead(oscillators[3].flags, 1)   );             

                        if (busyFlag) {digitalWrite(13, HIGH);}
                           else       {digitalWrite(13,  LOW);}

                                               }


// ------------------------------- GEN BUSY?  (returns bool PREDICATE)
//
//
//
//

bool genBusyP(byte gen)                        {

  genBusyFlag = (bitRead(oscillators[gen].flags, 0)  |  bitRead(oscillators[0].flags, 1));         // <-------------------------- !!!!!!!!

  return genBusyFlag;

                                               }





// ------------------------------- ENVELOPES
//
//
// Does all gen envelopes using flags, state machine, attack and decay parameters
//
//
//                                             

void doEnv() {
  
  for (n=0; n<4; n++)                        {          // each of four oscillators

     switch (oscillators[n].flags & 0x7)                                                      {

     case 0:                                            // AR - IDLE ----------------------------          
     
     break;


     case 1:                                            // AR - ATTACK --------------------------

       temp  = oscillators[n].volume;
       temp += oscillators[n].attack;                   // add attack val

       if (temp > 125)                        {         // check for ovf
         temp = 125;
         bitSet  (oscillators[n].flags, 0);
         bitSet  (oscillators[n].flags, 1);   }         // proceed to decay [11]
         oscillators[n].volume = temp;                  // new volume value to table
     break;


     case 2:                                            // AR - SUSTAIN (ILLEGAL, SKIP) -------      
     break;


     case 3:                                            // AR - DECAY --------------------------

       temp  = oscillators[n].volume;
       temp -= oscillators[n].decay;                    // subtract decay value
       if (temp > 125)                      {           // check for ovf
         temp = 0;
         bitClear(oscillators[n].flags, 0);
         bitClear(oscillators[n].flags, 1); }           // proceed to idle [00]
         oscillators[n].volume = temp;                  // new volume value to table
      break;


      case 4:                                            // ASR - IDLE  --------------------------
    
      break;

      case 5:                                            // ASR - ATTACK  ------------------------

          temp  = oscillators[n].volume;
          temp += oscillators[n].attack;                 // add attack val
          if (temp > 125)                       {        // check for ovf
           temp = 125;
           bitClear(oscillators[n].flags, 0);
           bitSet  (oscillators[n].flags, 1);  }         // proceed to sustain [10]
           oscillators[n].volume = temp;                 // new volume value to table
       break;


       case 6:                                            // ASR - SUSTAIN -------------------------

                                                          // wait for noteOff, decay

       break;


       case 7:                                            // ASR - DECAY ---------------------------

       temp  = oscillators[n].volume;
       temp -= oscillators[n].decay;                      // subtract decay value
       if (temp > 125)                      {             // check for ovf
         temp = 0;
         bitClear(oscillators[n].flags, 0);
         bitClear(oscillators[n].flags, 1); }             // proceed to idle [00]
         oscillators[n].volume = temp;                    // new volume value to table
       break;
                                                                                    
                                                   }      // end switch/case

                                                   }      // end n = [0..3] loop for generators

                                                   }       // end doEnv()



// ------------------------------- NOTE ON                                                               <-------------------------- !!!!!!!!
//                                                                                                       <-------------------------- !!!!!!!!
//
// Turns selected voice on, starts envelopes
//
// note is indexed in MIDI freq table, add FARF, set rank multiply, put frequency
//
//

void noteOn(byte noteTemp)                          {

//
// noteTemp is midi note table lookup 
//
// set flags to 01, attack
//
// 
// Huge midi lookup, FLOAT value, +/- FARF as %
//
//  
  
                                                    }




// ------------------------------- NOTE OFF
//
//
// Turns selected voice off by proceeding to envelope decay                                             <-------------------------- !!!!!!!!
//
// set flags to 11, to decay IF NOT IDLE                                                                <-------------------------- !!!!!!!!
//


void noteOff()                                     {
  

  
  
                                                   }
 



// ------------------------------- SET ENV
//
//
// Sets up envelopes for each gen, full routine, manual
//
//
// AR/ASR FLAG IN BIT POSITION 2 BOOLEAN arAsrFlag
//
//
// gen[0..3], att[31..1], dec[31..1], bool arAsrFlag false = AR
//
//

void setEnv(byte gen, byte att, byte dec, bool arAsrFlag)            {

  oscillators[gen].attack = att;
  oscillators[gen].decay  = dec;

  if (arAsrFlag)   { bitClear  (oscillators[gen].flags, 2); }
  else             { bitSet    (oscillators[gen].flags, 2); }


    
                                                                     }


// ------------------------------- SET AR/ASR
//
//
// Sets up AR / ASR for each gen
//
//
// AR/ASR FLAG IN BIT POSITION 2 BOOLEAN arAsrFlag
//
//
// envType (gen[0..3], bool arAsrFlag)
//
// AR  defined as false
// ASR defined as true 
//
//

void envType (byte gen, bool arAsrFlag)             {

  if (arAsrFlag)   { bitClear  (oscillators[gen].flags, 2); }                // AR  = 0 = false
  else             { bitSet    (oscillators[gen].flags, 2); }                // ASR = 1 = true
    
                                                     }


// ------------------------------- STACK TO ENVs
//
//
// Reads M5STACK [31..1], puts in oscillators[gen].attack / .decay, test routine <----------------------
//
//

void stack2Env()                      {
                                        
  oscillators[0].attack = map((MM.analogRead(7, 8)), 0, 255, 31 ,1);
  oscillators[0].decay  = map((MM.analogRead(6, 8)), 0, 255, 31 ,1);
  oscillators[1].attack = map((MM.analogRead(5, 8)), 0, 255, 31 ,1);
  oscillators[1].decay  = map((MM.analogRead(4, 8)), 0, 255, 31 ,1);
  oscillators[2].attack = map((MM.analogRead(3, 8)), 0, 255, 31 ,1);
  oscillators[2].decay  = map((MM.analogRead(2, 8)), 0, 255, 31 ,1);
  oscillators[3].attack = map((MM.analogRead(1, 8)), 0, 255, 31 ,1);
  oscillators[3].decay  = map((MM.analogRead(0, 8)), 0, 255, 31 ,1);

                                       }




// ------------------------------- TEST BLEAT
//
//

void testBleat()                           {

  oscillators[0].volume=127;                                        // initial settings, opening bleat
  oscillators[1].volume=127;
  oscillators[2].volume=127;
  oscillators[3].volume=127;
  
  oscillators[0].phaseStep=hzToPhaseStep(400);
  oscillators[1].phaseStep=hzToPhaseStep(430);
  oscillators[2].phaseStep=hzToPhaseStep(460);
  oscillators[3].phaseStep=hzToPhaseStep(490);                      // initial settings, two second tone
  
  setupTimers();                                                    // <------  MOVE TO SETUP?????            *******************************************

  delay(2000);

  oscillators[0].volume=0;                                          // end opening bleat wait a second
  oscillators[1].volume=0;
  oscillators[2].volume=0;
  oscillators[3].volume=0;

  delay(1000);
  
                                            }



// ------------------------------- PANIC
//
// silence, resets envs
//
//

void panic()                                {

  for (n=0; n<4; n++)                 {                    // each of four oscillators

   oscillators[n].volume=0;                                // silence
   bitClear(oscillators[n].flags,0);
   bitClear(oscillators[n].flags,1);                       // set env to idle
                                       }
               
                                            }


// --------------------------------------------------------------------------------------------------------- INIT ----------------

void setup()
                                                    {

  Serial.begin(115200);                                             //   <---------- TO SHOW ENV DIAGNOSTICS, RCV COMMS
  
  Wire.begin();
  MM.begin();                                                       // M5STACK init
  
  pinMode(3, OUTPUT);                                               // pin 3  = PWM  output
  pinMode(9, OUTPUT);                                               // pin 9  = PWM  output
  pinMode(10, OUTPUT);                                              // pin 10 = PWM  output
  pinMode(11, OUTPUT);                                              // pin 11 = PWM  output

  pinMode(12, OUTPUT);                                              // pin 12 = busy LED & pin out 

 
  testBleat();


                                                    }

                                                    

// --------------------------------------------------------------------------------------------------------- MAIN ----------------


void loop()                                         {

     doEnv();                                                      // envelope state machine
     delay(10);                                                    // envelope tick delay, now 40ms to 1.25 sec [31..1]

     busyP();                                                      // set busy pin, LED (bool predicate)

     stack2Env();                                                  // read M5STACK for envelope parameters during testing

     Serial.println(oscillators[0].attack);                        // test only, main loop running

//   if (Serial.available) {}                                      // put comms handler here

  
                                                   }




                           
