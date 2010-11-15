using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;

namespace UsbDummyProtectGui
{
    enum ButtonAction { Close, Refresh, Protect, Unprotect };

    public partial class MainForm : Form
    {
        string[] dimentions = new string[] { "B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB" };

        ButtonAction action = ButtonAction.Close;

        public MainForm()
        {
            InitializeComponent();
        }

        private string GetSize(long value)
        {
            int rest = 0;
            foreach (string dimention in dimentions)
            {
                if (value < 1024)
                {
                    string r = rest > 0 ? "." + rest.ToString() : "";
                    return value.ToString() + r + dimention;
                }
                long old = value;
                value >>= 10;
                old -= value << 10;
                rest = (int)old / 102;
            }
            return "?";
        }

        private void MainButton_Click(object sender, EventArgs e)
        {
            switch (action)
            {
                case ButtonAction.Protect: Protect(); break;
                case ButtonAction.Unprotect: Unprotect(); break;
                case ButtonAction.Refresh: LoadList(null); break;
                default: Close(); break;
            }
        }

        private void Protect()
        {
            DriveInfo di = cbDrives.SelectedItem as DriveInfo;
            if (di == null)
            {
                return;
            }
            Windows7Taskbar.SetProgressState(Handle, ProgressState.Indeterminate);
            MainButton.Text = "Close";
            action = ButtonAction.Close;
            try
            {
                MainButton.Enabled = false;
                MainButton.Update();
                long cnt = 0;
                long size = di.TotalFreeSpace;
                while (size > 0)
                {
                    cnt++;
                    FileInfo fi = new FileInfo((di.RootDirectory.FullName + "dummy.file" + (cnt > 1 ? cnt.ToString() : "")));
                    using (FileStream fs = fi.OpenWrite())
                    {
                        InfoLabel.Text = String.Format("Writing {0} {1} remaining", fi.Name, GetSize(size));
                        InfoLabel.Update();
                        long newSize = Math.Min(fs.Length + size, 4294967294);
                        fs.SetLength(newSize);
                    }
                    size = di.TotalFreeSpace;
                }
                InfoLabel.Text = String.Format("{0} dummy file(s) created, {1} free", cnt, GetSize(di.TotalFreeSpace));
            }
            catch (Exception ex)
            {
                InfoLabel.Text = ex.Message;
            }
            MainButton.Enabled = true;
            Windows7Taskbar.SetProgressState(Handle, ProgressState.NoProgress);
        }

        private void Unprotect()
        {
            DriveInfo di = cbDrives.SelectedItem as DriveInfo;
            if (di == null)
            {
                return;
            }
            Windows7Taskbar.SetProgressState(Handle, ProgressState.Indeterminate);
            MainButton.Text = "Close";
            action = ButtonAction.Close;
            try
            {
                string[] files = Directory.GetFiles(di.RootDirectory.FullName, "dummy.file*");
                foreach (string file in files)
                {
                    File.Delete(file);
                }
                long free = di.TotalFreeSpace;
                InfoLabel.Text = String.Format("{0} dummy file(s) deleted, {1} free", files.Length, GetSize(free));
            }
            catch (Exception ex)
            {
                InfoLabel.Text = ex.Message;
            }
            Windows7Taskbar.SetProgressState(Handle, ProgressState.NoProgress);
        }

        private void LoadList(string root)
        {
            cbDrives.Items.Clear();
            DriveInfo[] drives = DriveInfo.GetDrives();

            foreach (DriveInfo di in drives)
            {
                if (di.DriveType == DriveType.Removable)
                {
                    cbDrives.Items.Add(di);
                    if (di.RootDirectory.FullName == root)
                    {
                        cbDrives.SelectedItem = di;
                    }
                }
            }

            if (cbDrives.SelectedItem == null && cbDrives.Items.Count > 0)
            {
                cbDrives.SelectedIndex = 0;
            }
        }

        private void MainForm_Load(object sender, EventArgs e)
        {
            Windows7Taskbar.SetProgressState(Handle, ProgressState.NoProgress);
            LoadList(Directory.GetDirectoryRoot("."));
        }

        private void cbDrives_SelectedIndexChanged(object sender, EventArgs e)
        {
            MainButton.Text = "Refresh";
            action = ButtonAction.Refresh;
            DriveInfo di = cbDrives.SelectedItem as DriveInfo;
            if (di == null)
            {
                InfoLabel.Text = "Select drive";
                return;
            }
            try
            {
                long free = di.TotalFreeSpace;
                InfoLabel.Text = GetSize(free) + " free";
                if (free == 0)
                {
                    MainButton.Text = "Unprotect";
                    action = ButtonAction.Unprotect;
                }
                else
                {
                    MainButton.Text = "Protect";
                    action = ButtonAction.Protect;
                }
            }
            catch (Exception ex)
            {
                InfoLabel.Text = ex.Message;
            }
        }
    }
}
