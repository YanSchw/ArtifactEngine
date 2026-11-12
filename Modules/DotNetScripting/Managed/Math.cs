using System;
using System.Runtime.InteropServices;

namespace Artifact
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Vec2
    {
        public float X, Y;

        public Vec2(float x, float y) { X = x; Y = y; }

        public static Vec2 operator +(Vec2 a, Vec2 b) => new Vec2(a.X + b.X, a.Y + b.Y);
        public static Vec2 operator -(Vec2 a, Vec2 b) => new Vec2(a.X - b.X, a.Y - b.Y);
        public static Vec2 operator *(Vec2 a, float s) => new Vec2(a.X * s, a.Y * s);
        public static Vec2 operator -(Vec2 a) => new Vec2(-a.X, -a.Y);

        public float Length() => MathF.Sqrt(X * X + Y * Y);
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Vec3
    {
        public float X, Y, Z;

        public Vec3(float x, float y, float z) { X = x; Y = y; Z = z; }

        public static Vec3 operator +(Vec3 a, Vec3 b) => new Vec3(a.X + b.X, a.Y + b.Y, a.Z + b.Z);
        public static Vec3 operator -(Vec3 a, Vec3 b) => new Vec3(a.X - b.X, a.Y - b.Y, a.Z - b.Z);
        public static Vec3 operator *(Vec3 a, float s) => new Vec3(a.X * s, a.Y * s, a.Z * s);
        public static Vec3 operator -(Vec3 a) => new Vec3(-a.X, -a.Y, -a.Z);

        public float Length() => MathF.Sqrt(X * X + Y * Y + Z * Z);
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Vec4
    {
        public float X, Y, Z, W;

        public Vec4(float x, float y, float z, float w) { X = x; Y = y; Z = z; W = w; }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Color
    {
        public float R, G, B, A;

        public Color(float r, float g, float b, float a) { R = r; G = g; B = b; A = a; }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Quat
    {
        public float X, Y, Z, W;
    }
}
