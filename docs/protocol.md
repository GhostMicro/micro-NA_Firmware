# CLI & Protocol Reference

## 📡 Web Serial Protocol (Short-Key JSON)

The vehicle expects JSON packets over Serial at **115,200 baud**.

### Outgoing (To Vehicle)
| Key | Value  | Purpose                      |
| --- | ------ | ---------------------------- |
| `c` | String | Command (sm, sp, ca, ra, es) |
| `t` | Int    | Throttle (0-100)             |
| `s` | Int    | Steering (-100 to 100)       |

### Incoming (Telemetry)
| Key  | Value  | Purpose                 |
| ---- | ------ | ----------------------- |
| `t`  | Int    | Type (1=Telemetry)      |
| `v`  | Float  | Battery Voltage         |
| `s`  | String | Status (OK, FAIL, IDLE) |
| `up` | Int    | Uptime in seconds       |
