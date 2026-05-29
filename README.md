# CRT Calculator

Thie is the repository for my CRT calculator project. It contains the software for the microcontroller as well as production files for the PCB and case.

![Calculator](/assets/1.jpg)

## Project Idea

I knew about the completely intgerated, self-scanning flat CRTs available from China for a long time but never had a clear idea of what I could do with one. Then one day, YouTube recommended me [this video](https://www.youtube.com/watch?v=pjzK-Lppa1c), which inspired me to make my own interpretation of a CRT calculator.

## Implementation

My project differs from the original in a few aspects. Firstly, I wanted my calculator to be battery-operated. It should be able to chargef through USB power delivery. Secondly, I wanted mine to have a case, which would contain all the necessary components.

### PCB Design

I designed the PCB in EasyEDA. It contains all the components necessary including an ESP32 microcontroller, a charge controller, a 12V boost converter for the CRT and, of course, the keypad. All external components are attached using JST connectors.

![Schematic](/assets//pcb.png)

![PCB](/assets/8.png)

### Case Design

The case I designed in Onshape. It is intended to be laser-cut from 3mm plywood. I veneered it with walnut after assembly to give it a more uniform look.

![Onshape Design](/assets/7.png)

![Assembled](/assets/6.JPG)

### Software

The software is written in C(++) and is currently still fairly basic. It is capable of parsing compound math expressions but still lacks any graphing abilities, which would lend themselves well to the CRT aspect. I also had some fun implementing a couple of additional modes, including the games Snake and Space Invaders and the obligatory Bad Apple.

![Calculator](/assets/3.JPG)

![Space Invaders](/assets/5.JPG)

![Bad Apple](/assets/4.JPG)

### Videos

https://github.com/user-attachments/assets/d78290b7-812c-4778-8edb-842524321b57


https://github.com/user-attachments/assets/a9b024ef-ff24-45bf-a3b8-f66158780577


