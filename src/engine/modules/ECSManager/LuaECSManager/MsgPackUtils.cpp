#include "MsgPackUtils.hpp"
#include <iostream>
#include <cmath>
#include <limits>

namespace rtypeEngine {

    void serializeToMsgPack(const sol::object& obj, msgpack::packer<msgpack::sbuffer>& pk) {
        switch (obj.get_type()) {
            case sol::type::nil:
                pk.pack_nil();
                break;
            case sol::type::boolean:
                pk.pack(obj.as<bool>());
                break;
            case sol::type::number:
                // Check if integer or float
                if (obj.is<double>()) {
                    double val = obj.as<double>();
                    // Robust check: is it effectively an integer?
                    if (std::floor(val) == val && val >= static_cast<double>(std::numeric_limits<int64_t>::min()) && val <= static_cast<double>(std::numeric_limits<int64_t>::max())) {
                        pk.pack(static_cast<int64_t>(val));
                    } else {
                        pk.pack(val);
                    }
                } else if (obj.is<int64_t>()) {
                     pk.pack(obj.as<int64_t>());
                } else {
                     pk.pack(obj.as<double>());
                }
                break;
            case sol::type::string:
                pk.pack(obj.as<std::string>());
                break;
            case sol::type::table: {
                sol::table tbl = obj.as<sol::table>();
                // Basic heuristic: if keys are 1..N sequential, it's an array. Else map.
                // For simplicity/safety in game dev, often treating everything as map is safer unless we check keys.
                // But for packed size, array is better. Let's check size.
                size_t sz = tbl.size();
                bool isArray = true;
                if (sz == 0) {
                     // Empty table, check if it has any keys
                     if (tbl.begin() != tbl.end()) isArray = false;
                } else {
                     // Check if keys are 1..sz
                     for (size_t i = 1; i <= sz; ++i) {
                         if (!tbl[i].valid()) {
                             isArray = false;
                             break;
                         }
                     }
                }

                if (isArray) {
                    pk.pack_array(sz);
                    for (size_t i = 1; i <= sz; ++i) {
                        serializeToMsgPack(tbl[i], pk);
                    }
                } else {
                    // Count keys manually for map
                    size_t mapSize = 0;
                    for (auto kv : tbl) mapSize++;
                    pk.pack_map(mapSize);
                    for (auto kv : tbl) {
                        serializeToMsgPack(kv.first, pk);
                        serializeToMsgPack(kv.second, pk);
                    }
                }
                break;
            }
            default:
                std::cerr << "[MsgPackUtils] Warning: Unsupported type for MsgPack serialization" << std::endl;
                pk.pack_nil();
                break;
        }
    }

    sol::object msgpackToLua(sol::state_view& lua, const msgpack::object& obj) {
        switch (obj.type) {
            case msgpack::type::NIL:
                return sol::make_object(lua, sol::nil);
            case msgpack::type::BOOLEAN:
                return sol::make_object(lua, obj.via.boolean);
            case msgpack::type::POSITIVE_INTEGER:
                return sol::make_object(lua, obj.via.u64);
            case msgpack::type::NEGATIVE_INTEGER:
                return sol::make_object(lua, obj.via.i64);
            case msgpack::type::FLOAT32:
            case msgpack::type::FLOAT64:
                return sol::make_object(lua, obj.via.f64);
            case msgpack::type::STR:
                return sol::make_object(lua, std::string(obj.via.str.ptr, obj.via.str.size));
            case msgpack::type::BIN:
                return sol::make_object(lua, std::string(obj.via.bin.ptr, obj.via.bin.size));
            case msgpack::type::ARRAY: {
                sol::table tbl = lua.create_table();
                for (uint32_t i = 0; i < obj.via.array.size; ++i) {
                    tbl[i + 1] = msgpackToLua(lua, obj.via.array.ptr[i]);
                }
                return tbl;
            }
            case msgpack::type::MAP: {
                sol::table tbl = lua.create_table();
                for (uint32_t i = 0; i < obj.via.map.size; ++i) {
                    auto key = msgpackToLua(lua, obj.via.map.ptr[i].key);
                    auto val = msgpackToLua(lua, obj.via.map.ptr[i].val);
                    tbl[key] = val;
                }
                return tbl;
            }
            default:
                return sol::make_object(lua, sol::nil);
        }
    }
}
