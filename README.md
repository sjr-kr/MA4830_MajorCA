# MA4830_MajorCA

Waveform Generator README
=========================

Overview
--------
This application is a multi-threaded waveform generator designed for real-time operations in a QNX environment. It simulates digital-to-analog conversion by generating various waveforms (sine, square, triangular, sawtooth, and a placeholder arbitrary waveform) and outputting the values to the console. The design leverages pthreads for concurrent execution and non-canonical terminal I/O for real-time user input.

Compilation
-----------
To compile the code on QNX, ensure you have the necessary development tools and libraries (pthread, math, etc.). For example, you can compile using:
    
    gcc -o waveform_generator waveform_generator.c -lpthread -lm

Usage
-----
The program can be run with or without command-line arguments. When no arguments are provided, default settings are used:
    
    Default waveform: Sine
    Frequency: 1.0 Hz
    Amplitude: 1.0 (normalized)
    Steps per cycle: 100

Command-line arguments can override these defaults. The expected arguments are:

    waveform_generator [waveform] [frequency] [amplitude] [steps]

Where:
- **waveform**: Type of waveform ("sine", "square", "triangular", "sawtooth", or "arbitrary")
- **frequency**: Frequency in Hertz (Hz)
- **amplitude**: Normalized amplitude (range 0.0 to 1.0)
- **steps**: Number of discrete points per cycle

For example:
    
    ./waveform_generator square 2.0 0.8 150

Key Functions & Modes
---------------------
1. **Waveform Generation (waveform_thread)**
   - Continuously generates one complete cycle of the specified waveform.
   - Calculates the time delay between each point using `nanosleep` for precise timing.
   - Supports scaling to a 16-bit range, simulating a D/A converter.
   - Waveform types include:
     - **Sine:** Uses the sine function to create a smooth wave.
     - **Square:** Outputs low values for the first half and high values for the second half.
     - **Triangular:** Ramps up and then down linearly.
     - **Sawtooth:** Increases linearly over the cycle.
     - **Arbitrary:** Currently simulates a sine wave; can be extended to load custom data from disk.

2. **Real-time Keyboard Control (keyboard_thread)**
   - Monitors non-blocking keyboard inputs by configuring the terminal in non-canonical mode.
   - Allows on-the-fly adjustments:
     - **Up Arrow:** Increases frequency.
     - **Down Arrow:** Decreases frequency.
     - **Right Arrow:** Increases amplitude.
     - **Left Arrow:** Decreases amplitude.
   - Pressing the 'q' key (or 'Q') will signal the program to quit.
   - The terminal settings are restored upon program termination.

3. **Configuration Management**
   - **load_config:** Loads waveform parameters from a configuration file (`waveform.cfg`) if available.
   - **save_config:** Saves the current configuration to `waveform.cfg` upon exit to persist settings between runs.

4. **Signal Handling**
   - A SIGINT (Ctrl+C) handler is implemented to allow for graceful termination of the application.

Real-time Operations in QNX
----------------------------
- **Thread Scheduling:** The use of pthreads ensures that the waveform generation and keyboard input operate concurrently, fulfilling real-time requirements.
- **Precise Timing:** The use of `nanosleep` allows for high-resolution delays, crucial for accurate waveform generation.
- **Non-blocking I/O:** Terminal I/O is set to non-canonical mode to capture immediate user input without blocking the waveform generation thread.
- **Resource Management:** The program handles signal interruptions (e.g., SIGINT) and ensures resources (mutexes, terminal settings) are correctly managed for robust, real-time operation.

Troubleshooting & Notes
-----------------------
- **Keyboard Input Issues:** Verify that the terminal in the QNX environment supports non-canonical mode. Terminal configurations may vary between systems.
- **Real-time Performance:** Adjust the number of steps or delay values if the waveform generation does not meet real-time performance expectations.
- **Configuration File:** Ensure that `waveform.cfg` is accessible for reading/writing in the execution directory.

License & Authors
-----------------
This software is provided "as-is" without any warranty. Refer to the source code file for detailed licensing and author information.

Enjoy using the Waveform Generator!
