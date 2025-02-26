
// REV 11 KISS !!!
//
// 11.4.3
//
// SEND TARGETED SERIAL PACKET 9600 baud 
//
// LOCAL BUS MESSAGES
// 
// SERIAL PACKET MANAGER EDITION II
//
//
// 
//
//    'A' + four bytes amplitude [0..127]
//
//    'F' + eight bytes, loByte, hiByte freq
// 
//    'N' + 1 byte, set frequencies from 'midi' note table. Freq * Rank to voice, character [SPC..Z]
//
//    'R' + 4 bytes, rank array load, character [1..4]
//
//
//        LATER
//
//
//    'T' + 60 bytes, 'midi' note table load, for different scales
//
//    'H' + 4 bytes, char +/- 125/100 % Farf value?
//
//    'W" + 256 bytes, wavetable load
// 





// --------------------------------------------------------------------------------------------------------- DECLARES ----------------




byte temp, lob, hib, n, i;

int freqValue;

float midFreq [] = {                                                                 // 60 notes
                                                                                     // midi 33..92 ?   [0..59]
                                                                                     // A 440 = 36
  
55, 58.27047, 61.735413, 65.406391, 69.295658, 73.416192, 77.781746,
82.406889, 87.307058, 92.498606, 97.998859, 103.826174, 110, 116.54094,
123.470825, 130.812783, 138.591315, 146.832384, 155.563492, 164.813778,
174.614116, 184.997211, 195.997718, 207.65249, 220, 233.081881, 246.941651,
261.625565, 277.182631, 293.664768, 311.126984, 329.627557, 349.228231,
369.994423, 391.995436, 415.304698, 440, 466.163762 , 493.883301, 523.251131,
554.365262, 587.329536, 622.253967 ,659.255114, 698.456463, 739.988845,
783.990872, 830.609395, 880, 932.327523, 987.766603, 1046.502261, 1108.730524,
1174.659072, 1244.507935, 1318.510228, 1396.912926, 1479.977691, 1567.981744, 1661.21879
                  
                    };


byte rank [] = {1,2,3,4};

char farf [] = {0,0,0,0};




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

struct genState                 {
   unsigned int phaseStep;
   byte amplitude;                                                                    // <-------- BYTE AMPLITUDE !!!!!!!
   unsigned int phaseAccu;
                                };
                                

struct genState gen[4];                                                               // 4 generators


inline byte getByteLevel(int accumulator)             {
    char valOut=((unsigned int)(accumulator))>>7;
    valOut+=128;
    return (byte)valOut;
                                                      }

ISR(TIMER1_OVF_vect)                                                         {
  gen[0].phaseAccu += gen[0].phaseStep;
  int valOut0 = curWave[gen[0].phaseAccu>>8]*gen[0].amplitude; 
  gen[1].phaseAccu += gen[1].phaseStep;
  int valOut1 = curWave[gen[1].phaseAccu>>8]*gen[1].amplitude;
  OCR1A=getByteLevel(valOut0);
  OCR1B=getByteLevel(valOut1);
                                                                             }

ISR(TIMER2_OVF_vect)                                                          {
  gen[2].phaseAccu += gen[2].phaseStep;
  int valOut2 = curWave[gen[2].phaseAccu>>8]*gen[2].amplitude;  
  gen[3].phaseAccu += gen[3].phaseStep;
  int valOut3 = curWave[gen[3].phaseAccu>>8]*gen[3].amplitude;
  OCR2A=getByteLevel(valOut2);
  OCR2B=getByteLevel(valOut3);
                                                                              }


unsigned int hzToPhaseStep(float hz)                       {
  float phaseStep= hz *2.0886902978652881446683197032183;                                    //  (pow(2,16) * frequency) / 31376.6
  return (unsigned int)phaseStep;
                                                           }

// ----------------------------------------------------------------------------------------------------- ROUTINES ----------------


void messageSwitch ()                      {
  
  switch (temp)                       {

      case 65:                                                              // 65 = "A" for amplitude

       for(n = 0; n < 4; n++)    {                                          // wait for, and get 4 bytes
        
       while (Serial.available () == 0) {}
       gen[n].amplitude = Serial.read();
       
                                 }
       break;
      

      case 70:                                                              // 70 = "F" for frequency

       for(n = 0; n < 4; n++)    {
        
       while (Serial.available () == 0) {}                                  // wait for, and get 8 bytes
       lob = Serial.read();
       while (Serial.available () == 0) {}
       hib = Serial.read();
       freqValue = hib * 16 + lob;
       gen[n].phaseStep = hzToPhaseStep(freqValue);                         //  pitch in hz
       
                                 }
       break;
       

       case 78:                                                             // 78 = "N" for note table

       while (Serial.available () == 0) {}                                  // wait for, and get 1 byte notenum[0..59]
       temp = Serial.read();

       for(n = 0; n < 4; n++)    {
        
       gen[n].phaseStep = hzToPhaseStep(midFreq[temp - 32] * rank[n]);      //  pitch in hz
                                                                            //
                                                                            // NOTE: jiggered for printable characters
                                                                            // A 440 = "D"

                                 }


       case 82:                                                             // 82 = "R" for rank table

       for(n = 0; n < 4; n++)    {

       while (Serial.available () == 0) {}                                  // wait for, and get 4 bytes
       temp = Serial.read();
        
       rank[n] = temp - 48;                                                 //
                                                                            // NOTE: jiggered for printable characters
                                                                            // char [1..4], asc [49..52]
                                 }


       break;

      
      default:

      
      break;

                                      }
                                            }


// --------------------------------------------------------------------------------------------------------- INIT ----------------

void setup()
{
  
  pinMode(3, OUTPUT);                                               // pin 3  = PWM  output
  pinMode(9, OUTPUT);                                               // pin 9  = PWM  output
  pinMode(10, OUTPUT);                                              // pin 10 = PWM  output
  pinMode(11, OUTPUT);                                              // pin 11 = PWM  output

  pinMode(13, OUTPUT);                                              // pin 13 = LED out 

 
  Serial.begin(9600); 


  gen[0].amplitude=127;                                             // initial settings, opening bleat
  gen[1].amplitude=127;
  gen[2].amplitude=127;
  gen[3].amplitude=127;
  
  gen[0].phaseStep=hzToPhaseStep(400);
  gen[1].phaseStep=hzToPhaseStep(500);
  gen[2].phaseStep=hzToPhaseStep(600);
  gen[3].phaseStep=hzToPhaseStep(700);                             // initial settings, two second tone
  
  setupTimers();

  delay(2000);

  gen[0].amplitude=0;                                              // end opening bleat wait a second
  gen[1].amplitude=0;
  gen[2].amplitude=0;
  gen[3].amplitude=0;

  delay(1000);

}

// --------------------------------------------------------------------------------------------------------- MAIN ----------------


void loop()
                          {

  if (Serial.available() != 0)    {
    
    temp = Serial.read();
    messageSwitch ();
 
                                  }
                          }
