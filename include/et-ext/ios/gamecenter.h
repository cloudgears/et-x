/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/singleton.h>

namespace et
{
	class GameCenterPrivate;
	class GameCenter : public Singleton<GameCenter>
	{
	public:
		void authenticate();
		
		void showLeaderboard(const std::string&);
		void reportScoreForLeaderboard(const std::string&, int64_t value);
		
		void showAchievements();
		void unlockAchievement(const std::string&);
		
	private:
		GameCenter();
		
		ET_SINGLETON_COPY_DENY(GameCenter)
				
	private:
		GameCenterPrivate* _private;
	};
}