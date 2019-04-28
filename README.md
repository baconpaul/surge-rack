# Surge in Rack (experimental)

This repo holds an experimental set of code which aims to break apart the surge synth at
https://github.com/surge-synthesizer/surge/ into distinct rack modules for the oscillator,
filter, effect, and waveshaper block segments. Maybe one day the LFOs also.

To build it you need (1) patience, (2) to probably have had a chat with the team on slack and
(3) rack 1.0

If you are at step 3, here's instructions:

```
cd (working directory)
git clone https://github.com/VCVRack/Rack.git
cd Rack
git checkout v1
make dep
make
cd plugins
git clone https://github.com/VCVRack/Fundamental.git
cd Fundamental
git checkout v1
make
cd ..
git clone https://github.com/surge-synthesizer/surge-rack.git 
cd surget-rack
make
cd ../..
make run
```

and good luck! See if you can open the patch in surge-rack then.
