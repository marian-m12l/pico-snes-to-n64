# SNES to N64 controller adapter

Emulate N64 controller with a SNES controller. I made this to play with sodium64 emulator.

# Wiring

Given the pinout:
```
  -----------------        1: VCC       4: Data
 | 1 2 3 4 | 5 6 7 )       2: Clock     7: Ground
  -----------------        3: Latch
```

The SNES controller is expected to be wired as follows:

```
 SNES         Pico
 ------------------
 1 VCC        3V3
 2 Clock      GP13
 3 Latch      GP14
 4 Data       GP15
 7 Ground     GND
```

The N64 controller data pin is expected to be wired to GP16.


## Build instructions

Be careful to clone this repository with its submodules.

```
mkdir build
cd build
cmake ..
make
```
