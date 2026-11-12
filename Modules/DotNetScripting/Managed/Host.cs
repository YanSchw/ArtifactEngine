using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Loader;

namespace Artifact
{
    public static unsafe class Host
    {
        sealed class ScriptField
        {
            public FieldInfo Field;
            public FieldKind Kind;
            public uint Offset;
        }

        sealed class ScriptType
        {
            public Type Type;
            public List<ScriptField> Fields = new List<ScriptField>();
        }

        sealed class ScriptState
        {
            public Object Instance;
            public ScriptType Type;
            public IntPtr Storage;
            public object[] Shadow;
        }

        readonly record struct Reference(IntPtr Handle);

        sealed class ScriptContext : AssemblyLoadContext
        {
            readonly Dictionary<string, string> m_Paths;

            public ScriptContext(IEnumerable<string> paths) : base("Scripts", true)
            {
                m_Paths = paths.ToDictionary(path => Path.GetFileNameWithoutExtension(path));
            }

            public IEnumerable<Assembly> LoadAll() => m_Paths.Keys.Select(name => LoadFromAssemblyName(new AssemblyName(name))).ToList();

            protected override Assembly Load(AssemblyName name)
            {
                if (!m_Paths.TryGetValue(name.Name, out string path))
                {
                    return null;
                }
                string symbols = Path.ChangeExtension(path, ".pdb");
                using MemoryStream assembly = new MemoryStream(File.ReadAllBytes(path));
                using MemoryStream pdb = File.Exists(symbols) ? new MemoryStream(File.ReadAllBytes(symbols)) : null;
                return LoadFromStream(assembly, pdb);
            }
        }

        enum LifecycleCall { BeginPlay, EndPlay, WorldUpdate }

        static readonly Dictionary<string, ScriptType> s_Types = new Dictionary<string, ScriptType>();
        static readonly Dictionary<IntPtr, ScriptState> s_States = new Dictionary<IntPtr, ScriptState>();
        static readonly Dictionary<IntPtr, Dictionary<string, object>> s_Preserved = new Dictionary<IntPtr, Dictionary<string, object>>();
        static ScriptContext s_Context;

        [UnmanagedCallersOnly]
        public static int Initialize(NativeApi* api)
        {
            Native.Api = *api;
            CultureInfo.DefaultThreadCurrentCulture = CultureInfo.InvariantCulture;
            CultureInfo.CurrentCulture = CultureInfo.InvariantCulture;
            return 1;
        }

        [UnmanagedCallersOnly]
        public static int Load(byte** paths, int count)
        {
            try
            {
                List<string> files = Enumerable.Range(0, count).Select(i => Native.ReadString((IntPtr)paths[i])).Where(File.Exists).ToList();
                s_Context = new ScriptContext(files);
                List<Type> types = s_Context.LoadAll().SelectMany(assembly => assembly.GetTypes())
                    .Where(type => type.IsClass && typeof(Object).IsAssignableFrom(type)).ToList();
                foreach (Type type in types.OrderBy(Depth))
                {
                    Register(type);
                }
                return 1;
            }
            catch (Exception exception)
            {
                Native.Log(exception.ToString(), 3);
                return 0;
            }
        }

        [UnmanagedCallersOnly]
        public static void Unload()
        {
            Guard(PreserveAll);
            if (!UnloadContext())
            {
                Native.Log("The previous script assemblies are still referenced and stay loaded", 2);
            }
        }

        [UnmanagedCallersOnly]
        public static void Restore()
        {
            Guard(() =>
            {
                foreach ((IntPtr handle, ScriptState state) in s_States)
                {
                    SyncIn(state);
                    if (s_Preserved.TryGetValue(handle, out Dictionary<string, object> values))
                    {
                        Apply(state.Instance, values);
                    }
                }
                s_Preserved.Clear();
            });
        }

        [UnmanagedCallersOnly]
        public static void CreateInstance(byte* className, IntPtr handle, IntPtr storage)
        {
            Guard(() =>
            {
                ScriptType type = s_Types[Native.ReadString((IntPtr)className)];
                ScriptState state = new ScriptState { Type = type, Storage = storage, Shadow = new object[type.Fields.Count] };
                s_States[handle] = state;
                state.Instance = Object.s_Constructing ?? Object.Adopt(type.Type, handle);
                SyncOut(state);
            });
        }

