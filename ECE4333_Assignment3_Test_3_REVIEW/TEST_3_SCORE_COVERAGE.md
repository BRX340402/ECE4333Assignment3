# Assignment 3 score sheet — Test 3 review status

| Rubric task | Status in Test 3 | Needs before demonstration |
|---|---|---|
| Controller tuning / Figure 8 | Baseline Test 2 line controller retained; right PWM trim experimental | Physical Figure 8 with track results; teammate PD controller not ported |
| Emergency stop | Web latched stop plus <=12 cm forward ultrasonic automatic stop; line-exhaustion fail-stop retained | Verify actual power/motor halt, obstacle response and reset |
| Kinematics prediction | Wheel PWM integration runs even without gyro; provisional coefficients | Measure wheel speed, spacing, trajectory and heading |
| Data logging / comparison | Wi-Fi CSV logger, state changes, estimated pose | Record *independent* actual checkpoints/time/heading; compare |
| Gyro fusion | Existing conditional MPU6050 gyro-based heading integration | Confirm actual chip; recent tests GY=0, no validated fusion |
| Camera sensor fusion | Disabled calibrated landmark scaffold; team alternate program exists | Map landmark, calibrate camera bearing/distance, port and test |
| Red traffic-light | Test 2 baseline red/green logic retained | Repeat test under scene lighting |
| STOP sign | Baseline FOMO / STOP_HOLD 3s | Physical stop duration; classification reliability |
| Intersection management | Existing Test 2 crossing state machine | Demonstrate intersection and track continuation |
| State machine | TrafficLogic + logged TS changes | Check transitions and failure cases |
| Serial protocol | Test 2 N204/N205/N221 + small N207 safety extension | Compile/upload matching UV=32 UNO/ESP32 |
| Vision-based target following | **Not active**; advanced teammate code preserved separately | Port nested camera outer loop / gyro inner loop to same mode/protocol, physical tune |
| Log global sign positions, completion time, speed, transitions | State events and *predicted* path/speed are logged | Actual map/ground truth, calibrated sign location, measured completion time |

**Avoid claiming implementation or demonstrated performance for the inactive/uncalibrated features.**
