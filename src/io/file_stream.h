#pragma once

#include <memory>
#include <string>
#include "io/base_byte_stream.h"
#include "io/path.h"

namespace au {
namespace io {

    enum class FileMode : u8
    {
        Read = 1,
        Write = 2,
    };

    class FileStream final : public BaseByteStream
    {
    public:
        FileStream(const path &path, const FileMode mode);
        ~FileStream();

        size_t size() const override;
        size_t tell() const override;

        std::unique_ptr<BaseByteStream> clone() const override;

    protected:
        void read_impl(void *destination, const size_t size) override;
        void write_impl(const void *source, const size_t size) override;
        void seek_impl(const size_t offset) override;
        void truncate_impl(const size_t new_size) override;

    private:
        struct Priv;
        std::unique_ptr<Priv> p;
    };

} }
