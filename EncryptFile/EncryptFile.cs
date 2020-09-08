using System.IO;
using System.IO.Compression;
using System.Linq;

public static class Program
{
	// Utility to compress and encrypt a file using GZip and a simple XOR algorithm for encryption.
	// Used for the VS build events.
	public static int Main(string[] args)
	{
		if (args.Length == 0) return 1;
		if (!File.Exists(args[0])) return 1;

		byte[] file = File.ReadAllBytes(args[0]);
		if (args.Contains("-c")) file = Compress(file);
		if (args.Contains("-e")) file = Encrypt(file);

		File.WriteAllBytes(args[0], file);
		return 0;
	}

	private static byte[] Compress(byte[] data)
	{
		using (MemoryStream memoryStream = new MemoryStream())
		{
			using (GZipStream gzipStream = new GZipStream(memoryStream, CompressionMode.Compress, true))
			{
				gzipStream.Write(data, 0, data.Length);
			}

			return memoryStream.ToArray();
		}
	}
	private static byte[] Encrypt(byte[] data)
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