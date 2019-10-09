/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/app/events.h>
#include <et/core/singleton.h>

namespace et {
class GameCenterPrivate;
class GameCenter : public Singleton<GameCenter> {
 public:
  enum AuthorizationStatus {
    AuthorizationStatus_NotAuthorized,
    AuthorizationStatus_Authorizing,
    AuthorizationStatus_Authorized,
  };

  struct Player {
    std::string playerId;
    std::string alias;
    std::string displayName;
  };

 public:
  AuthorizationStatus status() const;

  void authenticate();

  void showLeaderboards();
  void showLeaderboard(const std::string&);
  void reportScoreForLeaderboard(const std::string&, int64_t value);

  void showAchievements();
  void unlockAchievement(const std::string&);

  const Player& player() const {
    return _player;
  }

  ET_DECLARE_EVENT1(authorizationStatusChanged, AuthorizationStatus)
  ET_DECLARE_EVENT1(reportingFailed, std::string)
  ET_DECLARE_EVENT1(achievementFailedToUnlock, std::string)

 private:
  GameCenter();
  ~GameCenter();

  ET_SINGLETON_COPY_DENY(GameCenter)

  void authenticationCompleted();

 private:
  ET_DECLARE_PIMPL(GameCenter, 64)
  Player _player;
};
}  // namespace et
