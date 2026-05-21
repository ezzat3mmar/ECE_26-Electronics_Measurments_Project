# Digitally Controlled Low-Side Buck Converter with Closed-Loop PI Control

An advanced undergraduate capstone project for the **Electronics Measurement** course (3rd Year, Electronics & Communications Engineering, Tanta University). This project replaces traditional analog compensation networks with a fully digital, firmware-based Proportional-Integral (PI) control system running on an Arduino Nano (ATmega328P).

---

##  Comprehensive System Overview

The core objective of this project is to implement a digitally controlled buck converter operating in Continuous Conduction Mode (CCM). Traditional buck converters rely on dedicated analog PWM controller ICs or complex op-amp error amplifiers. This system shifts that entire control loop into software. 

The microcontroller samples the fluctuating output voltage, passes the error signal through a discrete Proportional-Integral (PI) algorithm, and dynamically updates the hardware duty cycle to maintain a steady target voltage despite variations in load resistance or input supply voltage.

---

##  System Architecture

The project features a clear separation between the high-current **Power Stage** and the interrupt-driven **Digital Control Path**. Below is the system dataflow and topology mapping:

```text
       +-----------------------------------------------------------+
       |                                                           |
       v                                                           |
+--------------+   5V PWM (31.25 kHz)   +-------------------+      |
|              |----------------------->|    Power Stage    |      |
| Arduino Nano |                        | (IRFZ44N MOSFET)  |      |
| (ATmega328P) |                        +-------------------+      |
|              |                                  |                |
+--------------+                           Low-Side Switching      |
  ^          ^                                    v                |
  |          |                          +-------------------+      |
  |          |                          |   LC Filter &     |------| (VSource)
  |          |                          | Freewheeling Diode|      |
  |          |                          +-------------------+      |
  |          |                                    |                |
  |    I2C Bus (SDA/SCL)                          | (VDrain)       |
  |          v                                    v                |
  |   +-------------+                   +-------------------+      |
  |   |   SSD1306   |                   | Precision Divider |      |
  |   |  OLED (128x64)                  | (Differential)    |<-----+
  |   +-------------+                   +-------------------+
  |                                               |
  +--- Quadrature Pulses <--- +-----------------+ | Scaled Analog
                              | Rotary Encoder  | | (0 - 1.107V)
                              +-----------------+ v
                                              [ADC A3]
```
---
## Project Schimatic 
![Circuit diagram](photos/circuit image.png)
---
##  Firmware Task Execution Strategy

To achieve reliable closed-loop control alongside a responsive UI, the code avoids blocking operations (`delay()`) entirely. Instead, it utilizes a cooperative multitasking scheduler structure:

1. **Continuous / Immediate Execution**: The Rotary Encoder pins are monitored continuously via hardware interrupts to handle user inputs immediately.
2. **60 ms Discrete Timeline Boundary**: The system pauses to sample the ADC 150 times, solves the differential low-side voltage calculation, passes the result to the PI controller loop, and instantly updates the Timer1 PWM register.
3. **300 ms Display Boundary**: The microcontroller writes the latest calculated metrics onto the SSD1306 display framebuffer and executes an $I^2C$ transmission.
```
 MAIN LOOP 
 │
 ├──> [Every Iteration] ──────> Poll & Decode Quadrature Rotary Encoder.
 │
 ├──> [Every 60 ms] ──────────> 150-Sample Oversampling Read. 
 │                               Execute Discrete PI Control Equation. 
 │                               Adjust Timer1 31.25 kHz Duty Cycle. 
 │
 └──> [Every 300 ms] ─────────> Refresh Frame Buffer & Redraw SSD1306 OLED Display.
```
---
##  Documentation
Detailed technical documentation and project reports can be found in the `Docs` folder:

*    [Project documentation (PDF)](./Docs/Digitally_Controlled_Buck_Converter_Docs.pdf)
*    [project presentation (PDF)](./Docs/Digitally_Controlled_Buck_Converter_presentation.pdf)

---

##  Hardware Components

