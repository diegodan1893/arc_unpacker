#pragma once

#include "dec/base_image_decoder.h"

namespace au {
namespace dec {
namespace west_vision {

    class SygImageDecoder final : public BaseImageDecoder
    {
    protected:
        bool is_recognized_impl(io::File &input_file) const override;
        res::Image decode_impl(
            const Logger &logger, io::File &input_file) const override;
    };

} } }
