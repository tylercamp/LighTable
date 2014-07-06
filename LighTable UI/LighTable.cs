using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace LighTable_UI
{
	class LighTable
	{
		[DllImport("LighTable.dll", CallingConvention=CallingConvention.Cdecl, EntryPoint="StartLighTable")]
		public static extern void Start(String comPort);
		[DllImport("LighTable.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "StopLighTable")]
		public static extern void Stop();
		[DllImport("LighTable.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "LighTableIsRunning")]
		public static extern bool IsRunning();

		[DllImport("LighTable.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "SetColorSensitivity")]
		public static extern void SetColorSensitivity(float sensitivityFactor);
		[DllImport("LighTable.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "SetBrightnessSensitivity")]
		public static extern void SetBrightnessSensitivity(float sensitivityFactor);
		[DllImport("LighTable.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "SetColorMode")]
		public static extern void SetColorMode(int colorMode);

		public static int ColorMode_MonoColor_Random = 0;
		public static int ColorMode_HueCycle_MonoColor = 1;
		public static int ColorMode_HueCycle_Diverse = 2;
		public static int ColorMode_TransformColor = 3;
	}
}
