#include "dec/kid/lnk_archive_decoder.h"
#include "algo/range.h"
#include "dec/kid/lnd_file_decoder.h"
#include "err.h"
#include "io/memory_stream.h"

using namespace au;
using namespace au::dec::kid;

static const bstr magic = "LNK\x00"_b;

namespace
{
    struct ArchiveEntryImpl final : dec::ArchiveEntry
    {
        bool compressed;
        size_t offset;
        size_t size;
    };
}

bool LnkArchiveDecoder::is_recognized_impl(io::File &input_file) const
{
    return input_file.stream.read(magic.size()) == magic;
}

std::unique_ptr<dec::ArchiveMeta> LnkArchiveDecoder::read_meta_impl(
    const Logger &logger, io::File &input_file) const
{
    input_file.stream.seek(magic.size());
    auto meta = std::make_unique<ArchiveMeta>();
    auto file_count = input_file.stream.read_le<u32>();
    input_file.stream.skip(8);
    auto file_data_start = input_file.stream.tell() + (file_count << 5);
    for (auto i : algo::range(file_count))
    {
        auto entry = std::make_unique<ArchiveEntryImpl>();
        entry->offset = input_file.stream.read_le<u32>() + file_data_start;
        u32 tmp = input_file.stream.read_le<u32>();
        entry->compressed = tmp & 1;
        entry->size = tmp >> 1;
        entry->path = input_file.stream.read_to_zero(24).str();
        meta->entries.push_back(std::move(entry));
    }
    return meta;
}

std::unique_ptr<io::File> LnkArchiveDecoder::read_file_impl(
    const Logger &logger,
    io::File &input_file,
    const dec::ArchiveMeta &m,
    const dec::ArchiveEntry &e) const
{
    auto entry = static_cast<const ArchiveEntryImpl*>(&e);
    auto output_file = std::make_unique<io::File>();
    output_file->path = entry->path;

    input_file.stream.seek(entry->offset);
    auto data = input_file.stream.read(entry->size);

    int key_pos = -1;
    if (output_file->path.has_extension(".wav"))
        key_pos = 0;
    else if (output_file->path.has_extension(".jpg"))
        key_pos = 0x1100;
    else if (output_file->path.has_extension(".scr"))
        key_pos = 0x1000;

    if (key_pos >= 0 && key_pos < static_cast<int>(entry->size))
    {
        u8 key = 0;
        for (const u8 c : entry->path.str())
            key += c;

        for (size_t i = 0; i < 0x100 && key_pos + i < entry->size; i++)
        {
            data[key_pos + i] -= key;
            key = key * 0x6D - 0x25;
        }
    }
    output_file->stream.write(data);

    if (entry->compressed)
    {
        const auto lnd_file_decoder = LndFileDecoder();
        return lnd_file_decoder.decode(logger, *output_file);
    }
    else
        return output_file;
}

std::vector<std::string> LnkArchiveDecoder::get_linked_formats() const
{
    return {"kid/cps", "kid/prt", "kid/waf"};
}

static auto _ = dec::register_decoder<LnkArchiveDecoder>("kid/lnk");
