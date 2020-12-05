# Living Off The Land

## Fileless attack with persistence

Since antivirus software became increasingly better at detecting malicious files, the obvious solution is to not use any files at all.

![](https://bytecode77.com/images/pages/living-off-the-land/payload.png)

It is possible to achieve persistence by solely relying on existing operating system files to do the job. On Windows, there are lots of LOLBins (living off the land binaries), like Powershell. The registry can be used for storage. Technically, the registry is stored on the disk, therefore this is a Type II fileless attack.

**Payload.exe**: A native executable file, displaying a MessageBox. Goal is to have this executable run at startup (persistence) and never write it to the file system.

In addition, **scripts or other files** must not be written to the file system either.

## Execution and persistence

Execution and persistence need to happen in multiple stages.

### Stage 1: Installer

The installer (LivingOffTheLand.exe) is a native executable file. It can be either started normally (double clicking the EXE file), or it can be executed in memory, i.e. by a RCE exploit. Normal execution will likely be detected, while AV evasion is more likely with in-memory execution.

The installer's job:

- Write Injector.exe to the registry
- Write this **inline** powershell script to HKCU\...\Run for persistence
- Run powershell.exe for immediate execution

```
mshta "javascript:close(new ActiveXObject('WScript.Shell').run('powershell \"[Reflection.Assembly]::Load([Microsoft.Win32.Registry]::CurrentUser.OpenSubKey(\\\"Software\\\\Microsoft\\\\Internet Explorer\\\").GetValue($Null)).EntryPoint.Invoke(0,$Null)\"',0))"
```

This startup command is written to the HKCU\...\Run key. It may only have 260 characters (MAX_PATH). Powershell.exe loads Injector.exe from the registry and executes it in memory. Injector.exe is required to be written in C#! Because of the MAX_PATH restriction, there is only room to perform a simple Assembly.Load().EntryPoint.Invoke() here.

mshta.exe is not essential. But without it, a powershell window is briefly visible. The JavaScript allows to start powershell with SW_HIDE.

Because mshta is wrapping powershell, which is wrapping C#, multiple layers of string escaping are required :(

![](https://bytecode77.com/images/pages/living-off-the-land/registry.png)

To masquerade the registry value, a **null embedded character** is used. The name of the registry value starts with a NULL character, followed by the actual name. Since WinAPI uses null terminated strings, the name of the value technically equals to NULL. However, the script is still executed on startup because the content of the value is valid. The registry editor is unable to display this value, and so is any program that uses the WinAPI to read the registry. It is required to use the native API, which uses UNICODE_STRING allowing to read and write embedded NULL characters.

It is also required to use RemovalTool.exe to delete this value, or [Sysinternals RegDelNull](https://docs.microsoft.com/en-us/sysinternals/downloads/regdelnull).

Done! Persistence is now achieved. Next, powershell.exe is executed with the above inline script for immediate execution.

### Stage 2: Injector

Injector.exe is a C# executable that is stored in a registry value. The powershell inline script loads this executable via Assembly.Load() and invokes its main entry point.

The injector then proceeds to load the actual Payload.exe from its own executable resources. The payload is then injected using the process hollowing technique (RunPE). This injection technique works by creating a process of a legitimate Windows binary (e.g. svchost.exe). The process is created in a suspended state, after which its process memory is unmapped and replaced with the payload file. The thread context is set continue running at the entry point of the payload and then the main thread is resumed.

### Stage 3: Payload

As a result, a new process (C:\Windows\System32\svchost.exe) is visible in TaskMgr, but it's actually Payload.exe. This process cannot be distinguished from legitimate instances of the same file without significant effort. Most 32-bit Windows binaries can be used for process hollowing of 32-bit executables.

![](https://bytecode77.com/images/pages/living-off-the-land/process.png)

*... and not a single file has been written to the disk today.*

## Downloads

[![](http://bytecode77.com/public/fileicons/zip.png) LivingOffTheLand 1.0.1.zip](https://bytecode77.com/downloads/LivingOffTheLand%201.0.1.zip)
(**ZIP Password:** bytecode77)

## Project Page

[![](https://bytecode77.com/public/favicon16.png) bytecode77.com/living-off-the-land](https://bytecode77.com/living-off-the-land)