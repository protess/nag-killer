# Nag Killer 3.1 Remote OTA build report

## Source baseline

- Upstream: `https://github.com/06066060606060/nag-killer`
- Commit: `60847aff169aa21b4b54b5986708ee2e2cda9926`
- Firmware version: `NAG-KILLER-v3.1`
- Local M5 ATOM Lite pin change preserved: TX GPIO22 / RX GPIO19

## Verified builds

Both builds use the `min_spiffs` partition scheme. The M5Atom board default is
`huge_app`, which has no OTA slot and must not be used for this firmware.

| Target | FQBN | App size | SHA-256 |
| --- | --- | ---: | --- |
| M5 ATOM Lite | `esp32:esp32:m5stack_atom:PartitionScheme=min_spiffs` | 987040 | `e75a286a373461635ab024e6b98c203734bbe89b8d41c4713ddceac9080b11af` |
| M5 AtomS3 | `esp32:esp32:m5stack_atoms3:PartitionScheme=min_spiffs` | 968016 | `f2a584dc919a1cfe9b43d3a35aded61c050c27f7f04880bc8c9eae6b9cfe6096` |

Program usage was 50% for M5 ATOM Lite and 49% for AtomS3 against a 1,966,080
byte app slot. The generated ESP images passed esptool header, checksum, chip-ID,
and embedded validation-hash inspection. The dashboard JavaScript passed a
syntax check, both manifests parsed as valid JSON, and manifest byte counts and
SHA-256 values were checked against the actual binaries.

The compiler still reports the upstream 3.1 warnings for incrementing volatile
runtime counters. No warnings originate from the added remote OTA module.

## Deployment prerequisite

The generated binaries intentionally contain no WiFi password. Copy
`ota_config.h.example` to `ota_config.h`, enter the station credentials, and
rebuild before the first device flash. The matching `firmware/*.bin` and
manifest files are published on the `protess/nag-killer` `m5-atom` branch,
which is also the default OTA channel.

An older installed firmware with no OTA downloader needs one initial USB flash.
Subsequent releases can use the dashboard's **Check server** and **Install**
buttons.
