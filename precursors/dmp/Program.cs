using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Threading;

namespace dmp
{
    class Program
    {
        private static readonly string dumper = @"dumpbin.exe";

        static int Main(string[] args)
        {
			Console.WriteLine("MKC Object Dumper for Secure Memory Library"); 

			if (args.Length < 1)
            {
                Console.WriteLine("USAGE: dmp [file0 [.. fileN]]");
                return -1;
            }

			foreach (string arg in args)
			{
				try
				{
					string file = Path.GetFullPath(arg);
					string stem = Path.GetFileNameWithoutExtension(file);

					if (!File.Exists(file))
					{
						Console.WriteLine("dmp: Error: File '{0}' does not exist.", file);
						return -1;
					}

					bool isdb = false;
					Process p = null;

					if (file.EndsWith(".dumpbin"))
					{
						isdb = true;
						goto LOC_LOAD;
					}

					if (File.Exists(file + ".dumpbin"))
						File.Delete(file + ".dumpbin");

					string argument = string.Format("/DISASM:BYTES \"{0}\"", file);

					Console.WriteLine("dmp: Calling: {0} {1}", dumper, argument);

					ProcessStartInfo psi = new ProcessStartInfo(dumper);
					psi.Arguments = arg;
					psi.CreateNoWindow = true;
					psi.UseShellExecute = false;
					psi.RedirectStandardOutput = true;

					p = Process.Start(psi);

					while (!p.HasExited)
						Thread.Sleep(64);

					if (File.Exists(file + ".code"))
						File.Delete(file + ".code");

					LOC_LOAD:

					string target = Path.ChangeExtension(file, "code");
					Console.WriteLine("dmp: Generating: {0}...", target);

					StreamReader sr;

					if (isdb)
						sr = new StreamReader(file);
					else sr = p.StandardOutput;

					StreamWriter sw = new StreamWriter(target);

					sw.WriteLine("# {0}.code", stem);
					sw.WriteLine();
					sw.WriteLine();

					string proc = "";
					List<string> bytes = new List<string>();

					while (!sr.EndOfStream)
					{
						string line = sr.ReadLine();

						Console.WriteLine(line);

						if (string.IsNullOrEmpty(line) || line.Trim().Length == 0) continue;
						if (line.StartsWith("Microsoft")) continue;
						if (line.StartsWith("Copyright")) continue;
						if (line.StartsWith("Dump")) continue;
						if (line.StartsWith("File")) continue;
						if (line.StartsWith("LINK:")) continue;
						if (line.Trim().StartsWith("Summary")) break;

						if (char.IsLetter(line[0]) && line.Trim().EndsWith(":"))
						{
							if (proc.Length > 0 && bytes.Count > 0)
							{
								sw.WriteLine("# {0}: {1}.", stem.ToUpper(), proc);
								sw.Write("FN: {0} = ", proc);
								for (int i = 0; i < bytes.Count; ++i)
								{
									sw.Write(bytes[i]);
									if (i < bytes.Count - 1)
										sw.Write(", ");
								}
								sw.WriteLine(";");
								sw.WriteLine();
							}

							proc = line.Trim().TrimEnd(':');
							bytes = new List<string>();
							continue;
						}

						if (line.Contains("int         3")) continue; // Remove any "int 3" that occurs after a final "ret".

						if (line.StartsWith("  0"))
						{
							line = line.Substring(20, 17).Trim();
							if (line.Length > 0)
							{
								string[] parts = line.Split(' ');

								foreach (string part in parts)
									bytes.Add(part.Trim().ToUpper());
							}
						}
						else if (line.StartsWith("                    "))
						{
							string[] parts = line.Trim().Split(' ');
							foreach (string part in parts)
								bytes.Add(part.Trim().ToUpper());
						}
					}

					if (proc.Length > 0 && bytes.Count > 0)
					{
						sw.WriteLine("# {0}: {1}.", stem.ToUpper(), proc);
						sw.Write("FN: {0} = ", proc);
						for (int i = 0; i < bytes.Count; ++i)
						{
							sw.Write(bytes[i]);
							if (i < bytes.Count - 1)
								sw.Write(", ");
						}
						sw.WriteLine(";");
						sw.WriteLine();
					}

					sw.Flush();
					sw.Close();
				}
				catch (Exception e)
				{
					Console.WriteLine("dmp: Error: {0}.", e.Message.Trim().TrimEnd('.'));
				}
			}

			return 0;
		}
    }
}
