#include "dec/crowd/cwd_image_decoder.h"
#include "algo/range.h"

using namespace au;
using namespace au::dec::crowd;

static const auto magic = "cwd format  - version 1.00 -"_b;

bool CwdImageDecoder::is_recognized_impl(io::File &input_file) const
{
    return input_file.stream.seek(0).read(magic.size()) == magic;
}

res::Image CwdImageDecoder::decode_impl(
    const Logger &logger, io::File &input_file) const
{
    const auto key = input_file.stream.seek(0x34).read<u8>() + 0x259A;
    const auto width = input_file.stream.seek(0x2C).read_le<u32>() + key;
    const auto height = input_file.stream.seek(0x30).read_le<u32>() + key;
    const auto data = input_file.stream.seek(0x38).read(width * height * 2);
    return res::Image(width, height, data, res::PixelFormat::BGR555X);
}

static auto _ = dec::register_decoder<CwdImageDecoder>("crowd/cwd");
