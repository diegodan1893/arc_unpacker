#include "dec/amuse_craft/pac_archive_decoder.h"
#include "algo/range.h"
#include "err.h"

using namespace au;
using namespace au::dec::amuse_craft;

static const bstr magic = "PAC\x20"_b;

namespace
{
    struct ArchiveEntryImpl final : dec::ArchiveEntry
    {
        size_t offset;
        size_t size;
    };
}

static int detect_version(io::BaseByteStream &input_stream)
{
    try
    {
        input_stream.seek(0);
        const auto file_count = input_stream.read_le<u16>();
        input_stream.seek(0x3FE);
        input_stream.skip((file_count - 1) * (16 + 8));
        input_stream.skip(16);
        const auto last_entry_offset = input_stream.read_le<u32>();
        const auto last_entry_size = input_stream.read_le<u32>();
        if (last_entry_offset + last_entry_size == input_stream.size())
            return 1;
    }
    catch (...) {}

    try
    {
        input_stream.seek(magic.size() + 4);
        const auto file_count = input_stream.read_le<u32>();
        input_stream.seek(0x804);
        input_stream.skip((file_count - 1) * (32 + 8));
        input_stream.skip(32);
        const auto last_entry_offset = input_stream.read_le<u32>();
        const auto last_entry_size = input_stream.read_le<u32>();
        if (last_entry_offset + last_entry_size == input_stream.size())
            return 2;
    }
    catch (...) {}

    throw err::RecognitionError();
}

static std::unique_ptr<dec::ArchiveMeta> read_meta(
    io::File &input_file, const size_t file_count, const size_t name_size)
{
    auto meta = std::make_unique<dec::ArchiveMeta>();
    for (const auto i : algo::range(file_count))
    {
        auto entry = std::make_unique<ArchiveEntryImpl>();
        auto path = input_file.stream.read_to_zero(name_size).str();
        std::replace(path.begin(), path.end(), '_', '/');
        entry->path = path;
        entry->size = input_file.stream.read_le<u32>();
        entry->offset = input_file.stream.read_le<u32>();
        meta->entries.push_back(std::move(entry));
    }
    return meta;
}

bool PacArchiveDecoder::is_recognized_impl(io::File &input_file) const
{
    return detect_version(input_file.stream) > 0;
}

std::unique_ptr<dec::ArchiveMeta> PacArchiveDecoder::read_meta_impl(
    const Logger &logger, io::File &input_file) const
{
    const auto version = detect_version(input_file.stream);
    if (version == 1)
    {
        input_file.stream.seek(0);
        const auto file_count = input_file.stream.read_le<u16>();
        input_file.stream.seek(0x3FE);
        return ::read_meta(input_file, file_count, 16);
    }
    else if (version == 2)
    {
        input_file.stream.seek(magic.size() + 4);
        const auto file_count = input_file.stream.read_le<u32>();
        input_file.stream.seek(0x804);
        return ::read_meta(input_file, file_count, 32);
    }
    else
        throw err::UnsupportedVersionError(version);
}

std::unique_ptr<io::File> PacArchiveDecoder::read_file_impl(
    const Logger &logger,
    io::File &input_file,
    const dec::ArchiveMeta &m,
    const dec::ArchiveEntry &e) const
{
    const auto entry = static_cast<const ArchiveEntryImpl*>(&e);
    input_file.stream.seek(entry->offset);
    const auto data = input_file.stream.read(entry->size);
    return std::make_unique<io::File>(entry->path, data);
}

std::vector<std::string> PacArchiveDecoder::get_linked_formats() const
{
    return
    {
        "truevision/tga",
        "amuse-craft/pgd-ge",
        "amuse-craft/pgd-c00",
        "amuse-craft/bgm",
    };
}

static auto _ = dec::register_decoder<PacArchiveDecoder>("amuse-craft/pac");
