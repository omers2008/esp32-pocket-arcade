# UWB servo tag centering

The UWB ESP32 can pan the Anchor on a **positional SG90** to keep a tag near
zero degrees (straight ahead). This is local feedback control from the Anchor's
UART, so tracking also works while the Pocket Arcade is off. ESP-NOW continues
forwarding the measurements using the existing packet format; the Arcade needs
no new firmware for this servo feature.

## Connections on the UWB ESP32

Disconnect power while wiring. These are connections on the UWB ESP32
(`20:50:0D:6F:FC:68`), not the Pocket Arcade or the old servo/floor receiver.

| Servo wire | Connection |
| --- | --- |
| Orange/yellow signal | **GPIO18 / D18** on the UWB ESP32 |
| Red | Separate regulated **5 V** servo supply; about **2 A available** is recommended |
| Brown/black | Servo supply **GND**, joined to UWB ESP32 **GND** |

Do not power the servo from an ESP32 3.3 V pin. Servo current can also overload
a small board's USB/power path and cause resets; the separate supply avoids
that. When the ESP32 is USB-powered, join the supply grounds and keep the
servo's positive supply on the servo circuit rather than tying it to USB 5 V.
The three-wire servo's signal ground must share the ESP32 reference.
TowerPro's [SG90 guidance](https://towerpro.com.tw/product/sg90-7/) specifies
external-adapter power; actual clone current and travel vary.

The Anchor's wiring stays on this same UWB ESP32: VCC=5 V, GND=GND,
TXD=GPIO16 through the existing 3.3 V-safe interface, and RXD disconnected.
GPIO17 also stays disconnected. The servo signal is separate from the Anchor's
TXD wire. Do not use GPIO18 on the Pocket Arcade, where it is the screen clock.

## Tracking behavior

- A 50 Hz hardware PWM signal starts at 1500 microseconds (neutral).
- Only the first valid tag/Anchor pair is locked. Other identities do not steer
  the head or refresh its target timeout. The Arcade's 13 button still selects
  a display tag only; it does not change the servo's locked target.
- The controller integrates a proportional angular-error correction into the
  commanded position, rather than assigning `90 + measured angle` repeatedly.
  The sensor rotates with the servo, so its report is relative to that head.
- Error smoothing is about 120 ms. Correction starts at 7 degrees of error,
  and holds once within 4 degrees; hysteresis prevents small-error twitching.
- The commanded speed is capped at 20 degrees/second, with a 50 ms control
  period and a bounded catch-up after a delayed loop.
- Commanded travel is restricted to 30-150, corresponding to approximately
  1167-1833 microseconds in the conservative 1000-2000 microsecond mapping.
  These command units are not a measurement of the physical mount angle.
- Invalid reports, angles outside the documented +/-60 degree front field, or
  no matching valid report for 300 ms hold the last position. No blind sweep
  is performed. PWM continues holding that position rather than releasing it.
- Software limits stop further correction at the travel boundary. The unit may
  need manual base reorientation if the tag cannot be centered within that range.

Use a loose servo cable loop and check bracket clearance. At first startup the
servo seeks its neutral pulse, which may move a previously positioned arm.
The console below can pause and center it for mounting the horn. The radio
and Anchor reading continue even while tracking is paused.

## First direction check and calibration

Hold the tag still in front, slightly to one side. The head should turn toward
it, and the displayed relative angle should approach zero. A mounting-dependent
servo direction cannot be determined from the photo alone. If it turns away,
pause and reverse it using the UWB ESP32's USB serial console (115200 baud).
The default is reversed polarity; reversing is persisted across restarts in
the separate `uwbpan` preferences namespace.

| Console character | Action |
| --- | --- |
| P | Pause tracking and hold the current position |
| R | Reverse correction direction and save that choice |
| T | Resume tracking, waiting for a fresh valid report |
| C | Command neutral and pause, useful for horn alignment |
| N | Forget the identity lock and acquire the next valid tag/Anchor pair |

For a wrong direction: send **P**, then **R**, then **T**. Reversing invalidates
old feedback and waits for fresh data. Keep the tag still for the test.
If it buzzes against a stop, pause and check the physical clearance and supply;
software command limits cannot identify a wrongly mounted mechanical stop.

Once per second the log includes the commanded pan, pulse width, state, locked
tag, filtered error, and reversal setting alongside the UART/radio counters.
`CENTERED`, `TRACKING`, `NO TAG`, `LIMIT`, `PAUSED`, or `PWM ERROR` identify the
control state. A reboot starts enabled at neutral and reacquires a target;
pause is session-only, while direction is saved. Use the existing NVS keys for
other features as before; this code does not clear them.

`PanTracker.h` contains the gains, limits, deadband, and timeout. The sketch's
`SERVO_PIN` selects GPIO18, using the ESP32 core LEDC API without an additional
servo library. This firmware is for positional servos, not continuous-rotation
speed-control servos.

Tests simulate convergence with both motor directions, target-identity locking,
near-center noise, loss/invalid reports, pulse/travel limits, long-loop delays,
pause/reversal, and timer wrap. Physical servo polarity, horn alignment and
tracking accuracy must be checked on the assembled mount.
