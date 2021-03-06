#include "dec/west_vision/syg_image_decoder.h"
#include "test_support/catch.h"
#include "test_support/decoder_support.h"
#include "test_support/file_support.h"
#include "test_support/image_support.h"

using namespace au;
using namespace au::dec::west_vision;

static const std::string dir = "tests/dec/west_vision/files/syg/";

static void do_test(
    const std::string &input_path, const std::string &expected_path)
{
    const auto decoder = SygImageDecoder();
    const auto input_file = tests::zlib_file_from_path(dir + input_path);
    const auto expected_file = tests::file_from_path(dir + expected_path);
    const auto actual_image = tests::decode(decoder, *input_file);
    tests::compare_images(actual_image, *expected_file);
}

TEST_CASE("WestVision SYG images", "[dec]")
{
    do_test("loadx-zlib.syg", "loadx-out.png");
}
