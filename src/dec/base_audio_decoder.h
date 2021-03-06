#pragma once

#include "base_decoder.h"
#include "res/audio.h"

namespace au {
namespace dec {

    class BaseAudioDecoder : public BaseDecoder
    {
    public:
        virtual ~BaseAudioDecoder() {}

        algo::NamingStrategy naming_strategy() const override;

        void accept(IDecoderVisitor &visitor) const override;

        res::Audio decode(const Logger &logger, io::File &input_file) const;

    protected:
        virtual res::Audio decode_impl(
            const Logger &logger, io::File &input_file) const = 0;
    };

} }
