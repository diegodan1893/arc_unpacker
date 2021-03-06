#include "dec/propeller/mgr_archive_decoder.h"
#include "algo/format.h"
#include "algo/range.h"
#include "io/memory_stream.h"

using namespace au;
using namespace au::dec::propeller;

namespace
{
    struct ArchiveEntryImpl final : dec::ArchiveEntry
    {
        size_t offset;
    };
}

static bstr decompress(const bstr &input, size_t size_original)
{
    const u8 *input_ptr = input.get<const u8>();
    const u8 *input_end = input_ptr + input.size();

    bstr output(size_original);
    u8 *output_ptr = output.get<u8>();
    u8 *output_end = output_ptr + size_original;

    while (output_ptr < output_end)
    {
        u32 c = *input_ptr++;

        if (c < 0x20)
        {
            u32 size = c + 1;
            while (size--)
                *output_ptr++ = *input_ptr++;
        }
        else
        {
            u32 size = c >> 5;
            if (size == 7)
                size += *input_ptr++;
            size += 2;

            u32 look_behind = ((c & 0x1F) << 8) + 1;
            look_behind += *input_ptr++;

            u8 *source = output_ptr - look_behind;
            while (size--)
                *output_ptr++ = *source++;
        }
    }

    return output;
}

bool MgrArchiveDecoder::is_recognized_impl(io::File &input_file) const
{
    return input_file.path.has_extension("mgr");
}

std::unique_ptr<dec::ArchiveMeta> MgrArchiveDecoder::read_meta_impl(
    const Logger &logger, io::File &input_file) const
{
    auto entry_count = input_file.stream.read_le<u16>();
    auto meta = std::make_unique<ArchiveMeta>();
    for (auto i : algo::range(entry_count))
    {
        auto entry = std::make_unique<ArchiveEntryImpl>();
        entry->offset = entry_count == 1
            ? input_file.stream.tell()
            : input_file.stream.read_le<u32>();
        entry->path = algo::format("%d.bmp", i);
        meta->entries.push_back(std::move(entry));
    }
    return meta;
}

std::unique_ptr<io::File> MgrArchiveDecoder::read_file_impl(
    const Logger &logger,
    io::File &input_file,
    const dec::ArchiveMeta &m,
    const dec::ArchiveEntry &e) const
{
    auto entry = static_cast<const ArchiveEntryImpl*>(&e);
    input_file.stream.seek(entry->offset);
    size_t size_orig = input_file.stream.read_le<u32>();
    size_t size_comp = input_file.stream.read_le<u32>();

    auto data = input_file.stream.read(size_comp);
    data = decompress(data, size_orig);
    return std::make_unique<io::File>(entry->path, data);
}

static auto _ = dec::register_decoder<MgrArchiveDecoder>("propeller/mgr");
