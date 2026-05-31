#include "render/texture_headless.h"

namespace buddd::engine {

TextureHeadless::TextureHeadless(int width, int height, int channels, std::vector<std::byte> data) noexcept
    : width_(width)
    , height_(height)
    , channels_(channels)
    , data_(std::move(data))
{}

} // namespace buddd::engine
