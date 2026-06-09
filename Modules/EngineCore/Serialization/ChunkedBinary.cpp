#include "ChunkedBinary.h"

#include "Platform/FileIO.h"

#include <cstring>

// ============================================================================
// ChunkReader Implementation
// ============================================================================

ChunkReader::ChunkReader() {
}

ChunkReader::ChunkReader(SharedObjectPtr<ByteString> InData)
    : m_Data(InData) {
}

size_t ChunkReader::Tell() const {
    return m_Cursor;
}

void ChunkReader::Seek(size_t InOffset) {
    if (!m_Data)
        return;

    AE_ASSERT(
        InOffset <= m_Data->GetSizeInBytes(),
        "Invalid seek");

    m_Cursor = InOffset;
}

bool ChunkReader::IsAtEnd() const {
    if (!m_Data)
        return true;

    return m_Cursor >= m_Data->GetSizeInBytes();
}

void ChunkReader::ReadBytes(void* OutData, size_t InSize) {
    AE_ASSERT(m_Data, "No data in chunk");
    AE_ASSERT(
        m_Cursor + InSize <= m_Data->GetSizeInBytes(),
        "Read outside chunk");

    memcpy(
        OutData,
        m_Data->GetData() + m_Cursor,
        InSize);

    m_Cursor += InSize;
}

size_t ChunkReader::GetSize() const {
    if (!m_Data)
        return 0;

    return m_Data->GetSizeInBytes();
}

size_t ChunkReader::GetRemaining() const {
    if (!m_Data)
        return 0;

    return m_Data->GetSizeInBytes() - m_Cursor;
}

SharedObjectPtr<ByteString> ChunkReader::GetByteString() const {
    return m_Data;
}

ChunkReader& ChunkReader::operator>>(String& OutValue) {
    uint64_t len;

    *this >> len;

    OutValue.resize((size_t)len);

    if (len > 0)
        ReadBytes(OutValue.data(), len);

    return *this;
}

// ============================================================================
// ChunkWriter Implementation
// ============================================================================

ChunkWriter::ChunkWriter() {
}

size_t ChunkWriter::Tell() const {
    return m_Buffer.Size();
}

void ChunkWriter::WriteBytes(const void* InData, size_t InSize) {
    size_t currentSize = m_Buffer.Size();

    m_Buffer.Resize(currentSize + InSize);

    memcpy(
        m_Buffer.Data() + currentSize,
        InData,
        InSize);
}

ChunkWriter& ChunkWriter::operator<<(const String& InValue) {
    uint64_t len = (uint64_t)InValue.size();

    *this << len;

    if (len > 0)
        WriteBytes(InValue.data(), len);

    return *this;
}

SharedObjectPtr<ByteString> ChunkWriter::GetData() const {
    byte* data = new byte[m_Buffer.Size()];

    memcpy(data, m_Buffer.Data(), m_Buffer.Size());

    return new ByteString(m_Buffer.Size(), data);
}

// ============================================================================
// ChunkedBinary Implementation
// ============================================================================

ChunkedBinary::ChunkedBinary() {
}

SharedObjectPtr<ChunkedBinary> ChunkedBinary::LoadFromFile(const String& InPath) {
    if (!FileIO::FileExists(InPath))
        return nullptr;

    // Load only the header to get chunk count
    uint64_t fileSize = FileIO::GetFileSize(InPath);

    if (fileSize < sizeof(Header))
        return nullptr;

    SharedObjectPtr<ByteString> headerData =
        FileIO::ReadFileRegion(InPath, 0, sizeof(Header));

    if (!headerData)
        return nullptr;

    Header header;
    memcpy(&header, headerData->GetData(), sizeof(Header));

    if (header.MagicValue != Magic) {
        AE_ASSERT(false, "Invalid chunked binary file");
        return nullptr;
    }

    if (header.VersionValue != Version) {
        AE_ASSERT(false, "Unsupported chunked binary version");
        return nullptr;
    }

    // Load chunk table
    uint64_t tableSize =
        sizeof(Header) +
        sizeof(Chunk) * header.ChunkCount;

    if (fileSize < tableSize) {
        AE_ASSERT(false, "Corrupted chunked binary file");
        return nullptr;
    }

    SharedObjectPtr<ByteString> tableData =
        FileIO::ReadFileRegion(InPath, 0, tableSize);

    if (!tableData)
        return nullptr;

    SharedObjectPtr<ChunkedBinary> result = new ChunkedBinary();
    result->m_FilePath = InPath;
    result->m_IsLoaded = true;

    // Parse chunk table
    size_t offset = sizeof(Header);
    result->m_Chunks.Resize(header.ChunkCount);

    for (uint32_t i = 0; i < header.ChunkCount; i++) {
        memcpy(
            &result->m_Chunks[i],
            tableData->GetData() + offset,
            sizeof(Chunk));

        offset += sizeof(Chunk);
    }

    return result;
}

