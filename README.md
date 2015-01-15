# daas
Arduino standing desk controller

Obviously I connected my motorised standing desk to an arduino. The circuit uses an ultrasonic ping sensor to detect height. Top and bottom heights can be set such that an up command takes the desk up no further than the top and a down command takes the desk no further down than the bottom. Free mode disables this feature.

Basic noise filtering is hacked in to prevent off readings stopping the desk at a weird height. I had something fancier in mind but this hack works reliably.

If you want the circuit diagram let me know.