        [UnmanagedCallersOnly]
        public static void Call(IntPtr handle, int call, float deltaTime)
        {
            Guard(() =>
            {
                if (!s_States.TryGetValue(handle, out ScriptState state) || state.Instance is not Node node)
                {
                    return;
                }
                SyncIn(state);
                switch ((LifecycleCall)call)
                {
                    case LifecycleCall.BeginPlay: node.BeginPlay(); break;
                    case LifecycleCall.EndPlay: node.EndPlay(); break;
                    case LifecycleCall.WorldUpdate: node.WorldUpdate(deltaTime); break;
                }
                SyncOut(state);
            });
        }

        [UnmanagedCallersOnly]
        public static void Destroyed(IntPtr handle)
        {
            s_States.Remove(handle);
            Object.Forget(handle);
        }

        internal static Type FindManagedType(string className)
        {
            for (string name = className; !string.IsNullOrEmpty(name); name = Native.GetParentClassName(name))
            {
                if (s_Types.TryGetValue(name, out ScriptType script))
                {
                    return script.Type;
                }
                Type binding = typeof(Object).Assembly.GetType("Artifact." + name);
                if (binding != null)
                {
                    return binding;
                }
            }
            return typeof(Object);
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        static void PreserveAll()
        {
            foreach ((IntPtr handle, ScriptState state) in s_States)
            {
                s_Preserved[handle] = Preserve(state.Instance);
            }
            s_States.Clear();
            s_Types.Clear();
            Object.ForgetAll();
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        static WeakReference ReleaseContext()
        {
            WeakReference context = new WeakReference(s_Context);
            s_Context.Unload();
            s_Context = null;
            return context;
        }

        static bool UnloadContext()
        {
            if (s_Context == null)
            {
                return true;
            }
            WeakReference context = ReleaseContext();
            for (int i = 0; i < 10 && context.IsAlive; i++)
            {
                GC.Collect();
                GC.WaitForPendingFinalizers();
            }
            return !context.IsAlive;
        }

        static IEnumerable<FieldInfo> ScriptFields(Type type)
        {
            for (Type current = type; current != null && current.Assembly != typeof(Object).Assembly; current = current.BaseType)
            {
                foreach (FieldInfo field in current.GetFields(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.DeclaredOnly))
                {
                    if (field.GetCustomAttribute<PropertyAttribute>() == null)
                    {
                        yield return field;
                    }
                }
            }
        }

        static string Key(FieldInfo field) => $"{field.DeclaringType.FullName}:{field.Name}";

        static Dictionary<string, object> Preserve(Object instance)
        {
            Dictionary<string, object> values = new Dictionary<string, object>();
            foreach (FieldInfo field in ScriptFields(instance.GetType()))
            {
                Type type = field.FieldType;
                object value = field.GetValue(instance);
                if (typeof(Object).IsAssignableFrom(type))
                {
                    values[Key(field)] = value is Object reference && reference.IsValid ? new Reference(reference.Handle) : null;
                }
                else if (!type.IsCollectible && (type.IsPrimitive || type.IsEnum || type == typeof(string) || (type.IsValueType && type.Assembly == typeof(Object).Assembly)))
                {
                    values[Key(field)] = value;
                }
            }
            return values;
        }

        static void Apply(Object instance, Dictionary<string, object> values)
        {
            foreach (FieldInfo field in ScriptFields(instance.GetType()))
            {
                if (!values.TryGetValue(Key(field), out object value))
                {
                    continue;
                }
                if (value is Reference reference)
                {
                    value = Object.FromHandle<Object>(reference.Handle);
                }
                if (value == null ? !field.FieldType.IsValueType : field.FieldType.IsInstanceOfType(value))
                {
                    field.SetValue(instance, value);
                }
            }
        }

        static int Depth(Type type)
        {
            int depth = 0;
            for (Type current = type; current != null; current = current.BaseType)
            {
                depth++;
            }
            return depth;
        }

        static void Register(Type type)
        {
            List<FieldInfo> fields = type.GetFields(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.DeclaredOnly)
                .Where(field => field.GetCustomAttribute<PropertyAttribute>() != null).ToList();
            List<Utf8> names = fields.Select(field => new Utf8(field.Name)).ToList();
            ScriptFieldDesc* descs = stackalloc ScriptFieldDesc[Math.Max(1, fields.Count)];
            for (int i = 0; i < fields.Count; i++)
            {
                descs[i] = new ScriptFieldDesc { Name = names[i].Bytes, Kind = (int)KindOf(fields[i]) };
            }

            using Utf8 name = new Utf8(type.Name);
            using Utf8 parent = new Utf8(type.BaseType.Name);
            using Utf8 module = new Utf8(type.Assembly.GetName().Name);
            ScriptClassDesc desc = new ScriptClassDesc
            {
                Name = name.Bytes,
                Parent = parent.Bytes,
                Module = module.Bytes,
                Ticks = (byte)(Overrides(type, "WorldUpdate") ? 1 : 0),
                Fields = descs,
                FieldCount = fields.Count,
            };
            bool registered = Native.Api.RegisterScriptClass(&desc) != 0;
            names.ForEach(utf8 => utf8.Dispose());
            if (!registered)
            {
                Native.Log($"Script class {type.Name} could not be registered", 3);
                return;
            }

            ScriptType script = new ScriptType { Type = type };
            if (s_Types.TryGetValue(type.BaseType.Name, out ScriptType parentScript))
            {
                script.Fields.AddRange(parentScript.Fields);
            }
            for (int i = 0; i < fields.Count; i++)
            {
                script.Fields.Add(new ScriptField { Field = fields[i], Kind = (FieldKind)descs[i].Kind, Offset = descs[i].Offset });
            }
            s_Types[type.Name] = script;
        }

        static bool Overrides(Type type, string method)
        {
            MethodInfo info = type.GetMethod(method);
            return info != null && info.DeclaringType != typeof(Node) && typeof(Node).IsAssignableFrom(type);
        }

        static FieldKind KindOf(FieldInfo field)
        {
            Type type = field.FieldType;
            if (type == typeof(float)) return FieldKind.Float;
            if (type == typeof(double)) return FieldKind.Double;
            if (type == typeof(int)) return FieldKind.Int32;
            if (type == typeof(bool)) return FieldKind.Bool;
            if (type == typeof(string)) return FieldKind.String;
            if (type == typeof(Vec2)) return FieldKind.Vec2;
            if (type == typeof(Vec3)) return FieldKind.Vec3;
            if (type == typeof(Vec4)) return FieldKind.Vec4;
            if (type == typeof(Color)) return FieldKind.Color;
            throw new NotSupportedException($"[Property] {field.DeclaringType.Name}.{field.Name} has unsupported type {type.Name}");
        }

        static object Read(ScriptField field, IntPtr storage)
        {
            void* data = (byte*)storage + field.Offset;
            return field.Kind switch
            {
                FieldKind.Float => *(float*)data,
                FieldKind.Double => *(double*)data,
                FieldKind.Int32 => *(int*)data,
                FieldKind.Bool => *(byte*)data != 0,
                FieldKind.String => Native.ReadString((IntPtr)Native.Api.GetString((IntPtr)data)),
                FieldKind.Vec2 => *(Vec2*)data,
                FieldKind.Vec3 => *(Vec3*)data,
                FieldKind.Vec4 => *(Vec4*)data,
                FieldKind.Color => *(Color*)data,
                _ => null,
            };
        }

        static void Write(ScriptField field, IntPtr storage, object value)
        {
            void* data = (byte*)storage + field.Offset;
            switch (field.Kind)
            {
                case FieldKind.Float: *(float*)data = (float)value; break;
                case FieldKind.Double: *(double*)data = (double)value; break;
                case FieldKind.Int32: *(int*)data = (int)value; break;
                case FieldKind.Bool: *(byte*)data = (byte)((bool)value ? 1 : 0); break;
                case FieldKind.String:
                    using (Utf8 utf8 = new Utf8((string)value)) { Native.Api.SetString((IntPtr)data, utf8.Bytes); }
                    break;
                case FieldKind.Vec2: *(Vec2*)data = (Vec2)value; break;
                case FieldKind.Vec3: *(Vec3*)data = (Vec3)value; break;
                case FieldKind.Vec4: *(Vec4*)data = (Vec4)value; break;
                case FieldKind.Color: *(Color*)data = (Color)value; break;
            }
        }

        static void SyncIn(ScriptState state)
        {
            for (int i = 0; i < state.Type.Fields.Count; i++)
            {
                ScriptField field = state.Type.Fields[i];
                object native = Read(field, state.Storage);
                if (!Equals(native, state.Shadow[i]))
                {
                    field.Field.SetValue(state.Instance, native);
                    state.Shadow[i] = native;
                }
            }
        }

        static void SyncOut(ScriptState state)
        {
            for (int i = 0; i < state.Type.Fields.Count; i++)
            {
                ScriptField field = state.Type.Fields[i];
                object value = field.Field.GetValue(state.Instance) ?? (field.Kind == FieldKind.String ? string.Empty : null);
                Write(field, state.Storage, value);
                state.Shadow[i] = value;
            }
        }

        static void Guard(Action action)
        {
            try
            {
                action();
            }
            catch (Exception exception)
            {
                Native.Log(exception.ToString(), 3);
            }
        }
    }
}
