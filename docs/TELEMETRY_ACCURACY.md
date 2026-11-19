# Telemetry accuracy and compatibility

Audit date: 18 November 2026.

The app displays measurements from Windows counters and supported sensor providers. It does not generate demonstration readings in production. Preview/sample data lives in development-only test programs. An unavailable, invalid, stale, or ambiguously identified sensor must remain unavailable (`--`), rather than become an estimated number or a confirmed zero.

## Meaning of each value

| Value | Source and interpretation |
| --- | --- |
| CPU % | Busy processor time between two Windows counter samples. Idle is already included in kernel time and is subtracted once. This is busy time, not Task Manager's frequency-adjusted utility. Multi-group systems use the all-group Processor Information counter. |
| CPU GHz | Windows-reported average current MHz across the queried logical processors, divided by 1000. It is not a measured maximum boost clock. Zero/failed readings are unavailable. Frequency is unavailable on multi-group systems rather than misreporting one group as the whole system. |
| CPU temperature | An identified package/die/core sensor from the hardware provider. Package/die readings have explicit selection priority. ACPI thermal zones and distance-to-TjMax are not substituted for CPU temperature. |
| RAM | Physical total and available memory from GlobalMemoryStatusEx; used = total - available. Percentage uses those same byte counts. Inconsistent snapshots are rejected. |
| GPU % | Process contexts are grouped into physical engines; the busiest engine is selected. Different GPUs and different engines are not added into one percentage. Exact duplicate counter instances are counted once. |
| GPU temperature | Only a provider reading whose adapter identity is unambiguous. The legacy managed bridge exposes no adapter ID, so its GPU values are not combined with another GPU on hybrid systems. A sole NVIDIA handle can supply temperature only when it matches the active adapter's vendor. Ambiguous multi-NVIDIA temperatures remain unavailable. |
| Network speed | Byte-counter change for one selected interface divided by actual elapsed time, including subsecond intervals. Adapter index/LUID changes establish a new baseline and do not create a traffic spike. This includes traffic at that interface, not just web downloads. |
| Session traffic | Integer byte deltas counted once for the selected interface. Changing units or repainting does not add traffic. |
| Battery watts | Signed absolute battery rate in mW divided by 1000, aggregated across system battery packs once per device identity. Positive = charging; negative = discharging. Relative-unit, failed, or partially missing pack readings do not become a watt total. UPS devices are excluded. |
| Storage | Capacity available to the caller from GetDiskFreeSpaceEx, including Windows quota behavior. Duplicate volume GUIDs are not added as separate drives. Invalid free/total pairs are rejected. |

UI values are rounded for readability; internal network/history byte counts remain integers. KB/MB/GB displays use 1024-based units. A sampling interval means values are interval measurements, not instantaneous readings or promises of agreement with another app sampled at a different time.

## Network history and duplicate protection

- Windows-recorded history and app-tracked adapter history are separate sources. They are never added together. Tooltips identify the source and label cached Windows data.
- Summary queries deduplicate an adapter/profile identity. Distinct case-sensitive profile names are preserved.
- Today's and this month's validity are tracked separately. A failed subset of profiles is not presented as a complete total. Cached month totals must match the current month.
- Per-app attribution is collected and validated before normal rows are committed. Exact duplicate records are counted once. Conflicting records or attribution above the measured interface total produce an attribution-unavailable result; they are not rescaled into plausible numbers.
- Bytes Windows cannot attribute are labelled **Unattributed by Windows**. The app does not claim those bytes are proven kernel/system overhead.
- Windows SRUM accounting can be delayed. Physical and virtual/VPN interfaces can record the same underlying transfer at different layers. An all-network sum is interface-accounted traffic, not a deduplicated ISP bill or an exact unique-packet count. The CSV export records this limitation.
- App-tracked daily buckets assign changes at polling time; they are not a packet timestamp log. Windows history remains the preferred source for historical OS accounting.

## Windows and hardware support

The supplied build targets **Windows 10 1809+ and Windows 11, x64**. Optional DXCore classification is dynamically loaded; older Windows 10 systems can retain a neutral GPU label. Unsupported appearance attributes do not prevent basic telemetry. The supplied sensor bridge does not support native ARM64 or 32-bit builds.

Temperature access depends on the CPU/GPU, firmware, vendor driver, permissions, and the optional hardware-access provider. The UI continues with unavailable sensor fields when those requirements are not met. The app cannot certify the physical calibration of a vendor's sensor or make unsupported hardware expose measurements.

This workspace runs Windows 11 Home x64, build 26300. Windows 10 compatibility can be reviewed at the API/code level here, but a Windows 10 machine/VM and a hardware matrix are still required for runtime certification. No claim of testing every Windows 10/11 device is made.

## Validation

Result: all 18 CTest tests passed in Release with assertions enabled on Windows 11 x64 build 26300. A live standard-user sensor probe confirmed an NVIDIA GPU temperature from NVAPI matched to the active adapter; CPU temperature was correctly unavailable in that probe. Windows 10 runtime/hardware-matrix validation remains outstanding.

`telemetry_accuracy_tests` checks CPU arithmetic, exact RAM math, duplicate and multi-pack battery rates, GPU identity rules, profile/attribution deduplication, invalid data, elapsed-time rates, adapter-index reuse, and native Windows RAM totals. GPU aggregation, network reconnect/history/export tests, rendering tests, and the rest of the CTest suite provide additional coverage. Release test builds execute assertions (`/UNDEBUG`). Test outcomes are recorded in `build-review/Testing/Temporary/LastTest.log`.
