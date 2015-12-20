#include "fmt/libido/arc_archive_decoder.h"
#include "algo/pack/lzss.h"
#include "algo/range.h"

using namespace au;
using namespace au::fmt::libido;

namespace
{
    struct ArchiveEntryImpl final : fmt::ArchiveEntry
    {
        size_t offset;
        size_t size_orig;
        size_t size_comp;
    };
}

bool ArcArchiveDecoder::is_recognized_impl(io::File &input_file) const
{
    auto file_count = input_file.stream.read_u32_le();
    if (file_count)
    {
        input_file.stream.skip((file_count - 1) * 32 + 24);
        const auto last_file_size_comp = input_file.stream.read_u32_le();
        const auto last_file_offset = input_file.stream.read_u32_le();
        input_file.stream.seek(last_file_size_comp + last_file_offset);
    }
    else
        input_file.stream.skip(1);
    return input_file.stream.eof();
}

std::unique_ptr<fmt::ArchiveMeta>
    ArcArchiveDecoder::read_meta_impl(io::File &input_file) const
{
    auto meta = std::make_unique<ArchiveMeta>();
    u32 file_count = input_file.stream.read_u32_le();
    for (auto i : algo::range(file_count))
    {
        auto entry = std::make_unique<ArchiveEntryImpl>();
        auto tmp = input_file.stream.read(20);
        for (auto i : algo::range(tmp.size()))
            tmp[i] ^= tmp[tmp.size() - 1];
        entry->path = tmp.str(true);
        entry->size_orig = input_file.stream.read_u32_le();
        entry->size_comp = input_file.stream.read_u32_le();
        entry->offset = input_file.stream.read_u32_le();
        meta->entries.push_back(std::move(entry));
    }
    return meta;
}

std::unique_ptr<io::File> ArcArchiveDecoder::read_file_impl(
    io::File &input_file,
    const fmt::ArchiveMeta &m,
    const fmt::ArchiveEntry &e) const
{
    auto entry = static_cast<const ArchiveEntryImpl*>(&e);
    input_file.stream.seek(entry->offset);
    auto data = input_file.stream.read(entry->size_comp);
    data = algo::pack::lzss_decompress(data, entry->size_orig);
    return std::make_unique<io::File>(entry->path, data);
}

static auto dummy = fmt::register_fmt<ArcArchiveDecoder>("libido/arc");
