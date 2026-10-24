#pragma once

namespace CRASHREPORT
{
    /**@brief
        Enable dmup mode
        After turning it on, when the program crashes, a dump file will be generated, which is in the C:\Users\<username>\AppData\Local\Temp directory.
        File name format: crash time_program name.exe.dmp
        As follows
        20150116174802_CrashShare.exe.dmp
    */
    void StartCrashReport();
}