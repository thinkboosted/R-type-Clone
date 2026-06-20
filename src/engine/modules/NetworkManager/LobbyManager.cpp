#include "LobbyManager.hpp"

namespace rtypeEngine {

LobbyManager::LobbyManager(std::size_t minPlayers) : _minPlayers(minPlayers) {}

void LobbyManager::onClientConnected(uint32_t clientId) {
  std::lock_guard<std::mutex> lock(_mutex);
  auto &player = _players[clientId];
  player.connected = true;
  player.joined = false;
  player.ready = false;
}

void LobbyManager::onClientDisconnected(uint32_t clientId) {
  std::lock_guard<std::mutex> lock(_mutex);
  _players.erase(clientId);
  if (_players.empty()) {
    _gameStarted = false;
  }
}

void LobbyManager::onPlayerJoin(uint32_t clientId) {
  std::lock_guard<std::mutex> lock(_mutex);
  auto &player = _players[clientId];
  player.connected = true;
  player.joined = true;
}

void LobbyManager::onPlayerReady(uint32_t clientId) {
  std::lock_guard<std::mutex> lock(_mutex);
  auto it = _players.find(clientId);
  if (it == _players.end()) {
    return;
  }
  it->second.ready = true;
}

bool LobbyManager::shouldStartGame() const {
  std::lock_guard<std::mutex> lock(_mutex);
  if (_gameStarted) {
    return false;
  }
  if (connectedPlayerCountUnsafe() < _minPlayers) {
    return false;
  }
  return allConnectedPlayersReadyUnsafe();
}

void LobbyManager::setGameStarted(bool started) {
  std::lock_guard<std::mutex> lock(_mutex);
  _gameStarted = started;
  if (!started) {
    for (auto &[_, player] : _players) {
      player.ready = false;
    }
  }
}

bool LobbyManager::isGameStarted() const {
  std::lock_guard<std::mutex> lock(_mutex);
  return _gameStarted;
}

std::size_t LobbyManager::connectedPlayerCountUnsafe() const {
  std::size_t count = 0;
  for (const auto &[_, player] : _players) {
    if (player.connected) {
      ++count;
    }
  }
  return count;
}

bool LobbyManager::allConnectedPlayersReadyUnsafe() const {
  for (const auto &[_, player] : _players) {
    if (player.connected && (!player.joined || !player.ready)) {
      return false;
    }
  }
  return true;
}

} // namespace rtypeEngine
