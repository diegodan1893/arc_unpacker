#include "dec/lilim/aos1_archive_decoder.h"
#include "algo/range.h"
#include "io/msb_bit_stream.h"

using namespace au;
using namespace au::dec::lilim;

namespace
{
    struct ArchiveEntryImpl final : dec::ArchiveEntry
    {
        size_t offset;
        size_t size;
    };
}

bool Aos1ArchiveDecoder::is_recognized_impl(io::File &input_file) const
{
    input_file.stream.seek(0);
    return input_file.path.has_extension("aos")
        && input_file.stream.read_le<u32>() > 0;
}

std::unique_ptr<dec::ArchiveMeta> Aos1ArchiveDecoder::read_meta_impl(
    const Logger &logger, io::File &input_file) const
{
    input_file.stream.seek(0);
    auto meta = std::make_unique<ArchiveMeta>();
    while (!input_file.stream.eof())
    {
        auto entry = std::make_unique<ArchiveEntryImpl>();
        const auto name = input_file.stream.read_to_zero(0x10);
        if (!name.size())
            break;
        if (name[0] == 0xFF)
        {
            auto offset = input_file.stream.read_le<u32>();
            input_file.stream.skip(12);
            input_file.stream.skip(offset);
            continue;
        }
        entry->path = name.str();
        entry->offset = input_file.stream.read_le<u32>();
        entry->size = input_file.stream.read_le<u32>();
        input_file.stream.skip(8);
        entry->offset += input_file.stream.tell();
        meta->entries.push_back(std::move(entry));
    }
    return meta;
}

std::unique_ptr<io::File> Aos1ArchiveDecoder::read_file_impl(
    const Logger &logger,
    io::File &input_file,
    const dec::ArchiveMeta &m,
    const dec::ArchiveEntry &e) const
{
    const auto entry = static_cast<const ArchiveEntryImpl*>(&e);
    const auto data = input_file.stream.seek(entry->offset).read(entry->size);
    return std::make_unique<io::File>(entry->path, data);
}

std::vector<std::string> Aos1ArchiveDecoder::get_linked_formats() const
{
    return {"lilim/scr", "lilim/abm", "microsoft/bmp"};
}

static auto _ = dec::register_decoder<Aos1ArchiveDecoder>("lilim/aos1");
