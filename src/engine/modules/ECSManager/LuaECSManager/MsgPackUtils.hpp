#pragma once

#include <sol/sol.hpp>
#include <msgpack.hpp>

namespace rtypeEngine {
    void serializeToMsgPack(const sol::object& obj, msgpack::packer<msgpack::sbuffer>& pk);
    sol::object msgpackToLua(sol::state_view& lua, const msgpack::object& obj);
}
