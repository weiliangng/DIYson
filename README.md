# DIYson
Firmware for driving single pole motors used in dyson vacuums with arduino

I had a dyson with a blown motherboard so instead of dealing with authentication rubbish i simply ripped out all the electronics and replaced them with parts i had lying around

Things you will need: 
1. Dyson vacuum running a single phase motor
2. Any high current H bridge, exact inputs doesnt matter as long as you can get current flowing in both directions
3. Any Atmega 168/328
4. Hall effect sensor salvaged from the old motherboard


A single phase motor only has 2 methods of control: amplitude and phase. Here, we keep amplitude at max (since motor already switches drive directions) at up to 16khz, even Arduino's hardware pwm peripheral reaches only 62khz
Then, I tuned the phase shift until max speed and efficiency is achieved. Speed control could be achieved by varying the input voltage with a variable buck OR very high frequency PWM minimum 160khz or rather depending on motor coil inductance (unachievable by atmega MCUs and most hobby H bridge modules)


WARNING: I blanket switch the whole of PORTC to save cycles since i have extra pins and those arent used.


step 1: poke motor a few times by alternating polarity until the hall sensor catches
step 2: once it starts rotating, switch pulse exactly when the hall sensor interrupts, spins to around 40000rpm at 12v
step 3: to speed it up, phase shift the drive waveform a little bit forward, this is accomplished by settomg timer1 to fire a little earlier than the previous edge


