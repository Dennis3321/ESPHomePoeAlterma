# ESPHome POE Daikin Alterma - Project Guide

## Project Overview

This is an ESPHome-based integration for Daikin Altherma heat pumps, specifically designed for M5Stack POE (Power over Ethernet) hardware. The project enables Home Assistant to monitor and control Daikin heat pumps via the X10A serial protocol.

### Key Features
- Read multiple heat pump parameters (temperatures, operation modes, etc.)
- Control operation modes: Off/Heat/Cool
- Smart grid feature support (4 modes: Free running, Forced off, Recommended on, Forced on)
- Built on ESPHome for easy configuration and updates
- Uses PoE for reliable power and network connectivity

## Hardware Setup

**Required Components:**
- M5 Stack ESP32 Ethernet Unit with PoE (SKU: U138)
- M5 Stack 4-Relay Unit (SKU: U097)
- M5 Stack ESP32 Downloader Kit (SKU: A105)
- Compatible Daikin Altherma 3 R F heat pump

**Pin Connections:**
- UART: RX=GPIO3, TX=GPIO1, 9600 baud, EVEN parity
- I2C (for relay unit): SDA=GPIO16, SCL=GPIO17
- Ethernet: IP101 PHY on GPIO23/GPIO18/GPIO0/GPIO5

## Project Structure

```
ESPHomePoeAlterma/
├── components/
│   └── daikin_x10a/           # Custom ESPHome component
│       ├── daikin_x10a.cpp    # Main component implementation
│       ├── daikin_x10a.h      # Component header
│       ├── daikin_package.h   # Packet handling
│       ├── register_definitions.cpp  # Register mappings
│       ├── register_definitions.h
│       ├── manifest.json
│       └── m5poe_user.yaml    # Extended configuration example
├── m5poe.yaml                 # Base ESPHome configuration
└── README.md                  # Detailed documentation
```

### Configuration Files Relationship

**Two-tier configuration pattern:**

1. **m5poe.yaml** - Base configuration
   - Hardware setup (UART, I2C, Ethernet, relays)
   - Basic daikin_x10a component initialization
   - Template sensors for common readings
   - Mode and smart grid controls

2. **m5poe_user.yaml** - Extended configuration (in components/daikin_x10a/)
   - Uses `packages:` to import base configuration from GitHub
   - Extends with custom register definitions via `registers:` array
   - Allows customization without modifying base config
   - Example shows extensive register mapping (200+ registers)

**Pattern example:**
```yaml
# m5poe_user.yaml structure
esphome:
  name: m5poe

packages:
  base: github://Dennis3321/ESPHomePoeAlterma/m5poe.yaml@main

# Define which registers to read from heat pump
daikin_x10a:
  uart_id: daikin_uart
  mode: 1
  registers:
    # mode: 0 = read only, not visible in HA
    - { mode: 0, registryID: 0x61, offset: 2, convid: 105, dataSize: 2, dataType: 1, label: "Internal calculation value" }

    # mode: 1 = read AND auto-create sensor in HA
    - { mode: 1, registryID: 0x61, offset: 10, convid: 105, dataSize: 2, dataType: 1, label: "DHW tank temp. (R5T)" }
    - { mode: 1, registryID: 0x61, offset: 8, convid: 105, dataSize: 2, dataType: 1, label: "Inlet water temp.(R4T)" }
    # ... sensors with mode=1 automatically appear in Home Assistant!
```

**No manual sensor definitions needed!** Sensors are auto-created for all `mode: 1` registers.

**When to use which:**
- Use `m5poe.yaml` directly for standard setup
- Create custom `*_user.yaml` based on `m5poe_user.yaml` for:
  - Different register selections
  - Additional sensors not in base config
  - Hardware variations
  - Testing new register mappings

**Visual flow:**
```
Heat Pump X10A Protocol
         ↓
[daikin_x10a component reads via UART]
         ↓
registers: array in m5poe_user.yaml
  - { mode: 0, label: "..." }  ← Read but not exposed to HA
  - { mode: 1, label: "DHW tank temp. (R5T)" }  ← AUTO-CREATES SENSOR
         ↓
[Home Assistant Entity] (automatically created for mode=1)
```

## Development Guidelines

### ESPHome Configuration
- Base config: `m5poe.yaml` (hardware + basic sensors)
- Extended config: `m5poe_user.yaml` (adds custom register definitions)
- Uses external_components to pull from this GitHub repository
- Requires secrets file for API encryption key and OTA password
- Supports `packages:` pattern for config inheritance and customization

### Custom Component (daikin_x10a)
- Written in C++ following ESPHome component architecture
- Implements X10A serial protocol for Daikin communication
- Exposes register values via `get_register_value()` method
- Supports custom register definitions via YAML configuration
- Register definition format: `{ mode, registryID, offset, convid, dataSize, dataType, label }`

**Dynamic Sensor Registration (IMPLEMENTED):**
- `mode: 0` = Read register but don't expose to HA (data available via `get_register_value()`)
- `mode: 1` = Read register AND automatically create sensor in HA
- No manual template sensor definitions needed for mode=1 registers
- Sensors automatically appear in Home Assistant based on register label

### Code Conventions
- Use ESPHome YAML syntax and conventions
- C++ code follows ESPHome component patterns
- Lambda functions in YAML for sensor value extraction
- Internal switches for relay control (not exposed to HA directly)
- Template selects for user-facing controls

