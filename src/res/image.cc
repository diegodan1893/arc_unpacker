#include "res/image.h"
#include <algorithm>
#include "algo/format.h"
#include "algo/range.h"
#include "err.h"

using namespace au;
using namespace au::res;

Image::Image(const Image &other) : Image(other.width(), other.height())
{
    for (const auto y : algo::range(_height))
    for (const auto x : algo::range(_width))
        at(x, y) = other.at(x, y);
}

Image::Image(const size_t width, const size_t height)
    : pixels(width * height),  _width(width), _height(height)
{
}

Image::Image(
    const size_t width,
    const size_t height,
    const bstr &input,
    const PixelFormat fmt) : Image(width, height)
{
    if (input.size() < pixel_format_to_bpp(fmt) * width * height)
        throw err::BadDataSizeError();
    read_pixels(input.get<const u8>(), pixels, fmt);
}

Image::Image(
    const size_t width,
    const size_t height,
    io::BaseByteStream &input_stream,
    const PixelFormat fmt) :
        Image(
            width,
            height,
            input_stream.read(width * height * pixel_format_to_bpp(fmt)),
            fmt)
{
}

Image::Image(
    const size_t width,
    const size_t height,
    const bstr &input,
    const Palette &palette)
        : Image(width, height, input, PixelFormat::Gray8)
{
    apply_palette(palette);
}

Image::Image(
    const size_t width,
    const size_t height,
    io::BaseByteStream &input_stream,
    const Palette &palette)
        : Image(width, height, input_stream, PixelFormat::Gray8)
{
    apply_palette(palette);
}

Image::~Image()
{
}

Image &Image::invert()
{
    for (const auto y : algo::range(_height))
    for (const auto x : algo::range(_width))
    {
        at(x, y).r ^= 0xFF;
        at(x, y).g ^= 0xFF;
        at(x, y).b ^= 0xFF;
    }
    return *this;
}

Image &Image::flip_vertically()
{
    for (const auto y : algo::range(_height >> 1))
    for (const auto x : algo::range(_width))
    {
        const auto t = at(x, _height - 1 - y);
        at(x, _height - 1 - y) = at(x, y);
        at(x, y) = t;
    }
    return *this;
}

Image &Image::flip_horizontally()
{
    for (const auto y : algo::range(_height))
    for (const auto x : algo::range(_width >> 1))
    {
        const auto t = at(_width - 1 - x, y);
        at(_width - 1 - x, y) = at(x, y);
        at(x, y) = t;
    }
    return *this;
}

Image &Image::crop(const size_t new_width, const size_t new_height)
{
    std::vector<Pixel> old_pixels(pixels.begin(), pixels.end());
    const auto old_width = _width;
    const auto old_height = _height;
    _width = new_width;
    _height = new_height;
    pixels.resize(new_width * new_height);
    for (const auto y : algo::range(std::min(old_height, new_height)))
    for (const auto x : algo::range(std::min(old_width, new_width)))
    {
        pixels[y * new_width + x] = old_pixels[y * old_width + x];
    }
    return *this;
}

Image &Image::apply_mask(const Image &other)
{
    if (other.width() != _width || other.height() != _height)
        throw std::logic_error("Mask image size is different from image size");
    for (const auto y : algo::range(_height))
    for (const auto x : algo::range(_width))
        at(x, y).a = other.at(x, y).r;
    return *this;
}

Image &Image::apply_palette(const Palette &palette)
{
    const auto palette_size = palette.size();
    for (auto &c : pixels)
    {
        if (c.r < palette_size)
            c = palette[c.r];
        else
            c.a = 0x00;
    }
    return *this;
}

Image &Image::overlay(
    const Image &other,
    const OverlayKind overlay_kind)
{
    return overlay(other, 0, 0, overlay_kind);
}

Image &Image::overlay(
    const Image &other,
    const int target_x,
    const int target_y,
    const OverlayKind overlay_kind)
{
    const int x1 = std::max<int>(0, target_x);
    const int x2 = std::min<int>(width(), target_x + other.width());
    const int y1 = std::max<int>(0, target_y);
    const int y2 = std::min<int>(height(), target_y + other.height());
    const int source_x = -target_x;
    const int source_y = -target_y;
    if (overlay_kind == OverlayKind::OverwriteAll)
    {
        for (const auto y : algo::range(y1, y2))
        for (const auto x : algo::range(x1, x2))
            at(x, y) = other.at(source_x + x, source_y + y);
    }
    else if (overlay_kind == OverlayKind::OverwriteNonTransparent)
    {
        for (const auto y : algo::range(y1, y2))
        for (const auto x : algo::range(x1, x2))
        {
            const auto &source_pixel = other.at(source_x + x, source_y + y);
            if (source_pixel.a)
                at(x, y) = source_pixel;
        }
    }
    else if (overlay_kind == OverlayKind::AddSimple)
    {
        for (const auto y : algo::range(y1, y2))
        for (const auto x : algo::range(x1, x2))
        {
            auto &target_pixel = at(x, y);
            const auto &source_pixel = other.at(source_x + x, source_y + y);
            target_pixel.r += source_pixel.r;
            target_pixel.g += source_pixel.g;
            target_pixel.b += source_pixel.b;
        }
    }
    else
    {
        throw std::logic_error("Unknown overlay kind");
    }
    return *this;
}

Pixel *Image::begin()
{
    return pixels.data();
}

Pixel *Image::end()
{
    return pixels.empty() ? nullptr : begin() + _width * _height;
}

const Pixel *Image::begin() const
{
    return pixels.data();
}

const Pixel *Image::end() const
{
    return pixels.empty() ? nullptr : begin() + _width * _height;
}
