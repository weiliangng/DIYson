# DIYson
Firmware for driving single pole motors used in dyson vacuums with arduino

I had a dyson with a blown motherboard so instead of dealing with authentication rubbish i simply ripped out all the electronics and replaced them with parts i had lying around

Things you will need: 
1. Dyson vacuum running a single phase motor
2. Any high current H bridge, exact inputs doesnt matter as long as you can get current flowing in both directions
3. Any Atmega 168/328
4. Hall effect sensor salvaged from the old motherboard


A single phase motor only has 2 methods of control: amplitude and phase. Here, we keep amplitude at max, then I tuned the phase shift until max speed and efficiency is achieved. 
Speed control could be achieved by varying the input voltage with a variable buck OR very high frequency PWM minimum 160khz or rather depending on motor coil inductance (unachievable by atmega MCUs and most hobby H bridge modules)



Heres a few problems/design conflicts: 
1. hardswitching at 62khz causes alot of switching losses espescially with typical off the shelf h bridge with slow transistors designed for typical 20khz switching (just to get it out of hearing range)
2. I havent found a way to make the pwm peripheral flip directions immediately upon timer 1 interrupt, usually it waits until next overflow before the direction variable is loaded and acted on
3. So I ditched PWM to remove the delay before the PWM peripheral fires and simply fullsend the motor ie it is stuck at max speed
4. This would significantly reduce mosfet switching losses by 3x from 63khz to 15-20khz(theoretical max toggling frequency depending on input voltage/motor speed)
5. I found that at a certain phase shift (coincidentally where the motor is at max speed) the fets run alot cooler which leads me to assume some zero voltage/current switching is occuring
6. assuming 20khz input frequency (at 120,000 rpm) theres 800 clock cycles between each INT0 & TIM1 interrupt which really doesnt leave much room for interrupt handling (each interrupt takes around 10 uS = 160 clock cycles alone)

Taking reference from traction motor waveforms, I could implement a 1 step pwm by simply letting the current freewheel by setting COMPB vect to trigger and switch everything off/disable drivers earlier before COMPA switches the direction. 
Since official dyson vacuums only have discrete speed steps (HIGH MED LOW) at most, we can simply have a 2 speed setting (PWM_decimation to be tuned)
TODO: add rudimentary PWM by using timer1 compB to disable the drivers earlier to let it freewheel and tune how early to let the motor freewheel

//in setup,
//enable COMPB interrupt
//set digital pin to input_pullup


//in main loop, digitalread an additional pin and then set PWM_decimation according to HIGH/LOW/med

//in setoneshot, set compB trigger point not too small but not too big either
OCR1B = durationin*PWM;//where 2 < PWM_decimation < phase_advance

//in compb_vect, disable/ensure all drivers are in the same state ie freewheeling/open motor
PORTC = 0;//everything off
//DO NOT DISABLE TIMER






WARNING: I blanket switch the whole of PORTC to save cycles since i have extra pins and those arent used.


step 1: poke motor a few times by alternating polarity until the hall sensor catches
step 2: once it starts rotating, switch pulse exactly when the hall sensor interrupts, spins to around 40000rpm at 12v
step 3: to speed it up, phase shift the drive waveform a little bit forward, this is accomplished by settomg timer1 to fire a little earlier than the previous edge


Spot the cringe: reset function and lazy virtual ground/supplies
