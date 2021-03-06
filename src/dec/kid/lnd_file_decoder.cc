#include "dec/kid/lnd_file_decoder.h"
#include "algo/range.h"
#include "io/memory_stream.h"

using namespace au;
using namespace au::dec::kid;

static const bstr magic = "lnd\x00"_b;

bstr LndFileDecoder::decompress_raw_data(const bstr &input, size_t size_orig)
{
    bstr output(size_orig);

    auto output_ptr = output.get<u8>();
    auto output_end = output.end<const u8>();
    auto input_ptr = input.get<const u8>();
    auto input_end = input.end<const u8>();

    while (output_ptr < output_end && input_ptr < input_end)
    {
        u8 byte = *input_ptr++;
        if (byte & 0x80)
        {
            if (byte & 0x40)
            {
                int repetitions = (byte & 0x1F) + 2;
                if (byte & 0x20)
                    repetitions += *input_ptr++ << 5;
                for (auto i : algo::range(repetitions))
                {
                    if (output_ptr >= output_end)
                        break;
                    *output_ptr++ = *input_ptr;
                }
                input_ptr++;
            }
            else
            {
                int size = ((byte >> 2) & 0xF) + 2;
                int look_behind = ((byte & 3) << 8) + *input_ptr++ + 1;
                if (look_behind < 0)
                    look_behind = 0;
                for (auto i : algo::range(size))
                {
                    u8 tmp = output_ptr[-look_behind];
                    if (output_ptr >= output_end)
                        break;
                    *output_ptr++ = tmp;
                }
            }
        }
        else
        {
            if (byte & 0x40)
            {
                int repetitions = *input_ptr++ + 1;
                int size = (byte & 0x3F) + 2;
                for (auto i : algo::range(repetitions))
                    for (auto i : algo::range(size))
                    {
                        if (output_ptr >= output_end)
                            break;
                        *output_ptr++ += input_ptr[i];
                    }
                input_ptr += size;
            }
            else
            {
                int size = (byte & 0x1F) + 1;
                if (byte & 0x20)
                    size += *input_ptr++ << 5;
                for (auto i : algo::range(size))
                {
                    if (output_ptr >= output_end)
                        break;
                    *output_ptr++ = *input_ptr++;
                }
            }
        }
    }

    return output;
}

bool LndFileDecoder::is_recognized_impl(io::File &input_file) const
{
    return input_file.stream.read(magic.size()) == magic;
}

std::unique_ptr<io::File> LndFileDecoder::decode_impl(
    const Logger &logger, io::File &input_file) const
{
    input_file.stream.seek(magic.size());
    input_file.stream.skip(4);
    auto size_orig = input_file.stream.read_le<u32>();
    input_file.stream.skip(4);
    auto data = input_file.stream.read_to_eof();
    data = decompress_raw_data(data, size_orig);
    return std::make_unique<io::File>(input_file.path, data);
}

static auto _ = dec::register_decoder<LndFileDecoder>("kid/lnd");
