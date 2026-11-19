# Sensor references and dependencies

TrafficMonitor by zhongyang219 is the implementation reference for the LibreHardwareMonitor bridge and Windows GPU Engine performance counters:
https://github.com/zhongyang219/TrafficMonitor
Inspected commit: 930f17533d6098989ebad62f210aa97d75ef174b.
The existing OpenHardwareMonitorApi DLL, import library and headers expose this project's bridge API. Its license and copyright notice are preserved in licenses/TrafficMonitor-LICENSE.txt. The GPU counter aggregator here groups by physical adapter and engine rather than combining different GPUs.

LibreHardwareMonitorLib.dll (existing version 0.9.4.0) provides hardware sensor access. Project: https://github.com/LibreHardwareMonitor/LibreHardwareMonitor. MPL 2.0 text is in licenses/LibreHardwareMonitor-LICENSE.txt. Sensors depend on driver and hardware support; no security settings are changed by this app.

HidSharp.dll 2.1.0 by James F. Bellinger is a required managed dependency of the supplied LibreHardwareMonitor library. Obtained from the NuGet HidSharp 2.1.0 package, net35 assembly. License: licenses/HidSharp-LICENSE.txt. Package: https://www.nuget.org/packages/HidSharp/2.1.0.
