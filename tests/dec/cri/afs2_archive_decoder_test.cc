#include "dec/cri/afs2_archive_decoder.h"
#include "test_support/catch.h"
#include "test_support/decoder_support.h"
#include "test_support/file_support.h"

using namespace au;
using namespace au::dec::cri;

static const std::string dir = "tests/dec/cri/files/afs2/";

static void do_test(const std::string &input_path)
{
    const std::vector<std::shared_ptr<io::File>> expected_files
    {
        tests::stub_file("0.dat", "1234567890"_b),
        tests::stub_file("1.dat", "1234567890"_b),
        tests::stub_file("2.dat", "abcdefghijklmnopqrstuvwxyz"_b),
    };
    const auto decoder = Afs2ArchiveDecoder();
    const auto input_file = tests::file_from_path(dir + input_path);
    const auto actual_files = tests::unpack(decoder, *input_file);
    tests::compare_files(actual_files, expected_files, true);
}

TEST_CASE("Cri AFS2 archives", "[dec]")
{
    do_test("test.awb");
}
