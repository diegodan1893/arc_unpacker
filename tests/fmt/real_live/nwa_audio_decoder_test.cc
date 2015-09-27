#include "fmt/real_live/nwa_audio_decoder.h"
#include "test_support/catch.hh"
#include "test_support/file_support.h"

using namespace au;
using namespace au::fmt::real_live;

static void do_test(
    const std::string &input_path, const std::string &expected_path)
{
    NwaAudioDecoder decoder;
    auto input_file = tests::zlib_file_from_path(input_path);
    auto expected_file = tests::zlib_file_from_path(expected_path);
    auto actual_file = decoder.decode(*input_file);
    tests::compare_files(*expected_file, *actual_file, false);
}

TEST_CASE("RealLive NWA level 0-compressed mono audio", "[fmt]")
{
    do_test(
        "tests/fmt/real_live/files/nwa/BT_KOE_HCC01-zlib.nwa",
        "tests/fmt/real_live/files/nwa/BT_KOE_HCC01-zlib-out.wav");
}

TEST_CASE("RealLive NWA level 0-compressed stereo audio", "[fmt]")
{
    do_test(
        "tests/fmt/real_live/files/nwa/BATSWING-zlib.nwa",
        "tests/fmt/real_live/files/nwa/BATSWING-zlib-out.wav");
}