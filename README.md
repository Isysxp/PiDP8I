**A cycle accurate implementation for the PiDP8/I**
There has been quite a bit of discussion in the PiDP8/I group in relation to a cycle accurate PDP8 simulator.
https://groups.google.com/g/pidp-8/search?q=cycle%20accurate
As we are all aware, there is a fundamental problem with SIMH as it is inherently an asynchronous process.
This is compounded by the fact the Linux itself is not a mealtime environment either.
This project is based on an RP2350 board from Waveshare: https://www.waveshare.com/wiki/RP2350-PiZero
This board has been designed to accept Raspberry Pi 'hats' so, why not try this with the ultimate hat
namely the most excellent PiDP8/I developed by Oscar Vermeulen and supported by many other contributing
to the repo at https://tangentsoft.com/pidp8i/wiki?name=Home.
Using the RP2350 which is an update of the well known RP2040 ... Pi Pico allows for 2 synchronous
processes to provide a simulator loop and to run the front panel display.
I would note that I have been tempted in the past to design and build a board like this but other
experiences with similar projects using an ESP32 have clearly shown that this is no easy task.
I have however found out that this was no easy programming task in that this board does present
some interesting problems:
The first difficulty was getting a reliable interface to the SD Card but it turned out that all the
standard Arduino libraries do not work. So have has to include (a lot of) example code to drive the card
from the Waveshare site. All of these files are in the /src directory.
The second problem was that the Pi interface GPIO pins are different to a standard Raspberry Pi.
This required a lot of bit twiddling to get everything lined up for the front panel display and the switches.
Then it became apparent that the SPI interface to the SD Card was disrupted by the GPIO activity
for the front panel such that a semaphore sharing system was required.
The next interesting wrinkle was a requirement to present the SD Card contents as a USB drive so that
files may be added or deleted without having to remove the SD Card from the board. However as the disk images
for the PDP8 are on the SD Card, this required some functionality to ensure that the SD Card contents were
consistent. We all know the risks of shared files when you don't have an OS to look after this function.
Despite this, the system as it stands seem reasonably stable.
Practically speaking, the only thing to look out for is to use a 4GB SD Card as this has a 512 byte block size.
It may be that larger cards may work, I haven't done any tests in this regard.
To get going, open the project in the Arduino IDE, select a Waveshare RP2350 Plus as the board, set the
USB stack to 'Ada fruit Tiny USB' and the clock rate to 200 MHz. This clock setting results in a
simulator loop time of about 1.5 uS ... just a little slower that the real thing. You can check this with
an oscilloscope on GPIO pin 19 which is on pins 15/16 of the extension connector on the PiDP11/I board.
The Arduino setup can be a bit of a challenge with various libraries but I suspect the app will compile
without any major changes .... see what error you get and have a look for the relevant libraries.
then dig out your priceless PiDP8/I and plug in the board. Attach the USB port to your PC and you should see:

SD Card size:7741440<br>
Attach SD Card as USB drive (y/N):<br>

Type 'y' and then wait for the SD Card to appear on your PC. Copy the OS images from the /data and /data-DF
directories to the SD Card. The, press any key to restart the app.
When you see the above again, just press return a few times and this should look like this:

Attach SD Card as USB drive (y/N):n<br>
Enter paper tape reader filename:<br>
File not found<br>
File PUNCH.TAPE attached.<br>
Run....<br>
Boot (1:DMS 2:OS/8):<br>
Run from: 0030<br>
.<br>

And, the front panel should light up! You should see a rock steady glow from the LEDS.
try a DIR command and you will see them flash away with varying brightness.
There is no ILS code involved!!!!
the Display cycles at about 1.5 ms (666 Hz) such that the brightness variation is due to
the on/off ratio.
Then you can try a few things in OS/8.
eg:
. R LOADER
*CC, LIBC/I/O/G

this will run a c program and really flashes all the lights including the MQ.
Or:
R ABDLSR
*L/G

This will run Gordon Henderson's Larsen app which also flashes the lights .... quite a bit!


