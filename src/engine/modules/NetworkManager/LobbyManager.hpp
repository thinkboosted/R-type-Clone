#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <unordered_map>

namespace rtypeEngine {

class LobbyManager {
public:
  struct LobbyPlayer {
    bool connected = false;
    bool joined = false;
    bool ready = false;
  };

  explicit LobbyManager(std::size_t minPlayers = 2);

  void onClientConnected(uint32_t clientId);
  void onClientDisconnected(uint32_t clientId);
  void onPlayerJoin(uint32_t clientId);
  void onPlayerReady(uint32_t clientId);

  bool shouldStartGame() const;
  void setGameStarted(bool started);
  bool isGameStarted() const;

private:
  std::size_t connectedPlayerCountUnsafe() const;
  bool allConnectedPlayersReadyUnsafe() const;

  mutable std::mutex _mutex;
  std::unordered_map<uint32_t, LobbyPlayer> _players;
  std::size_t _minPlayers;
  bool _gameStarted = false;
};

} // namespace rtypeEngine
