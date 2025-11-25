# ATmega16 Dual-Mode Embedded Controller: Timer & Float Calculator

## 📖 Project Overview

This project features a custom-designed embedded system based on the **ATmega16 microcontroller**. It functions as a dual-purpose device: a **Precision Countdown Timer** for industrial relay control and a **Floating-Point Calculator**.

The system demonstrates advanced low-level engineering techniques, including **Time-Division Multiplexing** for display management and **Interrupt-Driven State Machines** to handle user inputs and real-time operations concurrently.

**Academic Context:**
* **Course:** Microprocessors and Microcontrollers
* **Institution:** VNU University of Engineering and Technology (UET)
* **Advisors:** Dr. Pham Manh Thang, CN. Vu Dinh Nam
* **Authors:** Le Cong Viet Anh, Tran Duc Manh, Le Thiem Giang

---

## ⚙️ System Architecture

The system is built around the AVR architecture, interfacing with power electronics and human-machine interfaces (HMI).

<img width="789" height="427" alt="Screenshot 2025-11-25 151430" src="https://github.com/user-attachments/assets/fbc12f05-dd42-4fd5-9fa7-933ed8c52bbc" />

1.  **Processing Unit:** ATmega16 (8MHz External Crystal) handles arithmetic logic and timing interrupts.
2.  **Display System:** 8-Digit 7-Segment LED display driven by **8x BJT Transistors** using a high-frequency multiplexing algorithm (>60Hz refresh rate).
3.  **Control Output:** A **12V Relay** isolated via optocouplers/transistors to switch high-power AC/DC loads (e.g., fans, lights, motors).
4.  **Input Interface:** 4x4 Matrix Keypad scanned via GPIO polling.

---

## 🛠 Hardware Specifications


[Image of System Block Diagram]


| Component | Specification | Function |
| :--- | :--- | :--- |
| **MCU** | **ATmega16** (DIP-40) | Central Processing Unit |
| **Display** | 2x 4-Digit 7-Segment Anode Common | Multiplexed Visual Output |
| **Drivers** | 8x BJT Transistors | Anode/Cathode switching for Multiplexing |
| **Switching** | 12VDC Relay  | Controls external High-Power Load |
| **Input** | 4x4 Matrix Keypad | User Data Entry |
| **Power** | 12V Input $\to$ 5V LDO | Power Management |
| **PCB** | Custom Design | Optimized footprint and signal routing |

---

## 🚀 Key Features

### 1. Dual-Mode Operation
* **Calculator Mode:** Performs addition, subtraction, multiplication, and division on **Floating-Point numbers**. It handles decimal points and negative signs logic on a raw 7-segment display.
* **Timer Mode:** User-programmable countdown timer (HH:MM:SS). Triggers the Relay OFF when the counter hits zero.

### 2. Time-Division Multiplexing (Multiplexing)
To drive 64 LED segments (8 digits x 8 segments) with limited GPIO pins, the system uses a scanning algorithm. Each digit is illuminated for **2ms** in a loop, utilizing the persistence of vision (POV) effect to create a steady display while saving **40% of GPIO pins**.

### 3. Floating-Point Arithmetic on 8-bit MCU
Implemented custom logic to split `float` data types into **Integer** and **Decimal** parts for display rendering, overcoming the hardware limitation of 7-segment LEDs which cannot natively show floating-point data.

---
## 🎮 Usage Guide
### Mode Selection
Mode switching is handled via hardware pins (or dedicated keys depending on wiring):

Mode 0 (Timer): Enter HH:MM:SS -> Press # to Start. Relay activates.
Mode 1 (Calculator): Enter numbers -> Press A/B/C/D for operations -> Press # for Result.

### 🎹 Keypad Mapping

The system uses a standard 4x4 matrix keypad. The function of each key changes slightly depending on the active mode (Calculator or Timer).

| Key Label | Calculator Mode Function | Timer Mode Function |
| :---: | :--- | :--- |
| **0 - 9** | Enter numbers (0-9) | Enter time (HH:MM:SS) |
| **A** | Divide ( `÷` ) | *(No function)* |
| **B** | Multiply ( `×` ) | *(No function)* |
| **C** | Subtract ( `-` ) | *(No function)* |
| **D** | Add ( `+` ) | *(No function)* |
| **\*** | **AC** (Clear All / Reset) | **Reset** Countdown to 00:00:00 |
| **#** | **=** (Calculate Result) | **Start / Pause** Countdown |

---


## 📂 Repository Structure

```text
├── firmware/
│   ├── timer_calc_2modes_FLOAT_ver2.cpp  # Main Source Code (C++)
│   ├── main.hex                          # Hex file for uploading to ATmega16
├── hardware/
│   ├── circuit.jpg                       # Circuit Diagram (Proteus)
│   ├── pcb_layout.jpg                    # PCB Board View
│   ├── schematic.jpg                     # Pin out Circuit before PCB
│   └── test.pdsprj                       # Proteus Simulation File
└── README.md                             # Project Documentation
