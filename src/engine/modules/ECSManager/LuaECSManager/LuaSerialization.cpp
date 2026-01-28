#include "LuaECSManager.hpp"
#include <sstream>
#include <iostream>

namespace rtypeEngine {

std::string LuaECSManager::serializeTable(const sol::table &table) {
  std::stringstream ss;
  ss << "{";
  bool first = true;
  for (const auto &kv : table) {
    if (!first)
      ss << ",";
    first = false;

    if (kv.first.is<std::string>()) {
      ss << "[\"" << kv.first.as<std::string>() << "\"]=";
    } else if (kv.first.is<double>()) {
      ss << "[" << kv.first.as<int>() << "]=";
    }

    if (kv.second.is<std::string>()) {
      std::string str = kv.second.as<std::string>();
      ss << "\"";
      for (char c : str) {
        if (c == '"' || c == '\\')
          ss << '\\';
        ss << c;
      }
      ss << "\"";
    } else if (kv.second.is<double>()) {
      ss << kv.second.as<double>();
    } else if (kv.second.is<bool>()) {
      ss << (kv.second.as<bool>() ? "true" : "false");
    } else if (kv.second.is<sol::table>()) {
      ss << serializeTable(kv.second.as<sol::table>());
    } else {
      ss << "nil";
    }
  }
  ss << "}";
  return ss.str();
}

std::string LuaECSManager::serializeState() {
  std::stringstream ss;
  ss << "ENTITIES:";
  for (size_t i = 0; i < _entities.size(); ++i) {
    ss << _entities[i] << (i == _entities.size() - 1 ? "" : ",");
  }
  ss << ";\n";

  for (auto &pair : _pools) {
    const std::string &poolName = pair.first;
    ComponentPool &pool = pair.second;
    ss << "POOL:" << poolName << ";\n";
    for (size_t i = 0; i < pool.dense.size(); ++i) {
      std::string entityId = pool.entities[i];
      sol::table comp = pool.dense[i];
      std::string serializedComp;
      try {
        serializedComp = serializeTable(comp);
      } catch (const std::exception &e) {
        std::cerr << "[LuaECSManager] Error serializing component: " << e.what() << std::endl;
        continue;
      }
      ss << "COMP:" << entityId << ":" << serializedComp << ";\n";
    }
  }
  return ss.str();
}

void LuaECSManager::deserializeState(const std::string &state) {
  for (const auto &id : _entities) {
    sendMessage("PhysicCommand", "DestroyBody:" + id + ";");
    sendMessage("RenderEntityCommand", "DestroyEntity:" + id + ";");
  }

  _entities.clear();
  _pools.clear();

  std::stringstream ss(state);
  std::string line;
  std::string currentPoolName;

  while (std::getline(ss, line)) {
    if (line.empty())
      continue;
    if (line.back() == ';')
      line.pop_back();

    if (line.rfind("ENTITIES:", 0) == 0) {
      std::string entitiesStr = line.substr(9);
      std::stringstream ess(entitiesStr);
      std::string entityId;
      while (std::getline(ess, entityId, ',')) {
        if (!entityId.empty()) {
          _entities.push_back(entityId);
        }
      }
    } else if (line.rfind("POOL:", 0) == 0) {
      currentPoolName = line.substr(5);
    } else if (line.rfind("COMP:", 0) == 0) {
      if (currentPoolName.empty())
        continue;

      size_t split1 = line.find(':', 5);
      if (split1 == std::string::npos)
        continue;

      std::string entityId = line.substr(5, split1 - 5);
      std::string data = line.substr(split1 + 1);

      try {
        sol::table comp = _lua.script("return " + data);
        ComponentPool &pool = _pools[currentPoolName];

        pool.dense.push_back(comp);
        pool.entities.push_back(entityId);
        pool.sparse[entityId] = pool.dense.size() - 1;

      } catch (const sol::error &e) {
        std::cerr << "[LuaECSManager] Error deserializing component: " << e.what() << std::endl;
      }
    }
  }
  std::cout << "[LuaECSManager] State deserialized" << std::endl;
}

} // namespace rtypeEngine
