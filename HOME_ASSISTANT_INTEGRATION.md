# Home Assistant Integration

The Daikin X10A component automatically decodes register values and makes them available to Home Assistant through two methods:

## Method 1: Template Sensors (Recommended - Simple)

Define template text sensors that call `get_register_value()` method:

```yaml
text_sensor:
  - platform: template
    name: "Daikin Operation Mode"
    lambda: |-
      return id(daikin_comp).get_register_value("Operation Mode");
    update_interval: 35s

  - platform: template
    name: "Daikin Water Temperature"
    lambda: |-
      return id(daikin_comp).get_register_value("Water Outlet Temperature");
    update_interval: 35s
```

The label (e.g., "Operation Mode") must match exactly what you defined in your `daikin_x10a` config registers.

## Method 2: Direct Publishing (Advanced - For Custom Triggers)

If you want the component to automatically publish values when they arrive (without polling):

1. Define text sensors in your `component.yaml`:
```yaml
text_sensor:
  - platform: template
    id: daikin_operation_mode
    name: "Daikin Operation Mode"
```

2. Register them with the component (via C++ code or future YAML automation):
```cpp
// This must be called during setup
daikin_comp->set_text_sensor_for_register("Operation Mode", op_mode_sensor);
```

Currently, Method 2 requires C++ code. **Method 1 is recommended for ease of use.**

## Configuring Registers

In your `m5poe_user.yaml`, set `mode: 1` for registers you want visible in Home Assistant:

```yaml
daikin_x10a:
  mode: 1
  uart_id: daikin_uart
  registers:
    - mode: 1
      convid: 0x65
      offset: 1
      registryID: 0x10
      dataSize: 1
      dataType: 2
      label: "Operation Mode"
    
    - mode: 1
      convid: 0x69
      offset: 0
      registryID: 0x61
      dataSize: 2
      dataType: 1
      label: "Water Outlet Temperature"
```

Only registers with `mode: 1` are decoded and published. Set `mode: 0` to hide registers from Home Assistant.
