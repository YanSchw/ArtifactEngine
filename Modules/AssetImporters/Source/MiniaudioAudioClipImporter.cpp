#include "MiniaudioAudioClipImporter.h"
#include "MiniaudioLibrary.h"

bool MiniaudioAudioClipImporter::CanImport(const String& InExtension) const {
    static const Array<String> s_Extensions = { ".wav", ".mp3", ".flac", ".ogg" };
    return s_Extensions.Contains(InExtension);
}

static bool OpenDecoder(const String& InFilePath, ma_decoder& OutDecoder) {
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);
    if (ma_decoder_init_file(InFilePath.c_str(), &config, &OutDecoder) != MA_SUCCESS) {
        return false;
    }
    const ma_uint32 channels = OutDecoder.outputChannels;
    if (channels <= 2) {
        return true;
    }

    ma_decoder_uninit(&OutDecoder);
    Array<ma_channel> channelMap((size_t)channels);
    ma_channel_map_init_standard(ma_standard_channel_map_microsoft, channelMap.Data(), channels, channels);
    config.channels = channels;
    config.pChannelMap = channelMap.Data();
    return ma_decoder_init_file(InFilePath.c_str(), &config, &OutDecoder) == MA_SUCCESS;
}

bool MiniaudioAudioClipImporter::Import(const String& InFilePath, AudioSampleBuffer& OutSamples) {
    ma_decoder decoder;
    if (!OpenDecoder(InFilePath, decoder)) {
        return false;
    }

    const ma_uint32 channels = decoder.outputChannels;
    const ma_uint32 sampleRate = decoder.outputSampleRate;
    constexpr ma_uint64 s_ChunkFrames = 4096;

    std::vector<float> samples;
    ma_uint64 lengthInFrames = 0;
    if (ma_decoder_get_length_in_pcm_frames(&decoder, &lengthInFrames) == MA_SUCCESS) {
        samples.reserve((size_t)(lengthInFrames * channels));
    }

    Array<float> chunk((size_t)(s_ChunkFrames * channels));
    ma_uint64 framesRead = s_ChunkFrames;
    while (framesRead == s_ChunkFrames) {
        framesRead = 0;
        const ma_result result = ma_decoder_read_pcm_frames(&decoder, chunk.Data(), s_ChunkFrames, &framesRead);
        samples.insert(samples.end(), chunk.begin(), chunk.begin() + (std::ptrdiff_t)(framesRead * channels));
        if (result != MA_SUCCESS) {
            break;
        }
    }
    ma_decoder_uninit(&decoder);

    if (channels == 0 || sampleRate == 0 || samples.empty()) {
        return false;
    }
    OutSamples = AudioSampleBuffer::FromInterleaved(samples.data(), samples.size() / channels, channels, sampleRate);
    return true;
}
