#include "fmt/leaf/leafpack_group/lfb_image_decoder.h"
#include "fmt/leaf/common/custom_lzss.h"
#include "fmt/microsoft/bmp_image_decoder.h"

using namespace au;
using namespace au::fmt::leaf;

bool LfbImageDecoder::is_recognized_impl(File &input_file) const
{
    return input_file.has_extension("lfb");
}

pix::Grid LfbImageDecoder::decode_impl(File &input_file) const
{
    input_file.stream.seek(0);
    const auto size_orig = input_file.stream.read_u32_le();
    const auto data = common::custom_lzss_decompress(
        input_file.stream.read_to_eof(), size_orig);
    const auto pseudo_file = std::make_unique<File>(input_file.name, data);
    return fmt::microsoft::BmpImageDecoder().decode(*pseudo_file);
}

static auto dummy = fmt::register_fmt<LfbImageDecoder>("leaf/lfb");