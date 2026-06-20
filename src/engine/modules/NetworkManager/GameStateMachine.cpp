#include "GameStateMachine.hpp"
#include <iostream>

namespace rtypeEngine {

GameStateMachine::GameStateMachine() {
  _startingBeganAt = std::chrono::steady_clock::now();
}

GameState GameStateMachine::getGameState() const {
  std::lock_guard<std::mutex> lock(_mutex);
  return _gameState;
}

std::string GameStateMachine::getGameStateString() const {
  return stateToString(getGameState());
}

std::string GameStateMachine::stateToString(GameState state) {
  switch (state) {
  case GameState::LOBBY:
    return "LOBBY";
  case GameState::STARTING:
    return "STARTING";
  case GameState::IN_GAME:
    return "IN_GAME";
  case GameState::ENDING:
    return "ENDING";
  default:
    return "UNKNOWN";
  }
}

std::string GameStateMachine::playerStateToString(PlayerState state) {
  switch (state) {
  case PlayerState::CONNECTED:
    return "CONNECTED";
  case PlayerState::IN_LOBBY:
    return "IN_LOBBY";
  case PlayerState::READY:
    return "READY";
  case PlayerState::IN_GAME:
    return "IN_GAME";
  case PlayerState::DEAD:
    return "DEAD";
  case PlayerState::DISCONNECTED:
    return "DISCONNECTED";
  default:
    return "UNKNOWN";
  }
}

void GameStateMachine::onPlayerConnected(uint32_t clientId) {
  std::lock_guard<std::mutex> lock(_mutex);

  auto now = std::chrono::steady_clock::now();
  PlayerSession session;
  session.id = clientId;
  session.state = PlayerState::CONNECTED;
  session.ready = false;
  session.entityId = "";
  session.lastHeartbeat = now;
  session.connectedAt = now;

  _players[clientId] = session;
  std::cout << "[GameStateMachine] Player " << clientId << " connected" << std::endl;
}

void GameStateMachine::onPlayerDisconnected(uint32_t clientId) {
  std::lock_guard<std::mutex> lock(_mutex);

  auto it = _players.find(clientId);
  if (it != _players.end()) {
    PlayerState oldState = it->second.state;
    it->second.state = PlayerState::DISCONNECTED;
    it->second.ready = false;

    std::cout << "[GameStateMachine] Player " << clientId << " disconnected" << std::endl;

    if (_onPlayerStateChange) {
      _onPlayerStateChange(clientId, oldState, PlayerState::DISCONNECTED);
    }

    // Remove the player
    _players.erase(it);
  }

  // If no players left, reset to lobby
  if (_players.empty() && _gameState != GameState::LOBBY) {
    std::cout << "[GameStateMachine] No players left, resetting to LOBBY" << std::endl;
    _gameState = GameState::LOBBY;
  }
}

void GameStateMachine::onPlayerJoin(uint32_t clientId) {
  std::lock_guard<std::mutex> lock(_mutex);

  auto it = _players.find(clientId);
  if (it == _players.end()) {
    // Player not found, create a new session
    PlayerSession session;
    session.id = clientId;
    session.state = PlayerState::IN_LOBBY;
    session.ready = false;
    session.lastHeartbeat = std::chrono::steady_clock::now();
    session.connectedAt = std::chrono::steady_clock::now();
    _players[clientId] = session;
    std::cout << "[GameStateMachine] Player " << clientId << " joined lobby (new)" << std::endl;
    return;
  }

  if (_gameState == GameState::LOBBY || _gameState == GameState::ENDING) {
    setPlayerState(clientId, PlayerState::IN_LOBBY);
    it->second.ready = false;
    std::cout << "[GameStateMachine] Player " << clientId << " joined lobby" << std::endl;
  }
}

void GameStateMachine::onPlayerReady(uint32_t clientId) {
  std::lock_guard<std::mutex> lock(_mutex);

  auto it = _players.find(clientId);
  if (it == _players.end()) {
    return;
  }

  // Only accept ready in LOBBY state
  if (_gameState != GameState::LOBBY) {
    std::cout << "[GameStateMachine] Player " << clientId
              << " tried to ready but game state is " << stateToString(_gameState) << std::endl;
    return;
  }

  if (it->second.state == PlayerState::IN_LOBBY) {
    it->second.state = PlayerState::READY;
    it->second.ready = true;
    std::cout << "[GameStateMachine] Player " << clientId << " is READY" << std::endl;

    if (_onPlayerStateChange) {
      _onPlayerStateChange(clientId, PlayerState::IN_LOBBY, PlayerState::READY);
    }
  }
}

void GameStateMachine::onPlayerLeave(uint32_t clientId) {
  std::lock_guard<std::mutex> lock(_mutex);

  auto it = _players.find(clientId);
  if (it == _players.end()) {
    return;
  }

  PlayerState oldState = it->second.state;

  // If in game, mark as dead first
  if (oldState == PlayerState::IN_GAME) {
    it->second.state = PlayerState::DEAD;
    it->second.entityId = "";
    if (_onPlayerStateChange) {
      _onPlayerStateChange(clientId, oldState, PlayerState::DEAD);
    }
  }

  // Reset to lobby state
  it->second.state = PlayerState::IN_LOBBY;
  it->second.ready = false;
  it->second.entityId = "";

  std::cout << "[GameStateMachine] Player " << clientId << " left game, back to lobby"
            << std::endl;
}

void GameStateMachine::onPlayerHeartbeat(uint32_t clientId) {
  std::lock_guard<std::mutex> lock(_mutex);

  auto it = _players.find(clientId);
  if (it != _players.end()) {
    it->second.lastHeartbeat = std::chrono::steady_clock::now();
  }
}

void GameStateMachine::onPlayerDied(uint32_t clientId) {
  std::lock_guard<std::mutex> lock(_mutex);

  auto it = _players.find(clientId);
  if (it == _players.end()) {
    return;
  }

  if (it->second.state == PlayerState::IN_GAME) {
    PlayerState oldState = it->second.state;
    it->second.state = PlayerState::DEAD;
    it->second.entityId = "";

    std::cout << "[GameStateMachine] Player " << clientId << " died" << std::endl;

    if (_onPlayerStateChange) {
      _onPlayerStateChange(clientId, oldState, PlayerState::DEAD);
    }
  }
}

void GameStateMachine::setPlayerEntityId(uint32_t clientId, const std::string &entityId) {
  std::lock_guard<std::mutex> lock(_mutex);

  auto it = _players.find(clientId);
  if (it != _players.end()) {
    it->second.entityId = entityId;
  }
}

std::string GameStateMachine::getPlayerEntityId(uint32_t clientId) const {
  std::lock_guard<std::mutex> lock(_mutex);

  auto it = _players.find(clientId);
  if (it != _players.end()) {
    return it->second.entityId;
  }
  return "";
}

PlayerSession *GameStateMachine::getPlayer(uint32_t clientId) {
  std::lock_guard<std::mutex> lock(_mutex);

  auto it = _players.find(clientId);
  if (it != _players.end()) {
    return &it->second;
  }
  return nullptr;
}

const PlayerSession *GameStateMachine::getPlayer(uint32_t clientId) const {
  std::lock_guard<std::mutex> lock(_mutex);

  auto it = _players.find(clientId);
  if (it != _players.end()) {
    return &it->second;
  }
  return nullptr;
}

std::vector<uint32_t> GameStateMachine::getPlayersInState(PlayerState state) const {
  std::lock_guard<std::mutex> lock(_mutex);

  std::vector<uint32_t> result;
  for (const auto &[id, session] : _players) {
    if (session.state == state) {
      result.push_back(id);
    }
  }
  return result;
}

std::vector<uint32_t> GameStateMachine::getConnectedPlayerIds() const {
  std::lock_guard<std::mutex> lock(_mutex);

  std::vector<uint32_t> result;
  for (const auto &[id, session] : _players) {
    if (session.state != PlayerState::DISCONNECTED) {
      result.push_back(id);
    }
  }
  return result;
}

size_t GameStateMachine::getPlayerCount() const {
  std::lock_guard<std::mutex> lock(_mutex);

  size_t count = 0;
  for (const auto &[id, session] : _players) {
    if (session.state != PlayerState::DISCONNECTED) {
      ++count;
    }
  }
  return count;
}

size_t GameStateMachine::getAlivePlayerCount() const {
  std::lock_guard<std::mutex> lock(_mutex);

  size_t count = 0;
  for (const auto &[id, session] : _players) {
    if (session.state == PlayerState::IN_GAME) {
      ++count;
    }
  }
  return count;
}

bool GameStateMachine::isPlayerAlive(uint32_t clientId) const {
  std::lock_guard<std::mutex> lock(_mutex);

  auto it = _players.find(clientId);
  if (it != _players.end()) {
    return it->second.state == PlayerState::IN_GAME;
  }
  return false;
}

void GameStateMachine::update() {
  std::lock_guard<std::mutex> lock(_mutex);

  auto now = std::chrono::steady_clock::now();

  switch (_gameState) {
  case GameState::LOBBY:
    // Check if we should transition to STARTING
    if (shouldTransitionToStarting()) {
      _gameState = GameState::STARTING;
      _startingBeganAt = now;
      std::cout << "[GameStateMachine] Transitioning to STARTING" << std::endl;
      if (_onStateChange) {
        _onStateChange(GameState::LOBBY, GameState::STARTING);
      }
    }
    break;

  case GameState::STARTING:
    // Wait for STARTING_DELAY, then go to IN_GAME
    if (now - _startingBeganAt >= STARTING_DELAY) {
      // Move all READY players to IN_GAME
      for (auto &[id, session] : _players) {
        if (session.state == PlayerState::READY) {
          session.state = PlayerState::IN_GAME;
          session.lastHeartbeat = now;
          std::cout << "[GameStateMachine] Player " << id << " now IN_GAME" << std::endl;
        }
      }

      _gameState = GameState::IN_GAME;
      std::cout << "[GameStateMachine] Transitioning to IN_GAME" << std::endl;
      if (_onStateChange) {
        _onStateChange(GameState::STARTING, GameState::IN_GAME);
      }
    }
    break;

  case GameState::IN_GAME:
    // Check heartbeat timeouts
    for (auto &[id, session] : _players) {
      if (session.state == PlayerState::IN_GAME) {
        if (now - session.lastHeartbeat > HEARTBEAT_TIMEOUT) {
          std::cout << "[GameStateMachine] Player " << id << " timed out" << std::endl;
          session.state = PlayerState::DISCONNECTED;
          if (_onPlayerStateChange) {
            _onPlayerStateChange(id, PlayerState::IN_GAME, PlayerState::DISCONNECTED);
          }
        }
      }
    }

    // Check if game should end
    if (shouldTransitionToEnding()) {
      _gameState = GameState::ENDING;
      std::cout << "[GameStateMachine] Transitioning to ENDING" << std::endl;
      if (_onStateChange) {
        _onStateChange(GameState::IN_GAME, GameState::ENDING);
      }
    }
    break;

  case GameState::ENDING:
    // Reset all players to lobby
    resetAllPlayersToLobby();
    _gameState = GameState::LOBBY;
    std::cout << "[GameStateMachine] Transitioning to LOBBY" << std::endl;
    if (_onStateChange) {
      _onStateChange(GameState::ENDING, GameState::LOBBY);
    }
    break;
  }
}

void GameStateMachine::checkHeartbeatTimeouts() {
  // This is handled in update() for IN_GAME state
}

bool GameStateMachine::shouldTransitionToStarting() const {
  // Must be in LOBBY
  if (_gameState != GameState::LOBBY) {
    return false;
  }

  // Need at least MIN_PLAYERS
  size_t readyCount = 0;
  size_t totalInLobby = 0;

  for (const auto &[id, session] : _players) {
    if (session.state == PlayerState::READY) {
      ++readyCount;
      ++totalInLobby;
    } else if (session.state == PlayerState::IN_LOBBY) {
      ++totalInLobby;
    }
  }

  // All players in lobby must be ready, and at least MIN_PLAYERS
  return readyCount >= MIN_PLAYERS && readyCount == totalInLobby;
}

bool GameStateMachine::shouldTransitionToEnding() const {
  // Must be in IN_GAME
  if (_gameState != GameState::IN_GAME) {
    return false;
  }

  size_t aliveCount = 0;
  for (const auto &[id, session] : _players) {
    if (session.state == PlayerState::IN_GAME) {
      ++aliveCount;
    }
  }

  // End only when ALL players are dead (0 alive)
  return aliveCount == 0;
}

void GameStateMachine::forceEndGame() {
  std::lock_guard<std::mutex> lock(_mutex);

  if (_gameState == GameState::IN_GAME || _gameState == GameState::STARTING) {
    _gameState = GameState::ENDING;
    std::cout << "[GameStateMachine] Force ending game" << std::endl;
    if (_onStateChange) {
      _onStateChange(_gameState, GameState::ENDING);
    }
  }
}

void GameStateMachine::resetToLobby() {
  std::lock_guard<std::mutex> lock(_mutex);

  resetAllPlayersToLobby();
  _gameState = GameState::LOBBY;
  std::cout << "[GameStateMachine] Reset to LOBBY" << std::endl;
}

void GameStateMachine::reviveAllPlayers() {
  std::lock_guard<std::mutex> lock(_mutex);

  // Only revive during IN_GAME state
  if (_gameState != GameState::IN_GAME) {
    return;
  }

  auto now = std::chrono::steady_clock::now();
  for (auto &[id, session] : _players) {
    if (session.state == PlayerState::DEAD) {
      session.state = PlayerState::IN_GAME;
      session.lastHeartbeat = now;
      std::cout << "[GameStateMachine] Player " << id << " revived for new level" << std::endl;

      if (_onPlayerStateChange) {
        _onPlayerStateChange(id, PlayerState::DEAD, PlayerState::IN_GAME);
      }
    }
  }
}

void GameStateMachine::setStateChangeCallback(StateChangeCallback cb) {
  std::lock_guard<std::mutex> lock(_mutex);
  _onStateChange = std::move(cb);
}

void GameStateMachine::setPlayerStateChangeCallback(PlayerStateChangeCallback cb) {
  std::lock_guard<std::mutex> lock(_mutex);
  _onPlayerStateChange = std::move(cb);
}

void GameStateMachine::transitionTo(GameState newState) {
  if (_gameState != newState) {
    GameState oldState = _gameState;
    _gameState = newState;
    if (_onStateChange) {
      _onStateChange(oldState, newState);
    }
  }
}

void GameStateMachine::setPlayerState(uint32_t clientId, PlayerState newState) {
  auto it = _players.find(clientId);
  if (it != _players.end() && it->second.state != newState) {
    PlayerState oldState = it->second.state;
    it->second.state = newState;
    if (_onPlayerStateChange) {
      _onPlayerStateChange(clientId, oldState, newState);
    }
  }
}

void GameStateMachine::resetAllPlayersToLobby() {
  for (auto &[id, session] : _players) {
    if (session.state != PlayerState::DISCONNECTED) {
      session.state = PlayerState::IN_LOBBY;
      session.ready = false;
      session.entityId = "";
    }
  }
  std::cout << "[GameStateMachine] All players reset to IN_LOBBY" << std::endl;
}

bool GameStateMachine::allPlayersReady() const {
  for (const auto &[id, session] : _players) {
    if (session.state == PlayerState::IN_LOBBY && !session.ready) {
      return false;
    }
  }
  return true;
}

size_t GameStateMachine::readyPlayerCount() const {
  size_t count = 0;
  for (const auto &[id, session] : _players) {
    if (session.state == PlayerState::READY) {
      ++count;
    }
  }
  return count;
}

} // namespace rtypeEngine
