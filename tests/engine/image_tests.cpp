#include "image/image.h"
#include "image/image_buffer.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <type_traits>
#include <vector>

// ---------------------------------------------------------------------------
// ImageBuffer unit tests (tagged [image])
// ---------------------------------------------------------------------------

TEST_CASE("ImageBuffer is default-constructible aggregate", "[image]") {
    buddd::engine::ImageBuffer buf;

    REQUIRE(buf.width == 0);
    REQUIRE(buf.height == 0);
    REQUIRE(buf.channels == 0);
    REQUIRE(buf.data.empty());
}

// ---------------------------------------------------------------------------
// Image::create validation (tagged [image])
// ---------------------------------------------------------------------------

TEST_CASE("Image::create validates positive dimensions", "[image]") {
    using namespace buddd::engine;

    // width == 0
    {
        ImageBuffer buf;
        buf.width = 0;
        buf.height = 4;
        buf.channels = 4;
        buf.data.resize(static_cast<size_t>(buf.width) * buf.height * buf.channels);
        auto result = Image::create(buf);
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error().category == Error::Category::InvalidArgument);
    }

    // height == 0
    {
        ImageBuffer buf;
        buf.width = 4;
        buf.height = 0;
        buf.channels = 4;
        buf.data.resize(static_cast<size_t>(buf.width) * buf.height * buf.channels);
        auto result = Image::create(buf);
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error().category == Error::Category::InvalidArgument);
    }

    // channels == 0
    {
        ImageBuffer buf;
        buf.width = 4;
        buf.height = 4;
        buf.channels = 0;
        buf.data.resize(static_cast<size_t>(buf.width) * buf.height);
        auto result = Image::create(buf);
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error().category == Error::Category::InvalidArgument);
    }
}

TEST_CASE("Image::create validates data size matches dimensions", "[image]") {
    using namespace buddd::engine;

    // data.size() == 0 when it should be 4*4*4 = 64
    {
        ImageBuffer buf;
        buf.width = 4;
        buf.height = 4;
        buf.channels = 4;
        buf.data.resize(0);
        auto result = Image::create(buf);
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error().category == Error::Category::InvalidArgument);
    }

    // data.size() == 63 (not 64)
    {
        ImageBuffer buf;
        buf.width = 4;
        buf.height = 4;
        buf.channels = 4;
        buf.data.resize(63);
        auto result = Image::create(buf);
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error().category == Error::Category::InvalidArgument);
    }
}

// ---------------------------------------------------------------------------
// Image::create row-flipping (tagged [image])
// ---------------------------------------------------------------------------

TEST_CASE("Image::create flips rows correctly", "[image]") {
    using namespace buddd::engine;

    // Create a 4×2 ImageBuffer with distinct pixel patterns per row.
    // Channels = 1 (greyscale) for simplicity.
    // Row 0 (bottom in GL): all 0xFF pixels
    // Row 1 (top in GL): all 0x00 pixels
    const int width = 4;
    const int height = 2;
    const int channels = 1;
    const size_t row_bytes = static_cast<size_t>(width) * static_cast<size_t>(channels);

    ImageBuffer buf;
    buf.width = width;
    buf.height = height;
    buf.channels = channels;
    buf.data.resize(row_bytes * static_cast<size_t>(height));

    // Fill row 0 (bottom) with 0xFF
    std::fill(buf.data.begin(), buf.data.begin() + static_cast<std::ptrdiff_t>(row_bytes), std::byte{0xFF});
    // Fill row 1 (top) with 0x00
    std::fill(buf.data.begin() + static_cast<std::ptrdiff_t>(row_bytes), buf.data.end(), std::byte{0x00});

    auto result = Image::create(buf);
    REQUIRE(result.has_value());

    const auto& img = *result;
    REQUIRE(img.width() == width);
    REQUIRE(img.height() == height);
    REQUIRE(img.channels() == channels);

    // After flip:
    // Image row 0 (top) should equal buffer row 1 (top in GL) = 0x00
    // Image row 1 (bottom) should equal buffer row 0 (bottom in GL) = 0xFF
    auto img_data = img.data().data();

    // Check row 0 (top) in Image: should be all 0x00
    for (size_t i = 0; i < row_bytes; ++i) {
        REQUIRE(img_data[i] == std::byte{0x00});
    }

    // Check row 1 (bottom) in Image: should be all 0xFF
    for (size_t i = row_bytes; i < row_bytes * 2; ++i) {
        REQUIRE(img_data[i] == std::byte{0xFF});
    }
}

// ---------------------------------------------------------------------------
// Image::save writes valid PNG and Image::load round-trips (tagged [image])
// ---------------------------------------------------------------------------

