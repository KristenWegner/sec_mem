using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace mkmcode
{
    class Program
    {
        private static readonly string dumpbin = @"C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin\dumpbin.exe";

        static void Main(string[] args)
        {
            if (args.Length < 1)
            {
                Console.WriteLine("mkmcode: Error: No input specified.");
                return;
            }

            string file = Path.GetFullPath(args[0]);

            if (!File.Exists(file))
            {
                Console.WriteLine("mkmcode: Error: File '{0}' does not exist.", file);
                return;
            }

            if (File.Exists(file + ".dumpbin"))
                File.Delete(file + ".dumpbin");

            Console.WriteLine("mkmcode: dumpbin.exe {0}", string.Format("/DISASM:BYTES /OUT:\"{0}.dumpbin\" \"{0}\"", file));

            ProcessStartInfo psi = new ProcessStartInfo(dumpbin);
            psi.Arguments = string.Format("/DISASM:BYTES /OUT:\"{0}.dumpbin\" \"{0}\"", file);
            psi.CreateNoWindow = true;
            psi.UseShellExecute = false;
            psi.RedirectStandardOutput = true;
            
            Process p = Process.Start(psi);

            while (!p.HasExited)
                Thread.Sleep(64);

            if (File.Exists(file + ".dumpbin"))
                File.Delete(file + ".c");

            StreamReader sr = new StreamReader(file + ".dumpbin");
            StreamWriter sw = new StreamWriter(file + ".c");

            sw.WriteLine("// {0}.c", file);
            sw.WriteLine();
            sw.WriteLine();

            string proc = "";
            List<string> bytes = new List<string>();

            while (!sr.EndOfStream)
            {
                string line = sr.ReadLine();

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
                        sw.WriteLine("// Proc: {0}.", proc);
                        sw.Write("static uint8_t {0}[] = {1} ", proc, '{');
                        for (int i=0; i < bytes.Count; ++i)
                        {
                            sw.Write(bytes[i]);
                            if (i < bytes.Count - 1)
                                sw.Write(", ");
                        }
                        sw.WriteLine(" {0};", '}');
                        sw.WriteLine();
                    }

                    proc = line.Trim().TrimEnd(':');
                    bytes = new List<string>();
                    continue;
                }

                if (line.Contains("int         3")) continue;

                if (line.StartsWith("  0"))
                {
                    line = line.Substring(20, 17).Trim();
                    if (line.Length > 0)
                    {
                        string[] parts = line.Split(' ');

                        foreach(string part in parts)
                            bytes.Add("0x" + part.Trim());
                    }
                }
                else if (line.StartsWith("                    "))
                {
                    string[] parts = line.Trim().Split(' ');
                    foreach (string part in parts)
                        bytes.Add("0x" + part.Trim());
                }
            }

            if (proc.Length > 0 && bytes.Count > 0)
            {
                sw.WriteLine("// Proc: {0}.", proc);
                sw.Write("static uint8_t {0}[] = {1} ", proc, '{');
                for (int i = 0; i < bytes.Count; ++i)
                {
                    sw.Write(bytes[i]);
                    if (i < bytes.Count - 1)
                        sw.Write(", ");
                }
                sw.WriteLine(" {0};", '}');
                sw.WriteLine();
            }

            sw.Flush();
            sw.Close();
        }
    }
}
