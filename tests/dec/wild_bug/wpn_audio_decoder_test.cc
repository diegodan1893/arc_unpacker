#include "dec/wild_bug/wpn_audio_decoder.h"
#include "test_support/audio_support.h"
#include "test_support/catch.h"
#include "test_support/decoder_support.h"
#include "test_support/file_support.h"

using namespace au;
using namespace au::dec::wild_bug;

static const std::string dir = "tests/dec/wild_bug/files/wpn/";

static void do_test(
    const std::string &input_path,
    const std::string &expected_path)
{
    const auto decoder = WpnAudioDecoder();
    const auto input_file = tests::zlib_file_from_path(dir + input_path);
    const auto expected_file = tests::zlib_file_from_path(dir + expected_path);
    const auto actual_audio = tests::decode(decoder, *input_file);
    tests::compare_audio(actual_audio, *expected_file);
}

TEST_CASE("Wild Bug WPN audio", "[dec]")
{
    do_test("SE_107-zlib.WPN", "SE_107-zlib-out.wav");
}
