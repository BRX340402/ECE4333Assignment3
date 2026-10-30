# Small-obstacle setup

The UI remains English and camera startup remains CIF (400 x 296).

- Trigger: 10-50 cm, default 10 cm; three readings are still required.
- Obstacle length along travel: 3-20 cm, default 6 cm (placeholder, enter actual length).
- Clearance beyond obstacle: 20-40 cm, default 20 cm.
- The browser sends length + clearance as the existing par/D9 field: 23-60 cm, default 26 cm.
- Side-step is 12 cm; webpage default is Always left. Turns remain 90 degrees. This is the user-requested close-range experimental profile, not a verified collision-free path.
- Drive axle to floor sensors: 0-25 cm, provisional default 15 cm.
- Ultrasonic offset: Overtake::SENSOR_FORWARD_CM = 15 cm, provisional.
- Parallel travel = initial range + ultrasonic offset + obstacle length + clearance; at range 10 cm, defaults give 51 cm. The initial straight-space check adds 40 cm (91 cm total). The existing WAIT timeout can still start at 85% of that space after 6 seconds.

Geometry supplied: width ~15 cm, length ~22 cm, rear axle to rear ~7 cm, motor spacing ~8 cm, wheel diameter ~6 cm. Sensors are near the front with 2-3 cm uncertainty. The 15 cm sensor offsets are estimated from 22-7, not measured. Check both from the driven axle before tuning. Motor spacing was not substituted for wheel contact-centre track width; wheel diameter alone cannot calibrate the PWM-to-speed model.

This is preset-distance detouring, not obstacle-tail or width sensing. Forward emergency stop is 5 cm (three valid readings); detour immediate forward checks also stop at or below 5 cm. No turn timeout or gyro safety checks were relaxed. The new settings use a separate browser storage key so old distance/direction defaults do not silently return. Protocol field meanings are unchanged (UV43); flash both boards together because accepted ranges and defaults changed.

Validation: UNO target compilation passed (32162/32256 flash, 1462/2048 static RAM). JavaScript tests exercise validation, minimum/maximum inputs and persisted settings. Full host C++ suite requires g++, unavailable on this machine; new pass-distance C++ regression is included but not executed here. Hardware testing remains required.
