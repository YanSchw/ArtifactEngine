using System;
using System.Runtime.InteropServices;
using System.Text;

namespace Artifact
{
    public enum FieldKind
    {
        Float, Double, Int32, Bool, String, Vec2, Vec3, Vec4, Color, Class
    }

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct ScriptFieldDesc
    {
        public byte* Name;
        public int Kind;
        public uint Offset;
    }

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct ScriptClassDesc
    {
        public byte* Name;
        public byte* Parent;
        public byte* Module;
        public byte Ticks;
        public ScriptFieldDesc* Fields;
        public int FieldCount;
    }

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct NativeApi
    {
        public delegate* unmanaged<byte*, byte*, IntPtr> FindFunction;
        public delegate* unmanaged<IntPtr, IntPtr, void**, void*, byte> CallFunction;
        public delegate* unmanaged<byte*, IntPtr> CreateObject;
        public delegate* unmanaged<IntPtr, void> Retain;
        public delegate* unmanaged<IntPtr, void> Release;
        public delegate* unmanaged<IntPtr, IntPtr> NewWeak;
        public delegate* unmanaged<IntPtr, byte> IsAlive;
        public delegate* unmanaged<IntPtr, void> DeleteWeak;
        public delegate* unmanaged<IntPtr, byte*> ClassNameOf;
        public delegate* unmanaged<byte*, byte*> ParentClassNameOf;
        public delegate* unmanaged<ScriptClassDesc*, byte> RegisterScriptClass;
        public delegate* unmanaged<IntPtr, byte*, void> SetString;
        public delegate* unmanaged<IntPtr, byte*> GetString;
        public delegate* unmanaged<int, byte*, void> Log;
    }

    public unsafe readonly struct Utf8 : IDisposable
    {
        public readonly IntPtr Pointer;

        public Utf8(string text)
        {
            Pointer = Marshal.StringToCoTaskMemUTF8(text ?? string.Empty);
        }

        public byte* Bytes => (byte*)Pointer;

        public void Dispose() => Marshal.FreeCoTaskMem(Pointer);
    }

    public static unsafe class Native
    {
        internal static NativeApi Api;

        public static string ReadString(IntPtr utf8) => utf8 == IntPtr.Zero ? string.Empty : Marshal.PtrToStringUTF8(utf8);

        public static IntPtr FindFunction(string className, string functionName)
        {
            using Utf8 type = new Utf8(className);
            using Utf8 name = new Utf8(functionName);
            return Api.FindFunction(type.Bytes, name.Bytes);
        }

        public static void Call(ref IntPtr function, string className, string functionName, IntPtr self, void** args, void* result)
        {
            if (function == IntPtr.Zero)
            {
                function = FindFunction(className, functionName);
            }
            if (Api.CallFunction(function, self, args, result) == 0)
            {
                throw new InvalidOperationException($"{className}.{functionName} could not be called");
            }
        }

        public static void Log(string text, int level = 1)
        {
            using Utf8 utf8 = new Utf8(text);
            Api.Log(level, utf8.Bytes);
        }

        internal static string GetClassName(IntPtr handle) => ReadString((IntPtr)Api.ClassNameOf(handle));

        internal static string GetParentClassName(string className)
        {
            using Utf8 name = new Utf8(className);
            return ReadString((IntPtr)Api.ParentClassNameOf(name.Bytes));
        }
    }
}
