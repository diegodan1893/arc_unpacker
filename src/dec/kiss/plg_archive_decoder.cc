#include "dec/kiss/plg_archive_decoder.h"
#include "algo/range.h"
#include "err.h"

using namespace au;
using namespace au::dec::kiss;

static const bstr magic = "LIB\x00\x00\x00\x01\x00"_b;

namespace
{
    struct ArchiveEntryImpl final : dec::ArchiveEntry
    {
        size_t offset;
        size_t size;
    };
}

bool PlgArchiveDecoder::is_recognized_impl(io::File &input_file) const
{
    input_file.stream.seek(0);
    return input_file.stream.read(magic.size()) == magic;
}

std::unique_ptr<dec::ArchiveMeta> PlgArchiveDecoder::read_meta_impl(
    const Logger &logger, io::File &input_file) const
{
    input_file.stream.seek(magic.size());
    const auto file_count = input_file.stream.read_le<u32>();
    input_file.stream.skip(4);
    auto meta = std::make_unique<ArchiveMeta>();
    for (const size_t i : algo::range(file_count))
    {
        auto entry = std::make_unique<ArchiveEntryImpl>();
        entry->path = input_file.stream.read_to_zero(0x20).str();
        if (input_file.stream.read_le<u32>() != i)
            throw err::CorruptDataError("Unexpected entry index");
        entry->size = input_file.stream.read_le<u32>();
        entry->offset = input_file.stream.read_le<u32>();
        if (input_file.stream.read_le<u32>() != 0)
            throw err::CorruptDataError("Expected '0'");
        meta->entries.push_back(std::move(entry));
    }
    return meta;
}

std::unique_ptr<io::File> PlgArchiveDecoder::read_file_impl(
    const Logger &logger,
    io::File &input_file,
    const dec::ArchiveMeta &m,
    const dec::ArchiveEntry &e) const
{
    const auto entry = static_cast<const ArchiveEntryImpl*>(&e);
    const auto data = input_file.stream.seek(entry->offset).read(entry->size);
    return std::make_unique<io::File>(entry->path, data);
}

std::vector<std::string> PlgArchiveDecoder::get_linked_formats() const
{
    return {"kiss/plg", "kiss/custom-png"};
}

static auto _ = dec::register_decoder<PlgArchiveDecoder>("kiss/plg");
