using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace UsbDummyProtectGui
{
    /// <summary>
    /// Represents the thumbnail progress bar state.
    /// </summary>
    public enum ProgressState
    {
        /// <summary>
        /// No progress is displayed.
        /// </summary>
        NoProgress = 0,
        /// <summary>
        /// The progress is indeterminate (marquee).
        /// </summary>
        Indeterminate = 0x1,
        /// <summary>
        /// Normal progress is displayed.
        /// </summary>
        Normal = 0x2,
        /// <summary>
        /// An error occurred (red).
        /// </summary>
        Error = 0x4,
        /// <summary>
        /// The operation is paused (yellow).
        /// </summary>
        Paused = 0x8
    }

    [ComImportAttribute()]
    [GuidAttribute("ea1afb91-9e28-4b86-90e9-9e9f8a5eefaf")]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface ITaskbarList3
    {
        // ITaskbarList
        [PreserveSig]
        void HrInit();

        // ITaskbarList3
        void SetProgressState(IntPtr hwnd, ProgressState tbpFlags);

    }

    [GuidAttribute("56FDF344-FD6D-11d0-958A-006097C9A090")]
    [ClassInterfaceAttribute(ClassInterfaceType.None)]
    [ComImportAttribute()]
    internal class CTaskbarList { }

    /// <summary>
    /// The primary coordinator of the Windows 7 taskbar-related activities.
    /// </summary>
    public static class Windows7Taskbar
    {
        private static ITaskbarList3 _taskbar;
        internal static ITaskbarList3 Taskbar
        {
            get
            {
                if (_taskbar == null)
                {
                    lock (typeof(Windows7Taskbar))
                    {
                        if (_taskbar == null)
                        {
                            _taskbar = (ITaskbarList3)new CTaskbarList();
                            _taskbar.HrInit();
                        }
                    }
                }
                return _taskbar;
            }
        }

        static readonly OperatingSystem osInfo = Environment.OSVersion;

        internal static bool Windows7OrGreater
        {
            get
            {
                return (osInfo.Version.Major == 6 && osInfo.Version.Minor >= 1)
                    || (osInfo.Version.Major > 6);
            }
        }

        /// <summary>
        /// Sets the progress state of the specified window's
        /// taskbar button.
        /// </summary>
        /// <param name="hwnd">The window handle.</param>
        /// <param name="state">The progress state.</param>
        public static void SetProgressState(IntPtr hwnd, ProgressState state)
        {
            if (Windows7OrGreater)
                Taskbar.SetProgressState(hwnd, state);
        }
    }

}