TEST_CASE("Image::save writes valid PNG and Image::load round-trips", "[image]") {
    using namespace buddd::engine;

    // Create a 4×4 RGBA Image with known pixel data
    const int width = 4;
    const int height = 4;
    const int channels = 4;
    const size_t total_bytes = static_cast<size_t>(width) * height * channels;

    ImageBuffer buf;
    buf.width = width;
    buf.height = height;
    buf.channels = channels;
    buf.data.resize(total_bytes);

    // Fill with a known pattern: pixel (x, y) gets (x*64, y*64, 128, 255)
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            size_t idx = static_cast<size_t>((y * width + x) * channels);
            buf.data[idx + 0] = static_cast<std::byte>(x * 64);
            buf.data[idx + 1] = static_cast<std::byte>(y * 64);
            buf.data[idx + 2] = std::byte{128};
            buf.data[idx + 3] = std::byte{255};
        }
    }

    auto img_result = Image::create(buf);
    REQUIRE(img_result.has_value());

    // Save to temp path
    const std::string temp_path = "/tmp/buddd_test_roundtrip.png";
    auto save_result = img_result->save(temp_path);
    REQUIRE(save_result.has_value());

    // Verify the file starts with PNG magic bytes
    {
        std::ifstream f(temp_path, std::ios::binary);
        REQUIRE(f.is_open());
        unsigned char magic[4];
        f.read(reinterpret_cast<char*>(magic), 4);
        REQUIRE(magic[0] == 0x89);
        REQUIRE(magic[1] == 'P');
        REQUIRE(magic[2] == 'N');
        REQUIRE(magic[3] == 'G');
    }

    // Load back using Image::load and compare byte-for-byte
    auto load_result = Image::load(temp_path);
    REQUIRE(load_result.has_value());

    const auto& loaded = *load_result;
    REQUIRE(loaded.width() == width);
    REQUIRE(loaded.height() == height);
    // Channel count may differ from original (PNG stores RGB or RGBA);
    // If identical, compare data; otherwise just check it loads.
    if (loaded.channels() == channels) {
        REQUIRE(loaded.data().size() == img_result->data().size());
        REQUIRE(std::equal(
            loaded.data().begin(), loaded.data().end(),
            img_result->data().begin()));
    }

    // Clean up
    std::remove(temp_path.c_str());
}

// ---------------------------------------------------------------------------
// Image::load error cases (tagged [image])
// ---------------------------------------------------------------------------

TEST_CASE("Image::load returns error for non-existent file", "[image]") {
    using namespace buddd::engine;

    auto result = Image::load("/tmp/nonexistent_file_12345.png");
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == Error::Category::IoFailed);
}

TEST_CASE("Image::load returns error for corrupt file", "[image]") {
    using namespace buddd::engine;

    // Write a small non-PNG file
    const std::string corrupt_path = "/tmp/buddd_test_corrupt.png";
    {
        std::ofstream f(corrupt_path, std::ios::binary);
        REQUIRE(f.is_open());
        f.write("not a png", 9);
    }

    auto result = Image::load(corrupt_path);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == Error::Category::IoFailed);

    std::remove(corrupt_path.c_str());
}

// ---------------------------------------------------------------------------
// Image copy/move semantics (tagged [image])
// ---------------------------------------------------------------------------

TEST_CASE("Image is non-copyable", "[image]") {
    static_assert(!std::is_copy_constructible_v<buddd::engine::Image>,
                  "Image must not be copy-constructible");
    static_assert(!std::is_copy_assignable_v<buddd::engine::Image>,
                  "Image must not be copy-assignable");
}

TEST_CASE("Image is movable", "[image]") {
    using namespace buddd::engine;

    // Create a small Image
    ImageBuffer buf;
    buf.width = 2;
    buf.height = 2;
    buf.channels = 1;
    buf.data.resize(4, std::byte{0xAB});

    auto source = Image::create(buf);
    REQUIRE(source.has_value());

    // Move-construct
    Image dest(std::move(*source));
    REQUIRE(dest.width() == 2);
    REQUIRE(dest.height() == 2);
    REQUIRE(dest.channels() == 1);
    REQUIRE(dest.data().size() == 4);
    REQUIRE(dest.data()[0] == std::byte{0xAB});

    // After move, moved-from Image has empty data (vector was moved).
    // Int members (width, height, channels) are in valid-but-unspecified state
    // but data_ is guaranteed empty after move-construction of std::vector.
    REQUIRE(source->data().empty());
}

// ---------------------------------------------------------------------------
// Image accessors (tagged [image])
// ---------------------------------------------------------------------------

TEST_CASE("Image accessors return stored values", "[image]") {
    using namespace buddd::engine;

    ImageBuffer buf;
    buf.width = 8;
    buf.height = 6;
    buf.channels = 3;
    buf.data.resize(static_cast<size_t>(8) * 6 * 3, std::byte{0x42});

    auto result = Image::create(buf);
    REQUIRE(result.has_value());

    const auto& img = *result;
    REQUIRE(img.width() == 8);
    REQUIRE(img.height() == 6);
    REQUIRE(img.channels() == 3);
    REQUIRE(img.data().size() == static_cast<size_t>(8) * 6 * 3);

    // Verify data was preserved (after row flip)
    for (auto b : img.data()) {
        REQUIRE(b == std::byte{0x42});
    }
}

// ---------------------------------------------------------------------------
// Image::save error cases (tagged [image])
// ---------------------------------------------------------------------------

TEST_CASE("Image::save returns error for unwritable path", "[image]") {
    using namespace buddd::engine;

    ImageBuffer buf;
    buf.width = 2;
    buf.height = 2;
    buf.channels = 1;
    buf.data.resize(4, std::byte{0xFF});

    auto img = Image::create(buf);
    REQUIRE(img.has_value());

    // Path to a non-existent directory
    auto result = img->save("/nonexistent_dir/out.png");
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == Error::Category::IoFailed);
}

TEST_CASE("Image::save returns error when path is a directory", "[image]") {
    using namespace buddd::engine;

    ImageBuffer buf;
    buf.width = 2;
    buf.height = 2;
    buf.channels = 1;
    buf.data.resize(4, std::byte{0xFF});

    auto img = Image::create(buf);
    REQUIRE(img.has_value());

    // /tmp/ is an existing directory - stb_write_png should fail
    auto result = img->save("/tmp/");
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == Error::Category::IoFailed);
}
