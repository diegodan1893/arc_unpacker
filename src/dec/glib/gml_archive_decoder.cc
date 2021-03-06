#include "dec/glib/gml_archive_decoder.h"
#include "algo/range.h"
#include "dec/glib/custom_lzss.h"
#include "io/memory_stream.h"

using namespace au;
using namespace au::dec::glib;

static const bstr magic = "GML_ARC\x00"_b;

namespace
{
    struct ArchiveEntryImpl final : dec::ArchiveEntry
    {
        size_t offset;
        size_t size;
        bstr prefix;
    };

    struct ArchiveMetaImpl final : dec::ArchiveMeta
    {
        bstr permutation;
    };
}

bool GmlArchiveDecoder::is_recognized_impl(io::File &input_file) const
{
    return input_file.stream.read(magic.size()) == magic;
}

std::unique_ptr<dec::ArchiveMeta> GmlArchiveDecoder::read_meta_impl(
    const Logger &logger, io::File &input_file) const
{
    input_file.stream.seek(magic.size());
    auto file_data_start = input_file.stream.read_le<u32>();
    auto table_size_orig = input_file.stream.read_le<u32>();
    auto table_size_comp = input_file.stream.read_le<u32>();
    auto table_data = input_file.stream.read(table_size_comp);
    for (auto i : algo::range(table_data.size()))
        table_data[i] ^= 0xFF;
    table_data = custom_lzss_decompress(table_data, table_size_orig);
    io::MemoryStream table_stream(table_data);

    auto meta = std::make_unique<ArchiveMetaImpl>();
    meta->permutation = table_stream.read(0x100);

    auto file_count = table_stream.read_le<u32>();
    for (auto i : algo::range(file_count))
    {
        auto entry = std::make_unique<ArchiveEntryImpl>();
        entry->path = table_stream.read(table_stream.read_le<u32>()).str();
        entry->offset = table_stream.read_le<u32>() + file_data_start;
        entry->size = table_stream.read_le<u32>();
        entry->prefix = table_stream.read(4);
        meta->entries.push_back(std::move(entry));
    }
    return std::move(meta);
}

std::unique_ptr<io::File> GmlArchiveDecoder::read_file_impl(
    const Logger &logger,
    io::File &input_file,
    const dec::ArchiveMeta &m,
    const dec::ArchiveEntry &e) const
{
    auto meta = static_cast<const ArchiveMetaImpl*>(&m);
    auto entry = static_cast<const ArchiveEntryImpl*>(&e);

    input_file.stream.seek(entry->offset);
    input_file.stream.skip(entry->prefix.size());
    auto suffix = input_file.stream.read(entry->size - entry->prefix.size());
    for (auto i : algo::range(suffix.size()))
        suffix[i] = meta->permutation[suffix.get<u8>()[i]];

    auto output_file = std::make_unique<io::File>();
    output_file->path = entry->path;
    output_file->stream.write(entry->prefix);
    output_file->stream.write(suffix);
    return output_file;
}

std::vector<std::string> GmlArchiveDecoder::get_linked_formats() const
{
    return {"glib/pgx", "vorbis/wav"};
}

static auto _ = dec::register_decoder<GmlArchiveDecoder>("glib/gml");
