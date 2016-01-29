#include "dec/real_live/g00_image_decoder.h"
#include "algo/ptr.h"
#include "algo/range.h"
#include "io/memory_stream.h"

using namespace au;
using namespace au::dec::real_live;

namespace
{
    struct Region final
    {
        size_t x1, y1;
        size_t x2, y2;
        size_t ox, oy;
        size_t block_offset, block_size;
    };
}

static bstr decompress(
    const bstr &input,
    const size_t output_size,
    const size_t byte_count,
    const size_t size_delta)
{
    bstr output(output_size);

    auto output_ptr = algo::make_ptr(output);
    auto input_ptr = algo::make_ptr(input);

    u16 control = 1;
    while (output_ptr.left() && input_ptr.left())
    {
        control >>= 1;
        if (!(control & 0x100))
            control = *input_ptr++ | 0xFF00;

        if (control & 1)
        {
            output_ptr.append_from(input_ptr, byte_count);
        }
        else
        {
            if (input_ptr.left()
                < 2) break;
            size_t tmp = *input_ptr++;
            tmp |= *input_ptr++ << 8;

            const auto look_behind = (tmp >> 4) * byte_count;
            const auto size = ((tmp & 0x0F) + size_delta) * byte_count;
            output_ptr.append_from(-look_behind, size);
        }
    }
    return output;
}

static res::Image decode_v0(
    io::File &input_file, const size_t width, const size_t height)
{
    const auto size_comp = input_file.stream.read_le<u32>() - 8;
    const auto size_orig = input_file.stream.read_le<u32>();
    const auto data = decompress(
        input_file.stream.read(size_comp), size_orig, 3, 1);
    return res::Image(width, height, data, res::PixelFormat::BGR888);
}

static res::Image decode_v1(
    io::File &input_file, const size_t width, const size_t height)
{
    const auto size_comp = input_file.stream.read_le<u32>() - 8;
    const auto size_orig = input_file.stream.read_le<u32>();
    io::MemoryStream tmp_stream(
        decompress(input_file.stream.read(size_comp), size_orig, 1, 2));
    const size_t colors = tmp_stream.read_le<u16>();
    const auto pal_data = tmp_stream.read(4 * colors);
    const auto pix_data = tmp_stream.read_to_eof();
    res::Palette palette(colors, pal_data, res::PixelFormat::BGRA8888);
    return res::Image(width, height, pix_data, palette);
}

static res::Image decode_v2(
    io::File &input_file, const size_t width, const size_t height)
{
    std::vector<std::unique_ptr<Region>> regions;
    const auto region_count = input_file.stream.read_le<u32>();
    for (const auto i : algo::range(region_count))
    {
        auto region = std::make_unique<Region>();
        region->x1 = input_file.stream.read_le<u32>();
        region->y1 = input_file.stream.read_le<u32>();
        region->x2 = input_file.stream.read_le<u32>();
        region->y2 = input_file.stream.read_le<u32>();
        region->ox = input_file.stream.read_le<u32>();
        region->oy = input_file.stream.read_le<u32>();
        regions.push_back(std::move(region));
    }

    const auto size_comp = input_file.stream.read_le<u32>() - 8;
    const auto size_orig = input_file.stream.read_le<u32>();

    io::MemoryStream input_stream(
        decompress(input_file.stream.read(size_comp), size_orig, 1, 2));
    input_stream.seek(0);
    if (input_stream.read_le<u32>() != regions.size())
        throw err::CorruptDataError("Region count mismatch");

    res::Image image(width * region_count, height);
    for (const auto &region : regions)
    {
        region->block_offset = input_stream.read_le<u32>();
        region->block_size = input_stream.read_le<u32>();
    }

    size_t region_offset = 0;
    for (const auto &region : regions)
    {
        if (region->block_size <= 0)
            continue;

        input_stream.seek(region->block_offset);
        u16 block_type = input_stream.read_le<u16>();
        u16 part_count = input_stream.read_le<u16>();
        if (block_type != 1)
            throw err::NotSupportedError("Unexpected block type");

        input_stream.skip(0x70);
        for (const auto j : algo::range(part_count))
        {
            const auto part_x = input_stream.read_le<u16>();
            const auto part_y = input_stream.read_le<u16>();
            input_stream.skip(2);
            const auto part_width = input_stream.read_le<u16>();
            const auto part_height = input_stream.read_le<u16>();
            input_stream.skip(0x52);

            res::Image part(
                part_width,
                part_height,
                input_stream,
                res::PixelFormat::BGRA8888);

            const size_t target_x = region->x1 + part_x;
            const size_t target_y = region->y1 + part_y;
            if (target_x + part_width > width
                || target_y + part_height > height)
            {
                throw err::CorruptDataError("Region out of bounds");
            }
            for (const auto y : algo::range(part_height))
            for (const auto x : algo::range(part_width))
            {
                image.at(target_x + x + region_offset, target_y + y) = part.at(x, y);
            }
        }

        region_offset += width;
    }

    return image;
}

bool G00ImageDecoder::is_recognized_impl(io::File &input_file) const
{
    return input_file.path.has_extension("g00");
}

res::Image G00ImageDecoder::decode_impl(
    const Logger &logger, io::File &input_file) const
{
    const auto version = input_file.stream.read<u8>();
    const auto width = input_file.stream.read_le<u16>();
    const auto height = input_file.stream.read_le<u16>();

    if (version == 0)
        return decode_v0(input_file, width, height);

    if (version == 1)
        return decode_v1(input_file, width, height);

    if (version == 2)
        return decode_v2(input_file, width, height);

    throw err::UnsupportedVersionError(version);
}

static auto _ = dec::register_decoder<G00ImageDecoder>("real-live/g00");
