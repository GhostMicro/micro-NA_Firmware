# Firmware Build Rules

> [!CAUTION]
> **Mandatory Rules - Do Not Violate**

## Quick Reference

Every time you build firmware, you **MUST**:

1. **Save location**: `docs/assets/firmware/`
2. **Naming format**: `{Board}-{Version}_firmware_{BuildDate}.bin`
3. **Example**: `ESP32-NA_V1.0.0_firmware_20250129.bin`

## Full Documentation

See [docs/assets/firmware/readme.md](docs/assets/firmware/readme.md) for complete rules and guidelines.

## Build Workflow

```bash
# 1. Build
pio run

# 2. Copy & Rename
cp .pio/build/esp32dev/firmware.bin \
   docs/assets/firmware/ESP32-NA_V1.0.0_firmware_20250129.bin

# 3. Commit
git add docs/assets/firmware/
git commit -m "build: add firmware ESP32-NA_V1.0.0 (20250129)"
```

---

**For detailed rules, see**: [docs/assets/firmware/readme.md](docs/assets/firmware/readme.md)
