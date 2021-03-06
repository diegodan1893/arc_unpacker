﻿#include "dec/majiro/rc8_image_decoder.h"
#include "algo/range.h"
#include "algo/str.h"
#include "err.h"
#include "io/memory_stream.h"

using namespace au;
using namespace au::dec::majiro;

static const bstr magic = "\x98\x5A\x92\x9A\x38"_b; // sjis "六丁8"

static bstr uncompress(const bstr &input, size_t width, size_t height)
{
    io::MemoryStream input_stream(input);

    bstr output(width * height);
    auto output_ptr = output.get<u8>();
    auto output_start = output.get<const u8>();
    auto output_end = output.end<const u8>();

    std::vector<int> shift_table;
    for (auto i : algo::range(4))
        shift_table.push_back(-1 - i);
    for (auto i : algo::range(7))
        shift_table.push_back(3 - i - width);
    for (auto i : algo::range(5))
        shift_table.push_back(2 - i - width * 2);

    if (output.size() < 1)
        return output;
    *output_ptr++ = input_stream.read<u8>();

    while (output_ptr < output_end)
    {
        auto flag = input_stream.read<u8>();
        if (flag & 0x80)
        {
            auto size = flag & 7;
            auto look_behind = (flag >> 3) & 0x0F;
            size = size == 7
                ? input_stream.read_le<u16>() + 0x0A
                : size + 3;
            auto source_ptr = &output_ptr[shift_table[look_behind]];
            if (source_ptr < output_start || source_ptr + size >= output_end)
                return output;
            while (size-- && output_ptr < output_end)
                *output_ptr++ = *source_ptr++;
        }
        else
        {
            auto size = flag == 0x7F
                ? input_stream.read_le<u16>() + 0x80
                : flag + 1;
            while (size-- && output_ptr < output_end)
                *output_ptr++ = input_stream.read<u8>();
        }
    }

    return output;
}

bool Rc8ImageDecoder::is_recognized_impl(io::File &input_file) const
{
    return input_file.stream.read(magic.size()) == magic;
}

res::Image Rc8ImageDecoder::decode_impl(
    const Logger &logger, io::File &input_file) const
{
    input_file.stream.skip(magic.size());

    if (input_file.stream.read<u8>() != '_')
        throw err::NotSupportedError("Unexpected encryption flag");

    const auto version = algo::from_string<int>(
        input_file.stream.read(2).str());
    if (version != 0)
        throw err::UnsupportedVersionError(version);

    const auto width = input_file.stream.read_le<u32>();
    const auto height = input_file.stream.read_le<u32>();
    input_file.stream.skip(4);

    res::Palette palette(256, input_file.stream, res::PixelFormat::BGR888);
    const auto data_comp = input_file.stream.read_to_eof();
    const auto data_orig = uncompress(data_comp, width, height);
    return res::Image(width, height, data_orig, palette);
}

static auto _ = dec::register_decoder<Rc8ImageDecoder>("majiro/rc8");
