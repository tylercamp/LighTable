using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.IO.Ports;
using System.Linq;
using System.Management;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Forms;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace LighTable_UI
{
	/// <summary>
	/// Interaction logic for MainWindow.xaml
	/// </summary>
	public partial class MainWindow : Window
	{
		NotifyIcon m_NotifyIcon;

		private String SelectedPortName
		{
			get
			{
				String selectedPort = cbPortList.SelectedItem as String;
				selectedPort = selectedPort.Split(' ')[0]; // Extract port name from human-friendly name
				return selectedPort;
			}
		}

		private void StopLighTable()
		{
			LighTable.Stop();

			btnStart.IsEnabled = true;
			btnStop.IsEnabled = false;

			cbPortList.IsEnabled = true;

			m_StartMenuItem.Enabled = true;
			m_StopMenuItem.Enabled = false;
		}

		private void StartLighTable()
		{
			LighTable.Start(SelectedPortName);

			LighTable.SetColorSensitivity((float)sldBrightnessSensitivity.Value);
			LighTable.SetBrightnessSensitivity((float)sldBrightnessSensitivity.Value);

			btnStart.IsEnabled = false;
			btnStop.IsEnabled = true;

			cbPortList.IsEnabled = false;

			m_StartMenuItem.Enabled = false;
			m_StopMenuItem.Enabled = true;
		}

		System.Windows.Forms.MenuItem m_StartMenuItem, m_StopMenuItem;

		public MainWindow()
		{
			InitializeComponent();

			System.Windows.Forms.ContextMenu iconMenu = new System.Windows.Forms.ContextMenu();

			System.Windows.Forms.MenuItem openSettingsMenuItem, exitMenuItem, startMenuItem, stopMenuItem;
			openSettingsMenuItem = new System.Windows.Forms.MenuItem("Open Settings");
			openSettingsMenuItem.Click += openSettingsMenuItem_Click;

			startMenuItem = new System.Windows.Forms.MenuItem("Start LighTable");
			startMenuItem.Click += delegate(object sender, EventArgs e) { this.StartLighTable(); };

			stopMenuItem = new System.Windows.Forms.MenuItem("Stop LighTable");
			stopMenuItem.Click += delegate(object sender, EventArgs e) { this.StopLighTable(); };
			stopMenuItem.Enabled = false;

			exitMenuItem = new System.Windows.Forms.MenuItem("Exit");
			exitMenuItem.Click += exitMenuItem_Click;

			iconMenu.MenuItems.Add(openSettingsMenuItem);
			iconMenu.MenuItems.Add(startMenuItem);
			iconMenu.MenuItems.Add(stopMenuItem);
			iconMenu.MenuItems.Add(exitMenuItem);

			m_StartMenuItem = startMenuItem;
			m_StopMenuItem = stopMenuItem;

			String executableFileName = Process.GetCurrentProcess().MainModule.FileName;
			executableFileName = executableFileName.Replace(".vshost", "");
			Icon lighTableIcon = System.Drawing.Icon.ExtractAssociatedIcon(executableFileName);

			this.Icon = Imaging.CreateBitmapSourceFromHIcon(
				lighTableIcon.Handle, Int32Rect.Empty, BitmapSizeOptions.FromEmptyOptions());

			NotifyIcon notifyIcon = new NotifyIcon();
			notifyIcon.Icon = lighTableIcon;
			notifyIcon.Visible = true;
			notifyIcon.DoubleClick += notifyIcon_DoubleClick;
			notifyIcon.ContextMenu = iconMenu;
			
			m_NotifyIcon = notifyIcon;

			this.Closing += MainWindow_Closing;


			//	http://stackoverflow.com/questions/2837985/getting-serial-port-information
			using (var searcher = new ManagementObjectSearcher
				("SELECT * FROM WIN32_SerialPort"))
			{
				string[] portnames = SerialPort.GetPortNames();
				var ports = searcher.Get().Cast<ManagementBaseObject>().ToList();
				var tList = (from n in portnames
							 join p in ports on n equals p["DeviceID"].ToString()
							 select n + " - " + p["Caption"]).ToList();

				foreach (string s in tList)
				{
					cbPortList.Items.Add(s);
					if (s.Contains("Arduino"))
						cbPortList.SelectedIndex = cbPortList.Items.Count - 1;
				}
			}
		}

		bool m_ExitMenuClicked = false;

		void exitMenuItem_Click(object sender, EventArgs e)
		{
			m_ExitMenuClicked = true;
			m_NotifyIcon.Visible = false;
			LighTable.Stop();
			this.Close();
		}

		void openSettingsMenuItem_Click(object sender, EventArgs e)
		{
			this.Show();
			this.WindowState = System.Windows.WindowState.Normal;
		}

		void MainWindow_Closing(object sender, System.ComponentModel.CancelEventArgs e)
		{
			if (!m_ExitMenuClicked)
			{
				e.Cancel = true;
				this.Hide();
			}
		}

		void notifyIcon_DoubleClick(object sender, EventArgs e)
		{
			this.Show();
			this.WindowState = System.Windows.WindowState.Normal;
		}

		private void btnStart_Click(object sender, RoutedEventArgs e)
		{
			this.StartLighTable();
		}

		private void btnStop_Click(object sender, RoutedEventArgs e)
		{
			this.StopLighTable();
		}

		private void sldBrightnessSensitivity_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
		{
			if (LighTable.IsRunning())
				LighTable.SetBrightnessSensitivity((float)sldBrightnessSensitivity.Value);
		}

		private void sldColorSensitivity_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
		{
			if (LighTable.IsRunning())
				LighTable.SetColorSensitivity((float)sldColorSensitivity.Value);
		}

		private void ResetBrightnessSensitivity(object sender, RoutedEventArgs e)
		{
			sldBrightnessSensitivity.Value = 2.5;
		}

		private void ResetColorSensitivity(object sender, RoutedEventArgs e)
		{
			sldColorSensitivity.Value = 2.0;
		}

		private void rbMonoColorRandom_Checked(object sender, RoutedEventArgs e)
		{
			LighTable.SetColorMode(LighTable.ColorMode_MonoColor_Random);
		}

		private void rbHueCycleDiverse_Checked(object sender, RoutedEventArgs e)
		{
			LighTable.SetColorMode(LighTable.ColorMode_HueCycle_Diverse);
		}
	}
}
