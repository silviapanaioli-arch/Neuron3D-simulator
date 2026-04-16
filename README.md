#Neuron3D: Hodgkin-Huxley and 3D Morphology Simulator


This project is a bio-physical simulator written in C that visualizes and simulates neuronal activity in a 3D environment using OpenGL and FreeGLUT. It integrates the Hodgkin-Huxley (HH) model for membrane dynamics and allows for the loading of complex neuronal morphologies from SWC files.

Scientific Background
The simulator calculates the membrane potential using the Hodgkin-Huxley model, accounting for Sodium (Na) and Potassium (K) conductance changes through gating variables m, h, and n. It supports multi-compartment neurons, where the morphology is parsed from SWC files and rendered as connected cylinders in 3D space.


How to Compile and Run

The project uses a makefile to manage the build process. To compile the program, you need a C compiler (GCC) and the OpenGL/GLUT development libraries.

Open your terminal (MSYS2 MINGW64 is recommended for Windows users).
Navigate to the project folder: cd /path/to/Neuron3D-simulator
Compile the project: make (Note: If make is not recognized, try mingw32-make).
Run the simulator: ./neuron3d.exe (Or use the shortcut: make run).

/data: Place your morphology files here. The program specifically looks for data/simple.swc when adding a new neuron.


Usage and Keyboard Commands

Once the simulator is running, use the following keys to interact with the 3D scene and the simulation parameters:

Arrows (Up/Down/Left/Right): Rotate the camera view around the 3D scene.

Plus / Minus: Zoom in or out of the 3D view (adjusts the zoom variable).

Spacebar: Trigger an electrical impulse. It resets the neuron variables for a new spike.

n: New Neuron. Loads data/simple.swc and adds a neuron to the scene.

c: Connect. Creates a synapse between the last two neurons added.

s: Slower. Increases the simulation delay by 5ms, slowing down the visual update.

f: Faster. Decreases the simulation delay, speeding up the visual update.

h: Higher Stimulus. Increases the stimulus amplitude by 10 nA.

l: Lower Stimulus. Decreases the stimulus amplitude by 10 nA.

r: Reset. Resets all simulation variables to their default state.


Default Values

The simulation starts with these predefined bio-physical and visual settings:

Stimulus Amplitude: 120.0 nA.

Resting Potential (V): -65.0 mV.

Initial Zoom: 600.0f.

Frame Delay: 10 ms (default simulation speed).

