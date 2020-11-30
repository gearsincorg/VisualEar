Visual Ear project
By Phil Malone
Phil.Malone@Mr-Phil.com
11/24/2020

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

 I decided to do some research and experiments to determine what was possible.

 > A lot of what I developed here was influenced strongly by Steve Marley https://github.com/s-marley/ESP32_FFT_VU
 
 > An explanation of the Spreadsheet used to determine the Band Bins is found here: https://www.youtube.com/watch?v=Mgh2WblO5_c
 
 > This project adapts the FFT library found here : https://github.com/kosme/arduinoFFT
 