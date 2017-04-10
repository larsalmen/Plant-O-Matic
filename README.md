# Plant-O-Matic
Plant watering automation based on Arduino Uno.
-----------------------------------------------------------------

The software monitors soil moisture levels and controls a pump connected to a relay board according to setpoints for wet and dry soil, and maximum pump time.

It has several other parameters available from the menu to fine-tune control, such as interval between measurements, how many samples is taken (and delay between those) each "run" and maximum pump runtime when dispensing water.

To minimize galvanic corrosion, and maximize sensor lifetime, the sensor is powered only for the few milliseconds it takes to get a sample.

It borrows heavily from Paul Siewert´s menu-system, with some tweaks, and Sverd Industries pumpless water system.


Changelog:

Version 1.1.0

+ Added alarm based on pump cycle count.
If the pump is activated for more than the set runtime, the cycle count is incremented. If the cycle counter reaches the alarm limit, the automation is stopped and an alarm message is shown.
Cycle power to reset.

- Some cleanup.

Version 1.0.0 - Initial release
