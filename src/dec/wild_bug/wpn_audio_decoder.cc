#include "dec/wild_bug/wpn_audio_decoder.h"
#include <map>
#include "algo/range.h"

using namespace au;
using namespace au::dec::wild_bug;

static const bstr magic = "WBD\x1AWAV\x00"_b;
static const bstr fmt_magic = "fmt\x20"_b;
static const bstr data_magic = "data"_b;

namespace
{
    struct Chunk final
    {
        size_t offset;
        size_t size;
    };
}

bool WpnAudioDecoder::is_recognized_impl(io::File &input_file) const
{
    return input_file.stream.read(magic.size()) == magic;
}

res::Audio WpnAudioDecoder::decode_impl(
    const Logger &logger, io::File &input_file) const
{
    input_file.stream.seek(magic.size());
    const auto chunk_count = input_file.stream.read_le<u32>();
    std::map<bstr, Chunk> chunks;
    for (const auto i : algo::range(chunk_count))
    {
        const auto chunk_name = input_file.stream.read(4);
        Chunk chunk;
        chunk.offset = input_file.stream.read_le<u32>();
        chunk.size = input_file.stream.read_le<u32>();
        chunks[chunk_name] = chunk;
    }

    res::Audio audio;

    const auto &fmt_chunk = chunks.at(fmt_magic);
    input_file.stream.seek(fmt_chunk.offset);
    audio.codec = input_file.stream.read_le<u16>();
    audio.channel_count = input_file.stream.read_le<u16>();
    audio.sample_rate = input_file.stream.read_le<u32>();
    const auto byte_rate = input_file.stream.read_le<u32>();
    const auto block_align = input_file.stream.read_le<u16>();
    audio.bits_per_sample = input_file.stream.read_le<u16>();
    if (input_file.stream.tell() - fmt_chunk.offset < fmt_chunk.size)
    {
        const auto extra_data_size = input_file.stream.read_le<u16>();
        audio.extra_codec_headers = input_file.stream.read(extra_data_size);
    }

    const auto &data_chunk = chunks.at(data_magic);
    input_file.stream.seek(data_chunk.offset);
    audio.samples = input_file.stream.read(data_chunk.size);

    return audio;
}

static auto _ = dec::register_decoder<WpnAudioDecoder>("wild-bug/wpn");
