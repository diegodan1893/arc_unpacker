#include "dec/bgi/cbg_image_decoder.h"
#include "dec/bgi/cbg/cbg1_decoder.h"
#include "dec/bgi/cbg/cbg2_decoder.h"
#include "err.h"

using namespace au;
using namespace au::dec::bgi;

namespace
{
    enum class Version : u8
    {
        Version1 = 1,
        Version2 = 2,
    };
}

static const bstr magic = "CompressedBG___\x00"_b;

static Version get_version(io::BaseByteStream &input_stream)
{
    Version ret;
    input_stream.peek(46, [&]()
    {
        ret = input_stream.read_le<u16>() == 2
            ? Version::Version2
            : Version::Version1;
    });
    return ret;
}

bool CbgImageDecoder::is_recognized_impl(io::File &input_file) const
{
    return input_file.stream.read(magic.size()) == magic;
}

res::Image CbgImageDecoder::decode_impl(
    const Logger &logger, io::File &input_file) const
{
    input_file.stream.skip(magic.size());

    auto version = get_version(input_file.stream);
    if (version == Version::Version1)
        return *cbg::Cbg1Decoder().decode(input_file.stream);
    if (version == Version::Version2)
        return *cbg::Cbg2Decoder().decode(input_file.stream);
    throw err::UnsupportedVersionError(static_cast<int>(version));
}

static auto _ = dec::register_decoder<CbgImageDecoder>("bgi/cbg");
