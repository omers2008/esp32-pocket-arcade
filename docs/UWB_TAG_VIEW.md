# UWB Tag View

This arcade utility reads a Makerfabs MaUWB AOA Gen2 Anchor and shows the
distance and angle of its paired Gen2 tag on the existing 320x240 landscape
ST7789 screen. It follows the supplied TagViewer Python application's layout,
angle convention, validity checks, trail, smoothing and raw-reading overlay.
The Anchor tracks compatible tags; it does not detect arbitrary objects.

Open **UWB Tag View** in the main menu. It appears after the games and before
the final **Floor Select** ESP-NOW control entry. Neither utility uses a game
difficulty or a high score. Existing game scores and joystick settings keep
their storage keys.

## Wiring

Turn off power while wiring. Use the Anchor's printed signal labels, since
connector positions may differ between the bare board and adapter assemblies.
The datasheet lists TXD(1), RXD(3), GND(4), and VCC(5); it does not define pin 2.

| Anchor label | Connection |
| --- | --- |
| VCC | Regulated **5 V**; the 5 V rail of the USB-powered ESP32 base may be used if rated for the module's load |
| GND | ESP32 **GND**; share ground with any separate 5 V supply |
| TXD | ESP32 **GPIO16 / D16 / RX2**, using the 3.3 V-safe interface described below |
| RXD | Leave disconnected; the viewer sends no commands |

**5 V VCC is a power specification, not proof of the UART signal voltage.**
The supplied datasheet calls the data interface UART-TTL but does not specify
TXD's high voltage. ESP32 GPIO requires 3.3 V logic. Do not connect an
unverified 5 V TXD signal directly to D16 or any other GPIO.

- If TXD is confirmed to be 3.3 V logic, connect TXD directly to GPIO16.
- If TXD is confirmed to be 5 V logic, use a UART-compatible 5 V to 3.3 V
  level translator. A resistor divider is another option for a confirmed 5 V
  output: TXD -> 1 kOhm -> GPIO16, then GPIO16 -> 2 kOhm -> GND (about 3.33 V).
  Do not use that divider on a 3.3 V TXD output; its output would be too low.
- If TXD is unknown, first measure/confirm its level, or use a 3.3 V-output
  buffer whose input is explicitly rated to accept both 3.3 V and 5 V signals.

GPIO12 is already the back/menu button, GPIO13 is select, and GPIO14 is the
secondary button. Do not use those for the Anchor. GPIO17 is reserved by this
utility as UART TX but remains physically unconnected. UART2 runs at
**115200 baud, 8 data bits, no parity, 1 stop bit**.

The existing screen wiring stays: SCL/SCLK=18, SDA/MOSI=23, CS=27, DC=26,
RST=25, with VCC and BL at 3.3 V. The screen's SCL/SDA labels are SPI signals.

## Controls and display

| Control | Action |
| --- | --- |
| 13 | Select the next discovered tag/anchor pair |
| 14 | Toggle display smoothing (about 0.25 seconds) |
| Joystick left/right | Change range: Auto, 1, 2, 5, 10, 20, 50, 100, 200, 700 m |
| Joystick up/down | Toggle the amber raw-reading overlay |
| 12 | Close UART and return to the arcade menu |

Center the joystick between menu steps. The automatic scale starts at 2 m
and expands as needed. The radar is a schematic top view: zero degrees is
ahead, negative angles go left, positive angles go right. The front half-circle
shows +/-90 degrees, following the Python viewer; the datasheet specifies a
120-degree physical positioning field. A reported point beyond the selected
range or behind the front half-circle is labeled **OUTSIDE VIEW**, not clamped
to a misleading visible point.

The distance/angle cards show the selected tag, a short trail shows recent
movement, and amber marks show raw data when enabled. Smoothing changes only
the display, not the Anchor configuration or measurement accuracy. Eight
tag/anchor pairs and 32 trail points are kept in bounded memory. When the tag
table is full, new identities replace the oldest non-selected entry.

After one second without a current valid report, the dot and trail disappear
and the display reads **NO SIGNAL**. A new invalid report also removes the live
dot immediately. **NO DATA** means no UART bytes have arrived since opening;
**FORMAT?** means bytes arrived but no supported positioning report was decoded.
The RX byte count and rejected-frame counter help distinguish those cases.

## Protocol and setup

The parser accepts the binary **General Format** (81 bytes, command 0x01) and
**General Format RSSI** (89 bytes, command 0x64). It checks the 0x2A header,
little-endian payload length, 0x23 footer, XOR checksum, report direction,
Anchor ID 0-3, the selected Anchor's mask bit, angle bounds and nonzero range.
Filtered and raw distances are centimeters on the wire and meters on screen.
The RSSI format's per-Anchor stride is six bytes rather than four.

The Anchor must already be paired and streaming one of these formats. If the
supplied Python viewer already works with it, its existing report configuration
is compatible. The ESP32 performs no pairing, configuration, resets, firmware
updates or UART writes. Do not connect an additional transmitter to the same
UART line. Disconnect any USB adapter's TX from the Anchor RXD while using this
passive wiring.

Source: [Makerfabs Anchor datasheet](https://github.com/Makerfabs/MaUWB_AOA-Gen2/blob/main/datasheet/Anchor/MaUWB_AOA%20Gen2%20Anchor_Data_Sheet_v1.0.docx)
and the user-supplied `TagViewer/tag_viewer.py` reference. Pinout diagrams were
inspected alongside the protocol tables; the reference files are not changed
or copied into this repository.

Software tests cover both formats and all four Anchor offsets, byte fragments,
corruption, resynchronization, invalid/stale reports, smoothing, tag selection,
bounded storage, controls, coordinates, and rendering. These checks do not
confirm physical UART voltage or a live Anchor-to-ESP32 connection.