### Relay Logic
**Relay assignments:**
- R1, R2: Heat pump mode control (Off/Cooling/Heating)
- R3, R4: Smart grid control (4-state logic)

**Safety pattern:** Always turn off relays before changing states:
```cpp
id(r1).turn_off();
id(r2).turn_off();
// Then set new state
```

### Temperature Sensors
All sensors follow this pattern:
```yaml
- platform: template
  name: "Sensor Name"
  id: sensor_id
  unit_of_measurement: "°C"
  device_class: temperature
  lambda: |-
    std::string val = id(daikin_comp).get_register_value("Register Name");
    if (val.empty()) return NAN;
    return atof(val.c_str());
```

## Important Notes

### UART Configuration
- Baud rate: 9600
- Parity: EVEN
- Stop bits: 1
- Debug mode available but commented out by default

### Ethernet (PoE)
- Type: IP101
- Power delivered via PoE eliminates need for separate power supply
- PIN 1 of X10A connector NOT used (PoE provides power)

### State Restoration
- Relays use `RESTORE_DEFAULT_OFF` for safety
- Ensures heat pump doesn't activate unexpectedly on power cycle

### Compatibility
Project is designed for specific Daikin models. See README.md for full list of compatible models (ERGA, EHVH, EHVX series).

## Building and Flashing

### Development Workflow
1. **Edit** code in VS Code
2. **Commit** changes using GitHub Desktop
3. **Push** to GitHub repository
4. In **Home Assistant ESPHome**:
   - Click "Clean" to remove cached external components
   - Click "Install" to build from GitHub and flash

### First-Time Setup
1. First flash requires M5Stack ESP32 Downloader Kit (SKU: A105)
2. Subsequent updates can be done via OTA through Home Assistant

### Component Updates
- ESPHome auto-pulls component updates from GitHub (5min refresh)
- Use "Clean" in ESPHome dashboard to force immediate update from GitHub
- Component is fetched from `external_components` GitHub URL on each build

## Testing

When modifying the custom component:
- Test with actual hardware or use UART debug mode
- Verify register value parsing returns valid data
- Check sensor values appear correctly in Home Assistant
- Test relay state changes don't cause unexpected behavior

## Common Tasks

### Adding New Sensors
To make a new sensor visible in Home Assistant, simply set `mode: 1` in the register definition:

**Example in m5poe_user.yaml:**
```yaml
daikin_x10a:
  uart_id: daikin_uart
  mode: 1
  registers:
    # This sensor will automatically appear in Home Assistant
    - { mode: 1, registryID: 0x61, offset: 10, convid: 105, dataSize: 2, dataType: 1, label: "DHW tank temp. (R5T)" }

    # This register is read but NOT shown in HA (useful for internal calculations)
    - { mode: 0, registryID: 0x60, offset: 5, convid: 203, dataSize: 1, dataType: -1, label: "Error type" }
```

**That's it!** No manual sensor definitions needed.

**Key points:**
- `mode: 1` = Sensor automatically created in Home Assistant
- `mode: 0` = Data read but not exposed to HA (still accessible via `get_register_value()`)
- Sensor name in HA = the `label` field
- All sensors default to temperature (°C) with 1 decimal place
- Labels must be unique

### Modifying Component
1. Edit files in `components/daikin_x10a/`
2. Component auto-updates from GitHub on ESPHome compilation
3. Can use local path instead of GitHub URL for testing

### Creating Custom Register Configuration
1. Copy `m5poe_user.yaml` as template
2. Modify `registers:` array to add/remove register definitions
3. Register fields:
   - `mode`: **0** (read but don't show in HA) or **1** (read AND auto-create sensor in HA)
   - `registryID`: X10A register address (e.g., 0x61)
   - `offset`: Byte offset within register
   - `convid`: Conversion ID for data interpretation
   - `dataSize`: Size in bytes
   - `dataType`: Type indicator (-1, 1, 2, etc.)
   - `label`: Human-readable name used as sensor name in HA - MUST be unique!
4. Use `packages:` to inherit base hardware configuration

**Dynamic Workflow (IMPLEMENTED):**
- `mode: 0` = Read register but don't expose to HA (accessible via `get_register_value()`)
- `mode: 1` = Read register AND automatically create sensor in HA
- Simply change `mode: 0` to `mode: 1` → sensor appears in Home Assistant!
- No manual template sensor definitions needed

### Debugging UART Communication
Uncomment debug section in `m5poe.yaml`:
```yaml
#debug:
#  direction: BOTH
#  dummy_receiver: true
#  after:
#    bytes: 16
```

## Known Limitations & Future Improvements

### Sensor Customization
**Current behavior:**
- All auto-created sensors default to:
  - Unit: °C (Celsius)
  - Device class: Temperature
  - Accuracy: 1 decimal place

**Future improvement:**
- Add optional fields to register definition for custom units/device classes
- Example: `{ mode: 1, ..., unit: "kW", device_class: "power" }`
- This would allow non-temperature sensors (pressure, flow, etc.) to have correct attributes

## Safety and Disclaimers

- Project is for educational/experimental purposes
- Modifying heat pump connections can void warranty
- Always follow heat pump safety manual
- No support or updates guaranteed
- Use at your own risk

## External Dependencies

- ESPHome framework
- M5Stack ESPHome components (for Unit4Relay)
- Home Assistant for integration
- GitHub repository for component distribution
