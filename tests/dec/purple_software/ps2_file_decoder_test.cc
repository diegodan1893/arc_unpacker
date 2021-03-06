#include "dec/purple_software/ps2_file_decoder.h"
#include "test_support/catch.h"
#include "test_support/decoder_support.h"
#include "test_support/file_support.h"
#include "test_support/image_support.h"

using namespace au;
using namespace au::dec::purple_software;

static const std::string dir = "tests/dec/purple_software/files/ps2/";

static void do_test(
    const std::string &input_path, const std::string &expected_path)
{
    const auto decoder = Ps2FileDecoder();
    const auto input_file = tests::file_from_path(dir + input_path);
    const auto expected_file = tests::file_from_path(dir + expected_path);
    const auto actual_file = tests::decode(decoder, *input_file);
    tests::compare_files(*actual_file, *expected_file, false);
}

TEST_CASE("Purple Software PS2 files", "[dec]")
{
    do_test("intproc.ps3", "intproc-out.ps3");
}
