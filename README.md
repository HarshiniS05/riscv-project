# riscv-project
SUMMARY:
The Neonatal Respiratory Monitoring System is a cost-effective, real-time biomedical monitoring device designed to continuously observe vital parameters—respiratory activity, body temperature, and heart rate/SpO2—of neonates, especially in incubator settings. Using a powerful yet low-cost RISC-V based VSDSquadron Mini microcontroller, the system integrates analog sensors (LM35DZ and FSR402) through an MCP3008 ADC using SPI, and a digital MAX30102 pulse oximeter using I2C. The collected data is transmitted via UART (FT232RL) to a serial terminal or GUI for live monitoring, with alert thresholds to notify caregivers of abnormal conditions.
Unlike many existing neonatal monitors that are expensive, bulky, or not open-source, our solution stands out due to its affordability (total cost ≈ ₹2500), modular design, and open-source accessibility, making it ideal for deployment in resource-limited clinical settings and training labs. The system emphasizes ease of reproduction, scalability, and low power consumption, with compact PCB-level integration possibilities. Targeting TRL-8 readiness, it has been tested in simulated neonatal environments and is ready for integration in real-world healthcare settings.
WHY RISC INSTEAD OF FPGA?
In the initial phase of our project, an FPGA-based implementation was proposed due to its flexibility and parallel processing capabilities. However, for the final prototype, we transitioned to a RISC-based microcontroller (VSDSquadron Mini) to achieve greater simplicity, lower power consumption, and cost-effectiveness. While FPGAs are excellent for high-speed custom hardware designs, they require complex development workflows, HDL programming, and higher budgets, which are not ideal for compact, real-time biomedical systems with tight power and size constraints.
On the other hand, RISC (Reduced Instruction Set Computing) microcontrollers offer a streamlined architecture, allowing faster instruction execution, lower latency, and easier firmware development using C/C++. They also integrate peripherals like SPI, I2C, and UART, making sensor interfacing straightforward. Most importantly, RISC-based systems provide a balance of performance, power efficiency, and programmability, making them better suited for affordable, reliable, and scalable medical-grade prototypes like our neonatal monitoring device.
• Simpler Architecture: Easier to program and debug for embedded systems.
• Power Efficient: Better suited for low-power, continuous neonatal monitoring.
•   Cost Reduction: Reduces system cost significantly compared to FPGA.
•   Open-Source Ecosystem: Enables scalable development with open IP cores.
•   Fast Time-to-Market: Rapid prototyping and integration with widely used tools like Arduino IDE and PlatformIO.
SENSORS USED
SENSOR	PARAMETER
MEASURED	INTERFACE	CHANNEL/PIN USED
LM35DZ	Body Temperature	Analog	MCP3008 CH0
FSR402	Respiratory effort	Analog	MCP3008 CH1
MAX30102	Heart Rate / SpO2	I2C	SCL: PC3, SDA: PC4
1.	LM35DZ – Temperature Sensor
o	Measures the body temperature of the neonate.
o	Analog output proportional to temperature in °C.
o	Connected to Channel 0 (CH0) of MCP3008 ADC.
2.	FSR402 – Force Sensitive Resistor
o	Detects respiratory rate by sensing chest expansion and contraction.
o	Outputs analog signal based on pressure variations.
o	Connected to Channel 1 (CH1) of MCP3008 ADC.
3.	MAX30102 – Heart Rate and SpO₂ Sensor
o	Integrated pulse oximetry and heart-rate sensor.
o	Uses photoplethysmography (PPG) to measure oxygen saturation and heart rate.
o	Communicates over I2C with VSDsquadron Mini (RISC-V board).

HARDWARE COMPONENTS
•	VSDsquadron Mini (RISC-V MCU development board)
•	MCP3008 – 8-channel ADC for analog signal acquisition
•	FT232RL – USB to UART converter module
•	Breadboard, resistors, and jumper wires
•	Power supply: 3.3V/5V regulated

 
TRL-8 GOALS (TECHNOLOGY READINESS LEVEL 8)
The system targets TRL-8, focusing on:
•	End-to-end testing in real-world incubator settings
•	System integration and robust interfacing of sensors and MCU.
•	Validation of all sensing, signal conversion, and alert mechanisms
•	Demonstrating integration readiness in a hospital environment
•	Providing open-source, reproducible setup for broader adoption
•	Validated performance through data logs and demo runs.
•	User interaction via a serial display and potential mobile/PC GUI.
SETUP INSTRUCTIONS
•	Hardware Assembly:
o	Connect LM35DZ and FSR402 to CH0 and CH1 of MCP3008
o	Connect LM35DZ, FSR402, and MAX30102 sensors to ADC and I2C lines
o	Link MCP3008 to RISC-V via SPI (PC1–PC7) as per pin_mapping.txt
o	Use MCP3008 to digitize analog sensor outputs.
o	Connect MAX30102 via I2C (PC3, PC4)
o	Interface UART Module (FT232RL) to PC for serial monitoring and data output
•	Pin Mapping
Refer to hardware/pin_mapping.txt for detailed sensor → MCU mapping.

•	Firmware flashing:
o	Use firmware/firmware.ino or main.py to flash the RISC-V board via Arduino IDE/PlatformIO.
o	Configure build environment via platformio.ini if neede

•	Data Logging:
o	Sensor logs will auto-generate in /test_logs/ as .csv files
o	Graphs and serial screenshots in /test_logs/screenshots/
•	Data Monitoring
o	Connect the UART to PC and use Serial Monitor or Python script for data visualization.
o	Implement alert thresholds for vitals in code.
•	Testing
o	Record test logs in test_logs/
o	Compare output with standard neonatal vital ranges.
•	Demo Output:
o	Refer to demo/demo.video.mp4 for a <3-min system demonstration


