#include "blusys/framework/capabilities/build_info.hpp"

namespace blusys {

build_info_capability::build_info_capability() : status_(make_build_info_status()) {}

build_info_capability::~build_info_capability() = default;

blusys_err_t build_info_capability::start(runtime &rt)
{
    rt_ = &rt;
    status_.capability_ready = true;
    return BLUSYS_OK;
}

void build_info_capability::poll(std::uint32_t /*now_ms*/)
{
}

void build_info_capability::stop()
{
    status_.capability_ready = false;
    rt_ = nullptr;
}

}  // namespace blusys
