using System;

namespace Artifact
{
    public readonly struct Class : IEquatable<Class>
    {
        public static readonly Class None = new Class(string.Empty);

        readonly string m_Name;

        public Class(string name) => m_Name = name ?? string.Empty;

        public string Name => m_Name ?? string.Empty;

        public bool IsValid => Name.Length != 0;

        public Class Parent => IsValid ? new Class(Native.GetParentClassName(Name)) : None;

        public Type ManagedType => IsValid ? Host.FindManagedType(Name) : null;

        public static Class Of<T>() where T : Object => new Class(typeof(T).Name);

        public static Class Of(Type type) => type == null ? None : new Class(type.Name);

        public bool IsSubclassOf(Class other)
        {
            if (!other.IsValid)
            {
                return false;
            }
            for (Class current = this; current.IsValid; current = current.Parent)
            {
                if (current == other)
                {
                    return true;
                }
            }
            return false;
        }

        public static implicit operator Class(Type type) => Of(type);

        public bool Equals(Class other) => Name == other.Name;
        public override bool Equals(object obj) => obj is Class other && Equals(other);
        public override int GetHashCode() => Name.GetHashCode();
        public override string ToString() => Name;

        public static bool operator ==(Class a, Class b) => a.Equals(b);
        public static bool operator !=(Class a, Class b) => !a.Equals(b);
    }
}
