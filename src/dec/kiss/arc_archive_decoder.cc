#include "dec/kiss/arc_archive_decoder.h"
#include "algo/locale.h"
#include "algo/range.h"
#include "err.h"

using namespace au;
using namespace au::dec::kiss;

namespace
{
    struct ArchiveEntryImpl final : dec::ArchiveEntry
    {
        size_t offset;
        size_t size;
    };
}

bool ArcArchiveDecoder::is_recognized_impl(io::File &input_file) const
{
    if (!input_file.path.has_extension("arc"))
        return false;
    Logger dummy_logger;
    dummy_logger.mute();
    const auto meta = read_meta_impl(dummy_logger, input_file);
    if (meta->entries.empty())
        return false;
    const auto last_entry
        = dynamic_cast<ArchiveEntryImpl*>(meta->entries.back().get());
    return last_entry->size + last_entry->offset == input_file.stream.size();
}

std::unique_ptr<dec::ArchiveMeta> ArcArchiveDecoder::read_meta_impl(
    const Logger &logger, io::File &input_file) const
{
    input_file.stream.seek(0);
    const auto file_count = input_file.stream.read_le<u32>();
    auto meta = std::make_unique<ArchiveMeta>();
    ArchiveEntryImpl *last_entry = nullptr;
    for (const size_t i : algo::range(file_count))
    {
        auto entry = std::make_unique<ArchiveEntryImpl>();
        entry->path = algo::sjis_to_utf8(
            input_file.stream.read_to_zero()).str();
        entry->offset = input_file.stream.read_le<u32>();
        if (input_file.stream.read_le<u32>() != 0)
            throw err::CorruptDataError("Expected '0'");
        if (last_entry)
            last_entry->size = entry->offset - last_entry->offset;
        last_entry = entry.get();
        meta->entries.push_back(std::move(entry));
    }
    if (last_entry)
        last_entry->size = input_file.stream.size() - last_entry->offset;
    return meta;
}

std::unique_ptr<io::File> ArcArchiveDecoder::read_file_impl(
    const Logger &logger,
    io::File &input_file,
    const dec::ArchiveMeta &m,
    const dec::ArchiveEntry &e) const
{
    const auto entry = static_cast<const ArchiveEntryImpl*>(&e);
    const auto data = input_file.stream.seek(entry->offset).read(entry->size);
    return std::make_unique<io::File>(entry->path, data);
}

std::vector<std::string> ArcArchiveDecoder::get_linked_formats() const
{
    return {"kiss/plg"};
}

static auto _ = dec::register_decoder<ArcArchiveDecoder>("kiss/arc");
