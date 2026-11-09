# Hardware verification status

Target: Waveshare ESP32-S3-Knob-Touch-LCD-1.8, not a similarly named 1.85-inch board.

Official references:
- https://www.waveshare.com/wiki/ESP32-S3-Knob-Touch-LCD-1.8
- https://www.waveshare.com/product/esp32-s3-knob-touch-lcd-1.8.htm

The indexed official material identifies the 1.8-inch knob product. Direct access to the official pages returned HTTP 403 during development. Consequently exact revision, LCD and touch controller, encoder press support, battery management, pins and supported firmware libraries remain unverified. The requested 360 x 360 resolution is the simulator specification, not a claim that a physical revision has been inspected.

No hardware-specific code is implemented. Before Milestone 5, obtain the exact unit revision, schematic and official example archive; verify controller names, all pins, encoder transport/behavior, charging and battery telemetry. Do not substitute another Waveshare board's configuration.
