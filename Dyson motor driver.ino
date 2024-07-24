//a single phase motor only has 2 methods of control: amplitude and phase
//here we keep amplitude at max (since motor already switches drive directions) at up to 16khz, even hardware pwm module reaches only 62khz
//then I tuned the phase shift until max speed and efficiency is achieved
//speed control could be achieved by varying the input voltage with a variable buck OR very high frequency PWM minimum 160khz depending on motor coil inductance (unachievable by atmega328p and most hobby H bridge modules)
//I blanket switch the whole of PORTC since i have extra unused pins to save cycles
//INT0 used on pin2 for hall sensor edges
//timer0 used for micros() system clock
//timer1 used to switch in advance (how much the motor actually follows along doesnt matter)
//serial debugging only works at low frequencies <1khz
//simplified spool up operation:
//step 1: poke motor a few times by alternating polarity until it turns over and the hall sensor catches
//step 2: once it starts rotating, switch pulse exactly when the hall sensor interrupts, spins up to around 40000rpm at 12v
//step 3: to speed it up, phase shift the drive waveform a little bit forward
//this is accomplished by firing timer1 a little earlier than the next supposed edge

#define HALL_EFFECT 2
#define PIN2_MASK (1 << PD2)//saves the MCU from doing a bitshift LOL
#define LBRIDGE A2 
#define RBRIDGE A3 

volatile uint8_t phase_advance = 12; //16  for full length[NOT RECOMMENDED, glitchy] minimum 12(75% earlier), 2 for 12.5% //period*16 000 000Hz/1000 000uS*phase_advance_%/100%;
volatile uint32_t abstickin;//absolute time of last input pulse
volatile uint32_t durationin = 1000000;//duration of last complete input pulse
volatile bool steadystate = false;//motor spinning steadily at transition phase
volatile bool stopped = true;//motor at rest
volatile uint32_t starttime = 0;

void(* resetFunc) (void) = 0;//wacky software reset function

void brake(){//for debugging only
  cli();
  digitalWrite(LBRIDGE, LOW);
  digitalWrite(RBRIDGE, LOW);
  while(1){}
}

void cold_start(){         // polarity order determined by trial/error/doesntreally matter
  for ( int i = 1000; i <= 4000; i += 100){//sweep through frequency range to get motor to oscillate until it turns over
    if (stopped){
      PORTC = 0b00001010;      //reverse();
      delayMicroseconds(i);
      PORTC = 0b00000101;      //forward();
      delayMicroseconds(i);
    }
  }
}

void setoneshot(){//  
  cli();
  OCR1A = durationin*phase_advance;//there is a natural 16x division from 16MHz system clock to microseconds, so multiply by fractions of 1-15/16 for how much earlier to switch aka phase shift
  TCNT1 = 0;//reset counter to 0
  TCCR1B = 0b00001001;//start timer
  sei();
}

ISR(INT0_vect) { 
  uint32_t timenow = micros();
  durationin = timenow - abstickin;//duration between transitions
  abstickin = timenow;

  if (timenow - starttime > 10000UL){//ignore the first tick, for some unknown reason my nano fires off an interrupt on startup
    stopped = false;
  }
  
  if (timenow - starttime < 150000UL){// 150ms, it is in spin up mode, toggle drive dir directly,
    PORTC = (PIND & PIN2_MASK) ? 0b00000101 : 0b00001010;
  }
  else if (!steadystate){ // transition (switch polarity then set timer to switch polarity again earlier than the next expected interrupt
    PORTC = (PIND & PIN2_MASK) ? 0b00000101 : 0b00001010;
    steadystate = true;
    setoneshot();
  }
  else{                    //switch polarity earlier than next expected interrupt
    setoneshot();
  }
}

ISR(TIMER1_COMPA_vect) {
  TCCR1B = 0b00001000;     // Stop the timer by clearing the prescaler bits
  PORTC = (PIND & PIN2_MASK) ? 0b00001010 : 0b00000101;//trying to switch earlier, always invert the sensor state rather than blindly toggling in case of timer upsets, here i simply inverted the ternary statement
}

void setup() {
  cli();

  //set DDR for pin2, vcc and gnd
  pinMode(HALL_EFFECT, INPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  digitalWrite(3, LOW);//virtual gnd
  digitalWrite(4, HIGH);//virtual vcc
  
  //set ddr for hbridge pins
  pinMode(A0, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(13, OUTPUT);
  
  // Enable INT0 interrupt (pin2)
  EICRA = 0b00000001;
  EIMSK = 0b00000001;
  
  //setup timer1 for oneshots
  TCCR1A = 0;
  TCCR1B = 0b00001000; //01 ctc, 001 prescaler 
  TCCR1C = 0;
  TCNT1 = 0;
  TIMSK1 |= (1 << OCIE1A); // Enable Timer1 Compare Match A interrupt
  OCR1A = 0;
  
  delay(500);//power supply rise time/time for power suppply to settle down
  starttime = micros();//motor starts here after the delay
  sei();
  cold_start();
  delay(200);
  
  if ((durationin > 1400) || (durationin < 200)){//reset if motor failed to start
    resetFunc();
  }
  
  phase_advance = 10;
  delay(100);
  phase_advance = 8;
  delay(100);
  phase_advance = 7;
  delay(200);
  phase_advance = 6;//max at 12v input
}

void loop() {//blinken status LEDs to show that the code has reached the main loop
  PORTB |= (1 << PORTB5);
  delay(500);
  PORTB &= ~(1 << PORTB5);
  delay(500);
}
