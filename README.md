**A cycle accurate implementation for the PiDP8/I**
There has been quite a bit of discussion in the PiDP8/I group in relation to a cycle accurate PDP8 simulator.
https://groups.google.com/g/pidp-8/search?q=cycle%20accurate
As we are all aware, there is a fundamental problem with SIMH as it is inherently an assynchronous process.
This is compunded by the fact the Linux itself is not a realtime environment either.
This project is based on an RP2350 board from Waveshare: https://www.waveshare.com/wiki/RP2350-PiZero
This board has been designed to accept Raspberry Pi 'hats' so, whay not try this with the ultimate hat
namely the most excellent PiDP8/I devloped by Oscar Vermeulen  and supported by many other contributing
to the repo at https://tangentsoft.com/pidp8i/wiki?name=Home.
Using the RP2350 which is an update of the well know RP2040 ... Pi Pico alllows for 2 synchronous
processes to provide a simulator loop and to run the fron panel display.
I would note that I have been tempted in the past to design and build a board like this but other
experiences with similar projects using an ESP32 have clearly show that this is no easy task.
I have howvwer found out that this was no easy programming task in that this board does present
some interesting problems:
The first difficulty was getting a relaible inerface to the SDCard but it turned out that all of the
standard Arduino libraries do not work. So have has to unclude (a lot of) example code to drive the card
from the Waveshare site. All of these files are in the /src directory.
The second problem was that the Pi inteface GPIO pins are differnet to a standard Rasberry Pi.
This required a lot of bit twidling to get everthing lined up for the front panel display and the switches.
Then it became apparent that the SPI interface to the SDCard was disrupedt by the GPIO activity
for the front panel such that a semaphore sharing system was required.
The next interesting wrinkle was a requirment to present the SDCard contents as a USB drive so that
files my be added or deleted without having to remove the SDCard from the board. However as the disk images
for the PDP8 are on the SDCard, this required some functionality to ensure that the SDCard contents were
consistent. We all know the risks of shared files when you don't have an OS to look after this function.
Despite this, the system as it stands seem reasonably stable.
Practically speaking, the only thing to look out for is to use a 4GB SDCard as this has a 512 byte blocksize.
It may be that larger cards may work, I haven't done any tests in this regard.
To get going, open the prject in the Arduino IDE, select a Waveshare RP2350 Plus as the board, set the
USB stack to 'Adafruit TinyUSB' and the clock rate to 200 MHz. This clock setting results in a
simulator loop time of abou 1.5 uS ... just a little slower that the real thing.
dig out your priceless PiDP8/I and 
