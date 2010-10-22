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
    enum ButtonAction { Close, Protect, Unprotect };

    public partial class MainForm : Form
    {
        ButtonAction action = ButtonAction.Close;

        public MainForm()
        {
            InitializeComponent();
        }

        private void MainButton_Click(object sender, EventArgs e)
        {
            switch (action)
            {
                case ButtonAction.Protect: break;
                case ButtonAction.Unprotect: break;
                default: Close(); break;
            }
        }

        private void MainForm_Load(object sender, EventArgs e)
        {
            string root = Directory.GetDirectoryRoot(".");
            DriveInfo di = new DriveInfo(root);
            if (di.DriveType != DriveType.Removable)
            {
                InfoLabel.Text = "Execute utility on removable drive.";
                return;
            }
        }
    }
}
