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
| M5 ATOM Lite | `esp32:esp32:m5stack_atom:PartitionScheme=min_spiffs` | 987040 | `735a7d517957dffbb12d00c3e71d4ec5663007ad281794000adced0b6baed374` |
| M5 AtomS3 | `esp32:esp32:m5stack_atoms3:PartitionScheme=min_spiffs` | 968016 | `19791014e1af33b7cacaa4a39c0966d80d9154598e7ba230dd9cc067e2f740e8` |

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
rebuild before the first device flash. Publish the matching `firmware/*.bin` and
manifest files to the URLs shown in the manifest. Until those files are pushed
to the `protess/nag-killer` remote, the default manifest URL will return 404.

An older installed firmware with no OTA downloader needs one initial USB flash.
Subsequent releases can use the dashboard's **Check server** and **Install**
buttons.
