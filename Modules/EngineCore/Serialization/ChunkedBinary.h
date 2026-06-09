#pragma once

#include "Object/Object.h"
#include "Object/Pointer.h"
#include "Common/ByteString.h"
#include "Common/Types.h"
#include "ChunkedBinary.gen.h"

/**
 * ChunkReader: Read interface for a single chunk from a ChunkedBinary file.
 * Provides a non-owning view into chunk data with its own cursor position.
 */
class ChunkReader {
public:
    ChunkReader();
    explicit ChunkReader(SharedObjectPtr<ByteString> InData);

    size_t Tell() const;
    void Seek(size_t InOffset);

    bool IsAtEnd() const;

    void ReadBytes(void* OutData, size_t InSize);

    size_t GetSize() const;
    size_t GetRemaining() const;

    SharedObjectPtr<ByteString> GetByteString() const;

    template<typename T>
    ChunkReader& operator>>(T& OutValue);

    ChunkReader& operator>>(String& OutValue);

private:
    SharedObjectPtr<ByteString> m_Data;
    size_t m_Cursor = 0;
};

/**
 * ChunkWriter: Write interface for building chunks in memory.
 * Accumulates data to be written as a single chunk.
 */
class ChunkWriter {
public:
    ChunkWriter();

    size_t Tell() const;

    void WriteBytes(const void* InData, size_t InSize);

    template<typename T>
    ChunkWriter& operator<<(const T& InValue);

    ChunkWriter& operator<<(const String& InValue);

    SharedObjectPtr<ByteString> GetData() const;

private:
    Array<byte> m_Buffer;
};

/**
 * ChunkedBinary: General-purpose chunked binary format for assets, caches, shaders, etc.
 * 
 * Supports:
 * - Lazy loading: Only chunk table loaded at startup (2 KB header for 2 GB texture pack)
 * - Region reads: Load specific chunk regions from disk on demand
 * - Multiple open chunks: No global cursor state
 * - mmap-friendly architecture
 * - Compression/encryption ready
 * 
 * Reading:
 *     ChunkedBinary file(path);
 *     ChunkReader chunk = file.GetChunk(ChunkId);
 *     chunk >> value;
 * 
 * Writing:
 *     ChunkedBinary file;
 *     ChunkWriter chunk;
 *     chunk << value;
 *     file.AddChunk(ChunkId, chunk);
 *     file.SaveToFile(path);
 */
class ChunkedBinary : public Object {
public:
    ARTIFACT_CLASS();

    static constexpr uint32_t Magic = 0x48435542;  // "UBCH" = UnsortedBinaryChunked
    static constexpr uint16_t Version = 1;

    struct Header {
        uint32_t MagicValue = ChunkedBinary::Magic;
        uint16_t VersionValue = ChunkedBinary::Version;
        uint16_t ChunkCount = 0;
    };

    struct Chunk {
        uint32_t Id = 0;
        uint64_t Offset = 0;
        uint64_t Size = 0;
    };

    struct ChunkData {
        Chunk Metadata;
        SharedObjectPtr<ByteString> Data;
    };

public:
    ChunkedBinary();
    
    // Loading from disk (lazy)
    static SharedObjectPtr<ChunkedBinary> LoadFromFile(const String& InPath);

    // Saving to disk (memory-based)
    bool SaveToFile(const String& InPath) const;

    // Chunk operations
    ChunkReader GetChunk(uint32_t InChunkId) const;
    bool HasChunk(uint32_t InChunkId) const;
    const Chunk* FindChunk(uint32_t InChunkId) const;

    // Writing interface
    void AddChunk(uint32_t InChunkId, const ChunkWriter& InWriter);
    void Clear();

    // Metadata
    size_t GetChunkCount() const;
    String GetFilePath() const;

private:
    void ParseChunkTable();

private:
    String m_FilePath;
    Array<Chunk> m_Chunks;
    Array<ChunkData> m_ChunkDataForWriting;  // Only used when building a file for writing
    bool m_IsLoaded = false;  // True when loaded from disk, false when building for writing
};

// ChunkReader template implementations
template<typename T>
ChunkReader& ChunkReader::operator>>(T& OutValue) {
    static_assert(std::is_trivially_copyable_v<T>);
    ReadBytes(&OutValue, sizeof(T));
    return *this;
}

// ChunkWriter template implementations
template<typename T>
ChunkWriter& ChunkWriter::operator<<(const T& InValue) {
    static_assert(std::is_trivially_copyable_v<T>);
    WriteBytes(&InValue, sizeof(T));
    return *this;
}