| Component Group | Part Number / Model | Functional Role inside the Circuit | Key Engineering Specifications |
| :--- | :--- | :--- | :--- |
| **Microcontroller** | Arduino Nano (ATmega328P) | Runs the digital PI loop, manages high-speed PWM registers, orchestrates oversampled ADC routines, and drives UI peripherals. | 16 MHz Clock, 10-bit native ADC, 5V Logic. |
| **Power Switching** | IRFZ44N N-Channel MOSFET | Positioned on the low-side to modulate power flow. Chosen for low conduction losses during ON state. | $R_{DS(on)} = 17.5\text{ m}\Omega$, $V_{DS(max)} = 55\text{ V}$, $I_D = 49\text{ A}$. |
| **Recirculation Diode** | MBRF30100CT Schottky | Acts as the freewheeling diode to provide a safe current path for the inductor energy when the MOSFET switches OFF. | Ultra-fast recovery, $V_f \approx 0.84\text{ V}$ @ $15\text{ A}$. |
| **Energy Storage (Filter)** | Inductor + Capacitor (LC) | Smooths out the high-frequency chopped square wave into a clean, low-ripple DC output voltage. | Tailored for Continuous Conduction Mode (CCM). |
| **Visual Output interface** | SSD1306 OLED Display | Provides instantaneous visual monitoring, displaying both the user-selected Setpoint Voltage and Measured Voltage. | $128\times64$ resolution, $I^2C$ communication protocol. |
| **User Input Control** | Quadrature Rotary Encoder | Enables manual, precision tuning of the target output voltage setpoint in real-time. | 2-Bit Quadrature, built-in push button, 0.1V step mapping. |
| **Voltage Feedback Network**| Precision Resistor Dividers | Steps down the high source and floating drain voltages to match the safe analog sampling boundaries. | $R_1 = 107.3\text{ k}\Omega$, $R_2 = 9.77\text{ k}\Omega$ ($\pm 1\%$ tolerance). |

---

##  Required Firmware Libraries

| Library Name | Official Inclusion Tag | Main Application & Dependency |
| :--- | :--- | :--- |
| **Encoder Library** | `#include <Encoder.h>` | Provides low-overhead decoding of the rotary encoder signals via hardware interrupts, avoiding missed clicks. |
| **Adafruit SSD1306** | `#include <Adafruit_SSD1306.h>` | Hardware-specific driver for handling display buffers and sending drawing commands over the $I^2C$ bus. |
| **Adafruit GFX** | `#include <Adafruit_GFX.h>` | Core graphics library providing geometric primitives (text formatting, lines, rectangles) used to structure the UI. |
| **Wire Library** | `#include <Wire.h>` | Built-in Arduino library required to initialize and manage $I^2C$ serial communication between the MCU and OLED screen. |

---

##  Technical Specifications & Operational Profiles

| Technical Feature | Operational Value | Engineering Rationale & Implementation Details |
| :--- | :--- | :--- |
| **Switching Frequency ($f_{sw}$)** | **31.25 kHz** | Configured via custom Timer1 register manipulation. Overrides the default 490 Hz Arduino PWM frequency to eliminate humanly audible acoustic coil whine. |
| **Control Loop Period** | **60 ms** ($f_{control} \approx 16.7\text{ Hz}$) | Controlled asynchronously via non-blocking `millis()` tracking. Balances real-time voltage corrections with CPU processing headroom. |
| **UI Refresh Interval** | **300 ms** | Dictates how often data is pushed to the OLED framebuffer. Prevents display flickering and frees up execution cycles for the main control tasks. |
| **Analog Reference ($V_{ref}$)** | **1.1V Internal Bandgap** | Replaces the unstable 5V VCC reference with the ATmega328P's internal precision bandgap to ensure highly consistent analog readings. |
| **Oversampling Depth** | **150 Samples per Loop** | Samples the analog channels 150 times consecutively during each control window. Smooths out high-frequency switching noise and increases Effective Number of Bits (ENOB). |
| **Low-Side Math Strategy** | Differential Sensing | Resolves the "floating ground" issue introduced by low-side switching. Calculates final load voltage dynamically: $V_{Load} = V_{Source} - V_{Drain}$. |

---


