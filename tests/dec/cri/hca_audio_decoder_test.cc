#include "dec/cri/hca_audio_decoder.h"
#include "test_support/audio_support.h"
#include "test_support/catch.h"
#include "test_support/decoder_support.h"
#include "test_support/file_support.h"

using namespace au;
using namespace au::dec::cri;

static const std::string dir = "tests/dec/cri/files/hca/";

static void do_test(
    const std::string &input_path, const std::string &expected_path)
{
    const auto decoder = HcaAudioDecoder();
    const auto input_file = tests::file_from_path(dir + input_path);
    const auto expected_file = tests::file_from_path(dir + expected_path);
    const auto actual_audio = tests::decode(decoder, *input_file);
    tests::compare_audio(actual_audio, *expected_file);
}

TEST_CASE("CRI HCA audio", "[dec]")
{
    SECTION("Mono, unlooped, cipher 0, no 'dec' chunk, no advanced compression")
    {
        do_test("test.hca", "test-out.wav");
    }
}
