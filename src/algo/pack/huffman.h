#pragma once

#include "io/base_bit_stream.h"

namespace au {
namespace algo {
namespace pack {

    struct HuffmanTree final
    {
        HuffmanTree(const bstr &data);
        HuffmanTree(io::BaseBitStream &input_stream);

        int size;
        u16 root;
        u16 nodes[2][512];
    };

    bstr decode_huffman(
        const HuffmanTree &huffman_tree,
        const bstr &input,
        const size_t target_size);

} } }
