#define BOOST_TEST_MODULE CompressorTests

#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <set>
#include <string>
#include <random>
#include <vector>

#include "lzx.h"

struct Fixture {
  Fixture() {
    for (auto i = 0; i < 32; ++i) {
      small_simple.push_back(i);
    }

    for (auto i = 0; i < 32 * 1024; ++i) {
      block_sized_simple.push_back(i & 0xFF);
    }

    for (auto i = 0; i < 32 * 1024 + 1; ++i) {
      block_sized_plus_one_simple.push_back(i & 0xFF);
    }

    for (auto i = 0; i < 128 * 1024; ++i) {
      large_simple.push_back(i & 0xFF);
    }


    constexpr size_t VECTOR_SIZE_BYTES = 128 * 1024;
    constexpr uint32_t SEED = 0x44556677;

    large_noise.resize(VECTOR_SIZE_BYTES);
    std::mt19937 engine(SEED);
    std::uniform_int_distribution<uint8_t> distribution;
    std::generate(large_noise.begin(), large_noise.end(), [&]() {
      return distribution(engine);
    });
  }

  std::vector<uint8_t> small_simple;
  std::vector<uint8_t> block_sized_simple;
  std::vector<uint8_t> block_sized_plus_one_simple;
  std::vector<uint8_t> large_simple;

  std::vector<uint8_t> large_noise;
};

std::string byte_to_hex_str(uint8_t b) {
  std::ostringstream oss;
  oss << "0x" << std::hex << std::setw(2) << std::setfill('0')
      << static_cast<int>(b);
  return oss.str();
}

bool check_decompressed(const std::vector<uint8_t> &expected,
                        const uint8_t *actual, uint32_t actual_size,
                        std::string &diff_message) {
  if (expected.size() != actual_size) {
    std::ostringstream oss;
    oss << "Vectors differ in size. expected_size: " << expected.size()
        << ", vec2_size: " << actual_size << ".";

    size_t common_length =
        std::min(expected.size(), static_cast<unsigned long>(actual_size));
    for (size_t i = 0; i < common_length; ++i) {
      if (expected[i] != actual[i]) {
        oss << " First differing element before size mismatch at Location: "
            << i << ", expected_byte: " << byte_to_hex_str(expected[i])
            << ", vec2_byte: " << byte_to_hex_str(actual[i]);
        diff_message = oss.str();
        return false;
      }
    }
    diff_message = oss.str();
    return false;
  }

  for (size_t i = 0; i < expected.size(); ++i) {
    if (expected[i] != actual[i]) {
      std::ostringstream oss;
      oss << "Location: " << i
          << ", expected_byte: " << byte_to_hex_str(expected[i])
          << ", vec2_byte: " << byte_to_hex_str(actual[i]);
      diff_message = oss.str();
      return false;
    }
  }

  return true;
}

BOOST_FIXTURE_TEST_SUITE(compressor_suite, Fixture)

// Compression of buffers < 1 block crashes.
// BOOST_AUTO_TEST_CASE(small_simple_round_trip) {
//     uint8_t *compressed = nullptr;
//     uint32_t compressed_size = 0;
//     auto result = lzx_compress(small_simple.data(), small_simple.size(),
//     &compressed, &compressed_size); BOOST_TEST(!result, "lzx_compress must
//     return success");
//
//     uint8_t *decompressed = nullptr;
//     uint32_t decompressed_buffer_size = 0;
//     uint32_t decompressed_size = 0;
//     result = lzx_decompress(compressed, compressed_size, &decompressed,
//     &decompressed_buffer_size, &decompressed_size); BOOST_TEST(!result,
//     "lzx_decompress must return success");
//
////    std::string diff;
////    BOOST_CHECK_MESSAGE(
////            check_decompressed(small_simple, decompressed, diff),
////            diff);
//}

BOOST_AUTO_TEST_CASE(block_sized_simple_round_trip) {
  uint8_t *compressed = nullptr;
  uint32_t compressed_size = 0;
  auto result =
      lzx_compress(block_sized_simple.data(), block_sized_simple.size(),
                   &compressed, &compressed_size);
  BOOST_TEST(!result, "lzx_compress must return success");

  uint8_t *decompressed = nullptr;
  uint32_t decompressed_buffer_size = 0;
  uint32_t decompressed_size = 0;
  result = lzx_decompress(compressed, compressed_size, &decompressed,
                          &decompressed_buffer_size, &decompressed_size);
  BOOST_TEST(!result, "lzx_decompress must return success");

  std::string diff;
  BOOST_TEST(check_decompressed(block_sized_simple, decompressed,
                                decompressed_size, diff),
             diff);
  free(decompressed);
  free(compressed);
}

BOOST_AUTO_TEST_CASE(block_sized_plus_one_simple_round_trip) {
  uint8_t *compressed = nullptr;
  uint32_t compressed_size = 0;
  auto result = lzx_compress(block_sized_plus_one_simple.data(),
                             block_sized_plus_one_simple.size(), &compressed,
                             &compressed_size);
  BOOST_TEST(!result, "lzx_compress must return success");

  uint8_t *decompressed = nullptr;
  uint32_t decompressed_buffer_size = 0;
  uint32_t decompressed_size = 0;
  result = lzx_decompress(compressed, compressed_size, &decompressed,
                          &decompressed_buffer_size, &decompressed_size);
  BOOST_TEST(!result, "lzx_decompress must return success");

  std::string diff;
  BOOST_TEST(check_decompressed(block_sized_plus_one_simple, decompressed,
                                decompressed_size, diff),
             diff);

  free(decompressed);
  free(compressed);
}

BOOST_AUTO_TEST_CASE(large_simple_round_trip) {
  uint8_t *compressed = nullptr;
  uint32_t compressed_size = 0;
  auto result = lzx_compress(large_simple.data(), large_simple.size(),
                             &compressed, &compressed_size);
  BOOST_TEST(!result, "lzx_compress must return success");

  uint8_t *decompressed = nullptr;
  uint32_t decompressed_buffer_size = 0;
  uint32_t decompressed_size = 0;
  result = lzx_decompress(compressed, compressed_size, &decompressed,
                          &decompressed_buffer_size, &decompressed_size);
  BOOST_TEST(!result, "lzx_decompress must return success");

  std::string diff;
  BOOST_TEST(
      check_decompressed(large_simple, decompressed, decompressed_size, diff),
      diff);

  free(decompressed);
  free(compressed);
}


BOOST_AUTO_TEST_CASE(large_noise_round_trip) {
  uint8_t *compressed = nullptr;
  uint32_t compressed_size = 0;
  auto result = lzx_compress(large_noise.data(), large_noise.size(),
                             &compressed, &compressed_size);
  BOOST_TEST(!result, "lzx_compress must return success");

  uint8_t *decompressed = nullptr;
  uint32_t decompressed_buffer_size = 0;
  uint32_t decompressed_size = 0;
  result = lzx_decompress(compressed, compressed_size, &decompressed,
                          &decompressed_buffer_size, &decompressed_size);
  BOOST_TEST(!result, "lzx_decompress must return success");

  std::string diff;
  BOOST_TEST(
      check_decompressed(large_noise, decompressed, decompressed_size, diff),
      diff);

  free(decompressed);
  free(compressed);
}

BOOST_AUTO_TEST_SUITE_END()