bool ChunkedBinary::SaveToFile(const String& InPath) const {
    Header header;
    header.ChunkCount = (uint16_t)m_ChunkDataForWriting.Size();

    uint64_t tableSize =
        sizeof(Header) +
        sizeof(Chunk) * m_ChunkDataForWriting.Size();

    // First pass: calculate payload offsets and build final chunks
    Array<Chunk> finalChunks;
    uint64_t payloadOffset = tableSize;

    for (const ChunkData& chunkData : m_ChunkDataForWriting) {
        Chunk finalChunk = chunkData.Metadata;
        finalChunk.Offset = payloadOffset;
        finalChunk.Size = chunkData.Data->GetSizeInBytes();

        finalChunks.Add(finalChunk);
        payloadOffset += finalChunk.Size;
    }

    // Build file data in memory
    Array<byte> fileData;
    fileData.Resize(payloadOffset);

    byte* dst = fileData.Data();

    // Write header
    memcpy(dst, &header, sizeof(Header));
    dst += sizeof(Header);

    // Write chunk table
    for (const Chunk& chunk : finalChunks) {
        memcpy(dst, &chunk, sizeof(Chunk));
        dst += sizeof(Chunk);
    }

    // Write chunk payloads
    for (const ChunkData& chunkData : m_ChunkDataForWriting) {
        memcpy(dst, chunkData.Data->GetData(), chunkData.Data->GetSizeInBytes());
        dst += chunkData.Data->GetSizeInBytes();
    }

    return FileIO::WriteBytesToFile(InPath, fileData.Data(), fileData.Size());
}

ChunkReader ChunkedBinary::GetChunk(uint32_t InChunkId) const {
    // If we're in write mode, return from in-memory data
    if (!m_IsLoaded) {
        for (const ChunkData& chunkData : m_ChunkDataForWriting) {
            if (chunkData.Metadata.Id == InChunkId) {
                return ChunkReader(chunkData.Data);
            }
        }

        AE_ASSERT(false, "Chunk not found");
        return ChunkReader();
    }

    // If we're in read mode, load from disk
    const Chunk* chunk = FindChunk(InChunkId);

    if (!chunk) {
        AE_ASSERT(false, "Chunk not found");
        return ChunkReader();
    }

    // Load only this specific chunk region from disk
    SharedObjectPtr<ByteString> data =
        FileIO::ReadFileRegion(
            m_FilePath,
            chunk->Offset,
            chunk->Size);

    return ChunkReader(data);
}

bool ChunkedBinary::HasChunk(uint32_t InChunkId) const {
    return FindChunk(InChunkId) != nullptr;
}

const ChunkedBinary::Chunk* ChunkedBinary::FindChunk(uint32_t InChunkId) const {
    for (const Chunk& chunk : m_Chunks) {
        if (chunk.Id == InChunkId)
            return &chunk;
    }

    return nullptr;
}

void ChunkedBinary::AddChunk(uint32_t InChunkId, const ChunkWriter& InWriter) {
    SharedObjectPtr<ByteString> data = InWriter.GetData();

    ChunkData chunkData;
    chunkData.Metadata.Id = InChunkId;
    chunkData.Metadata.Offset = 0;  // Will be calculated during Save()
    chunkData.Metadata.Size = data->GetSizeInBytes();
    chunkData.Data = data;

    m_ChunkDataForWriting.Add(chunkData);
}

void ChunkedBinary::Clear() {
    m_Chunks.Clear();
    m_ChunkDataForWriting.Clear();
    m_FilePath.clear();
    m_IsLoaded = false;
}

size_t ChunkedBinary::GetChunkCount() const {
    if (m_IsLoaded) {
        return m_Chunks.Size();
    } else {
        return m_ChunkDataForWriting.Size();
    }
}

String ChunkedBinary::GetFilePath() const {
    return m_FilePath;
}

void ChunkedBinary::ParseChunkTable() {
    // This is called during LoadFromFile, already implemented there
}
