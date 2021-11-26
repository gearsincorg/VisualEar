Visual Ear project
By Phil Malone
Phil.Malone@Mr-Phil.com
6/21/2021

https://hackaday.io/project/175944-the-visual-ear 
 
 This project is an experiment in audio perception.
 My goal was to produce a visual display that might help a deaf person experience sound.
 I got excited when I imagined what an orchestral performance might look like if each musician had such a display above their head.
 
 I started thinking about this project when I discovered that our Cochlea (the sensing part of our inner ear) does not just contain "hairs" that turn vibrations into electrical signals. 
 Instead, it is comprised of many micro structures (containing hairs) which are located along a tapered structure that filters out increasing frequencies of sound as you move into the structure.
 So, in reality, each successive cluster of hairs is detecting higher frequency components. 
 
 Thus the Cochlea does not convert vibrations into electrical impulses, but instead, it converts audio frequencies into electrical impulses.  It's acting as a spectrum analyzer.
 This is a marvelous piece of evolutionary adaption which removed the requirement of the brain to sense signals that were changing many thousands of times a second.
 
 This was quite a revelation to me, and it made me wonder if an external device that converted sound waves into similar frequency bands, and then displayed these bands using light (color and intensity) could be a crude substitute for a Cochlea.

 My first instinct was to think of a spectrum analyzer display that you might see on a high-end audio amplifier.
 There is nothing new about visual audio-spectrum displays, but I decided that it would be important to provide MUCH finer spectral resolution, while still maintaining near real-time response.

Display Specs:

- Display Bands:  104 Bands, spanning 43 Hz - 16744 Hz (8.5 Octaves, 12 LED's per octave)
- Intensity Range: Each Band is displayed with a secific color, and 255 intensity levels
- Display Update interval: 11.6 mSec (86 Hz)
- Audio to Visual Latency:  Max 13.5 mSec (11.6 mSec acquisition + 1.9 mSec for dual FFT & display)
- Visual Persistance:  Low Frequency (< 440 Hz) 185 mSec.  High Frequency (>= 440 Hz) 46 mSec.

 ** HOW IT WORKS 
 
 Note:  Program constants are referenced in this description as follows:  64 (CONSTANT_NAME)
 
 The Teensy audio library is used to sample the I2S digital microphone continuously at the standard 44100 Hz rate.  Successive Bursts of 128 samples are cataloged and saved as an array of Burst pointers.  This array holds 72 (NUM_BURSTS) Burst pointers.  Only 64 (BURSTS_PER_AUDIO) of these are being processed at any time.  The remaining 8 (EXTRA_BURSTS) are used to start capturing more audio while the current Packet of 64 Bursts is processed.  
 
 To cover the full 55-17,153 Hz spectrum, it’s necessary to aquire 8192 samples (64 Bursts of 128 samples).  But this would take 185 mSec to collect, so it’s not practical to wait for the full sample to be collected before updating the display.  
 
 To permit faster display updates, a sliding window is created to process the most recent 64 Bursts, every time a new set of 4 (BURSTS_PER_FFT_UPDATE) Bursts have been received.  This event occurs every 11.6 mSec, which becomes the update rate for the overall display (86 updates per second).
 
 A standard FFT (Fast Fourier Transform) is used to convert the Audio Packet (8196 Samples) into frequency buckets.  Initially, a single FFT was used to process the entire sample.  This transform could be performed in the available 11.6 mSec, but a high frequency sound spike would be spread over the entire 185 mS sample duration.  So a sudden sound would appear quickly, but it would persist on the display for the full 185 mSec.  This effectively limits the responsiveness of the system to rapid sounds like a drum roll.
 
 To accommodate BOTH slow low-frequency sounds, and rapid high-frequency sounds, the available Audio packet is processed by two separate FFTs.  Each optimized for a different frequency range.  The Low FFT is looking for sounds in the 55-880 Hz range.   The High FFT is looking for sounds in the 880-17,153 Hz range.  
 
 To capture and resolve low sounds, the Low FFT uses the entire Audio Packet, but it only processes every fourth sample.  This enables the FFT to work with a smaller input sample set of 2048, which still produces 1024 frequency bins of 5.4Hz width.
 
 To capture and resolve rapid high sounds, the High-FFT only uses the most recent quarter of the Audio Packet, but at the full 44.1 kHz sample rate.  This enables this FFT to also work with a smaller input sample set of 2048, but produces 1024 frequency bins of 21.5Hz width.  Since this FFT is only using a quarter of the sample history, short-pulse sounds only persist for a max of 46.4 mSec, which should be able to display a 10 Hz drum-roll.
 
CREDITS:

 > A lot of what I developed here was influenced strongly by Steve Marley https://github.com/s-marley/ESP32_FFT_VU
 
 > An explanation of the Spreadsheet used to determine the Band Bins is found here: https://www.youtube.com/watch?v=Mgh2WblO5_c
 
 > This project adapts the FFT library found here : https://github.com/kosme/arduinoFFT
 
 
 