#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace rtypeEngine {

/**
 * @brief Global game states for server-authoritative multiplayer
 */
enum class GameState { LOBBY, STARTING, IN_GAME, ENDING };

/**
 * @brief Individual player states
 */
enum class PlayerState { CONNECTED, IN_LOBBY, READY, IN_GAME, DEAD, DISCONNECTED };

/**
 * @brief Player session data
 */
struct PlayerSession {
  uint32_t id = 0;
  PlayerState state = PlayerState::CONNECTED;
  bool ready = false;
  std::string entityId;
  std::chrono::steady_clock::time_point lastHeartbeat;
  std::chrono::steady_clock::time_point connectedAt;
};

/**
 * @brief Server-side game state machine
 *
 * Manages the game lifecycle: LOBBY -> STARTING -> IN_GAME -> ENDING -> LOBBY
 * Handles player sessions, ready states, and game transitions.
 */
class GameStateMachine {
public:
  using StateChangeCallback = std::function<void(GameState oldState, GameState newState)>;
  using PlayerStateChangeCallback =
      std::function<void(uint32_t playerId, PlayerState oldState, PlayerState newState)>;

  static constexpr size_t MIN_PLAYERS = 2;
  static constexpr auto HEARTBEAT_TIMEOUT = std::chrono::seconds(3);
  static constexpr auto STARTING_DELAY = std::chrono::seconds(3);

  GameStateMachine();

  // State queries
  GameState getGameState() const;
  std::string getGameStateString() const;
  static std::string stateToString(GameState state);
  static std::string playerStateToString(PlayerState state);

  // Player management
  void onPlayerConnected(uint32_t clientId);
  void onPlayerDisconnected(uint32_t clientId);
  void onPlayerJoin(uint32_t clientId);
  void onPlayerReady(uint32_t clientId);
  void onPlayerLeave(uint32_t clientId);
  void onPlayerHeartbeat(uint32_t clientId);
  void onPlayerDied(uint32_t clientId);

  // Entity assignment
  void setPlayerEntityId(uint32_t clientId, const std::string &entityId);
  std::string getPlayerEntityId(uint32_t clientId) const;

  // State queries
  PlayerSession *getPlayer(uint32_t clientId);
  const PlayerSession *getPlayer(uint32_t clientId) const;
  std::vector<uint32_t> getPlayersInState(PlayerState state) const;
  std::vector<uint32_t> getConnectedPlayerIds() const;
  size_t getPlayerCount() const;
  size_t getAlivePlayerCount() const;
  bool isPlayerAlive(uint32_t clientId) const;

  // Game flow control
  void update();
  void checkHeartbeatTimeouts();
  bool shouldTransitionToStarting() const;
  bool shouldTransitionToEnding() const;
  void forceEndGame();
  void resetToLobby();
  void reviveAllPlayers(); // Revive dead players for new level

  // Callbacks
  void setStateChangeCallback(StateChangeCallback cb);
  void setPlayerStateChangeCallback(PlayerStateChangeCallback cb);

private:
  void transitionTo(GameState newState);
  void setPlayerState(uint32_t clientId, PlayerState newState);
  void resetAllPlayersToLobby();
  bool allPlayersReady() const;
  size_t readyPlayerCount() const;

  mutable std::mutex _mutex;
  GameState _gameState = GameState::LOBBY;
  std::unordered_map<uint32_t, PlayerSession> _players;
  std::chrono::steady_clock::time_point _startingBeganAt;

  StateChangeCallback _onStateChange;
  PlayerStateChangeCallback _onPlayerStateChange;
};

} // namespace rtypeEngine
