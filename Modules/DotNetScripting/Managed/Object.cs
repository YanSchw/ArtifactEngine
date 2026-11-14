using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;

namespace Artifact
{
    public unsafe class Object
    {
        [ThreadStatic] static IntPtr s_Adopting;
        [ThreadStatic] static Type s_AdoptingType;
        [ThreadStatic] internal static Object s_Constructing;
        static readonly Dictionary<IntPtr, Object> s_Objects = new Dictionary<IntPtr, Object>();

        public IntPtr Handle { get; private set; }
        IntPtr m_Weak;

        protected unsafe Object()
        {
            IntPtr handle = IntPtr.Zero;
            if (s_AdoptingType == GetType())
            {
                handle = s_Adopting;
                s_Adopting = IntPtr.Zero;
                s_AdoptingType = null;
            }
            if (handle == IntPtr.Zero)
            {
                using Utf8 className = new Utf8(GetType().Name);
                s_Constructing = this;
                handle = Native.Api.CreateObject(className.Bytes);
                s_Constructing = null;
            }
            Bind(handle);
        }

        public bool IsValid => Handle != IntPtr.Zero && Native.Api.IsAlive(m_Weak) != 0;

        public static Class StaticClass() => new Class("Object");

        public Class GetClass() => IsValid ? new Class(Native.GetClassName(Handle)) : Class.None;

        public void Retain() => Native.Api.Retain(Handle);

        public void Release() => Native.Api.Release(Handle);

        void Bind(IntPtr handle)
        {
            Handle = handle;
            m_Weak = Native.Api.NewWeak(handle);
            s_Objects[handle] = this;
        }

        internal static Object Adopt(Type type, IntPtr handle)
        {
            s_Adopting = handle;
            s_AdoptingType = type;
            return (Object)Activator.CreateInstance(type, true);
        }

        internal static void Forget(IntPtr handle)
        {
            if (s_Objects.Remove(handle, out Object instance))
            {
                Native.Api.DeleteWeak(instance.m_Weak);
                instance.m_Weak = IntPtr.Zero;
                instance.Handle = IntPtr.Zero;
            }
        }

        internal static void ForgetAll()
        {
            foreach (Object instance in s_Objects.Values)
            {
                Native.Api.DeleteWeak(instance.m_Weak);
                instance.m_Weak = IntPtr.Zero;
                instance.Handle = IntPtr.Zero;
            }
            s_Objects.Clear();
            s_Adopting = IntPtr.Zero;
            s_AdoptingType = null;
            s_Constructing = null;
        }

        internal static Object Find(IntPtr handle) => s_Objects.TryGetValue(handle, out Object instance) ? instance : null;

        public static T FromHandle<T>(IntPtr handle) where T : Object
        {
            if (handle == IntPtr.Zero)
            {
                return null;
            }
            if (s_Objects.TryGetValue(handle, out Object existing))
            {
                if (existing.IsValid)
                {
                    return existing as T;
                }
                Forget(handle);
            }
            Object wrapper = (Object)RuntimeHelpers.GetUninitializedObject(Host.FindManagedType(Native.GetClassName(handle)));
            wrapper.Bind(handle);
            return wrapper as T;
        }
    }
}
