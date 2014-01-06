/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <fstream>
#include <GameKit/GameKit.h>

#include <et/app/application.h>
#include <et/app/applicationnotifier.h>
#include <et-ext/ios/gamecenter.h>
#include <et-ext/json/json.h>

const std::string kReportedScores = "reported-scores";
const std::string kUnlockedAchievements = "unlocked-achievements";
const std::string kLeaderboardId = "leaderboard-id";
const std::string kAchievementId = "archievement-id";
const std::string kValue = "value";
const std::string kValueLow = "value-low";
const std::string kValueHigh = "value-high";
const std::string kCompleted = "completed";

using namespace et;

@interface SharedGameCenterDelegate : NSObject <GKGameCenterControllerDelegate,
	GKLeaderboardViewControllerDelegate, GKAchievementViewControllerDelegate>

+ (instancetype)instance;

- (void)showAchievements;
- (void)showLeaderboard:(NSString*)leaderboardId;

@end

class et::GameCenterPrivate
{
public:
	std::string optionsFileName;
	
	Dictionary options;
	GameCenter::AuthorizationStatus status = GameCenter::AuthorizationStatus_NotAuthorized;
	
public:
	std::string hashForScore(Dictionary score);
	
	void setScore(const std::string& lbUd, int64_t value, bool completed);
	void setAchievement(const std::string& aId, bool completed);
	
	void saveOptions();
};

GameCenter::GameCenter() :
	_private(new GameCenterPrivate)
{
	_private->optionsFileName = application().environment().applicationDocumentsFolder() + "gamecenter.options.json";
	if (fileExists(_private->optionsFileName))
	{
		ValueClass vc = ValueClass_Invalid;
		auto opts = json::deserialize(loadTextFile(_private->optionsFileName), vc);
		if (vc == ValueClass_Dictionary)
			_private->options = opts;
	}
	
	authenticate();
}

GameCenter::AuthorizationStatus GameCenter::status() const
{
	return _private->status;
}

void GameCenter::authenticate()
{
	GKLocalPlayer* player = [GKLocalPlayer localPlayer];
	player.authenticateHandler = ^(UIViewController* viewController, NSError* error)
	{
		if (viewController != nil)
		{
			_private->status = AuthorizationStatus_Authorizing;
			
			UIViewController* mainViewController =
				reinterpret_cast<UIViewController*>(application().renderingContextHandle());
			[mainViewController presentViewController:viewController animated:YES completion:nil];
		}
		else
		{
			if (player.authenticated)
			{
				_private->status = AuthorizationStatus_Authorized;
				authenticationCompleted();
			}
			else
			{
				_private->status = AuthorizationStatus_NotAuthorized;
				NSLog(@"Unable to authenticate to Game Center\n%@", error);
			}
		}
		
		authorizationStatusChanged.invokeInMainRunLoop(_private->status);
	};
}

void GameCenter::showLeaderboard(const std::string& ldId)
{
	if ([[GKLocalPlayer localPlayer] isAuthenticated])
	{
		[[SharedGameCenterDelegate instance] performSelectorOnMainThread:@selector(showLeaderboard:)
			withObject:[NSString stringWithUTF8String:ldId.c_str()] waitUntilDone:NO];
	}
}

void GameCenter::reportScoreForLeaderboard(const std::string& lId, int64_t value)
{
	if ([[GKLocalPlayer localPlayer] isAuthenticated])
	{
		GKScore* score = [[GKScore alloc] initWithCategory:[NSString stringWithUTF8String:lId.c_str()]];
		score.context = 0;
		score.value = value;
		[score reportScoreWithCompletionHandler:^(NSError *error)
		{
			if (error == nil)
			{
				_private->setScore(lId, value, true);
			}
			else
			{
				NSLog(@"Unable to report score to %s\n%@", lId.c_str(), error);
				_private->setScore(lId, value, false);
				reportingFailed.invokeInMainRunLoop(lId);
			}
		}];
	}
	else
	{
		_private->setScore(lId, value, false);
		reportingFailed.invokeInMainRunLoop(lId);
	}
}

void GameCenter::showAchievements()
{
	if ([[GKLocalPlayer localPlayer] isAuthenticated])
	{
		[[SharedGameCenterDelegate instance] performSelectorOnMainThread:@selector(showAchievements)
			withObject:nil waitUntilDone:NO];
	}
	else
	{
		authenticate();
	}
}

void GameCenter::unlockAchievement(const std::string& achId)
{
	if ([[GKLocalPlayer localPlayer] isAuthenticated])
	{
		std::string localId = achId;
		GKAchievement* ach = [[GKAchievement alloc] initWithIdentifier:[NSString stringWithUTF8String:achId.c_str()]];
		ach.percentComplete = 100.0;
		ach.showsCompletionBanner = YES;
		
		[ach reportAchievementWithCompletionHandler:^(NSError *error)
		{
			if (error == nil)
			{
				_private->setAchievement(localId, true);
			}
			else
			{
				_private->setAchievement(localId, false);
				NSLog(@"Unable to report achievement %s\n%@", localId.c_str(), error);
				achievementFailedToUnlock.invokeInMainRunLoop(localId);
			}
		}];
	}
	else
	{
		_private->setAchievement(achId, false);
		achievementFailedToUnlock.invokeInMainRunLoop(achId);
	}
}

void GameCenter::authenticationCompleted()
{
	Dictionary achievements = _private->options.dictionaryForKey(kUnlockedAchievements);
	for (auto& p : achievements->content)
	{
		int completed = IntegerValue(p.second)->content;
		std::string achievementId = p.first;
		Invocation i;
		i.setTarget([this, achievementId, completed]()
		{
			if (completed == 0)
				unlockAchievement(achievementId);
			else
				this->_private->setAchievement(achievementId, 1);
		});
		i.invokeInMainRunLoop();
	}
	
	Dictionary scores = _private->options.dictionaryForKey(kReportedScores);
	for (auto& p : scores->content)
	{
		Dictionary score = p.second;
		int64_t valueLow = score.integerForKey(kValueLow)->content;
		int64_t valueHigh = score.integerForKey(kValueHigh)->content;
		int64_t value = valueLow | (valueHigh << 32);
		
		Invocation i;
		i.setTarget([this, score, value]()
		{
			if (score.integerForKey(kCompleted)->content == 0)
				reportScoreForLeaderboard(score.stringForKey(kLeaderboardId)->content, value);
			else
				_private->setScore(score.stringForKey(kLeaderboardId)->content, value, true);
		});
		i.invokeInMainRunLoop();
	}
}

/*
 * Private
 */
void GameCenterPrivate::setScore(const std::string& lbUd, int64_t value, bool completed)
{
	Dictionary score;
	score.setStringForKey(kLeaderboardId, lbUd);
	score.setIntegerForKey(kCompleted, completed ? 1 : 0);
	score.setIntegerForKey(kValueLow, static_cast<int32_t>(value & 0x00000000ffffffff));
	score.setIntegerForKey(kValueHigh, static_cast<int32_t>((value & 0xffffffff00000000) >> 32));
	std::string scoreId = hashForScore(score);
	
	Dictionary scores = options.dictionaryForKey(kReportedScores);
	
	if (completed)
		scores.removeObjectForKey(scoreId);
	else
		scores.setDictionaryForKey(scoreId, score);
	
	options.setDictionaryForKey(kReportedScores, scores);
	saveOptions();
}

void GameCenterPrivate::setAchievement(const std::string& aId, bool completed)
{
	Dictionary achievements = options.dictionaryForKey(kUnlockedAchievements);
	
	if (completed)
		achievements.removeObjectForKey(aId);
	else
		achievements.setIntegerForKey(aId, 0);
	
	options.setDictionaryForKey(kUnlockedAchievements, achievements);
	saveOptions();
}

void GameCenterPrivate::saveOptions()
{
	std::string tempFileName = optionsFileName + ".tmp";
	
	std::ofstream outFile(tempFileName, std::ios::out);
	outFile << json::serialize(options);
	outFile.flush();
	outFile.close();
	
	if (rename(tempFileName.c_str(), optionsFileName.c_str()))
	{
		removeFile(optionsFileName);
		if (rename(tempFileName.c_str(), optionsFileName.c_str()))
			log::error("Unable to save Game Center options to file %s", optionsFileName.c_str());
	}
}

std::string GameCenterPrivate::hashForScore(Dictionary score)
{
	return "score-" + score.stringForKey(kLeaderboardId)->content + "-" +
		intToStr(score.integerForKey(kValueLow)->content) + "-" + intToStr(score.integerForKey(kValueHigh)->content);
}

/*
 * Obj-C implementation
 */
@implementation SharedGameCenterDelegate

+ (instancetype)instance
{
	static SharedGameCenterDelegate* sharedInstance = nil;
	static dispatch_once_t onceToken = 0;
	dispatch_once(&onceToken, ^{
		sharedInstance = [[SharedGameCenterDelegate alloc] init];
	});
	return sharedInstance;
}

- (void)gameCenterViewControllerDidFinish:(GKGameCenterViewController*)gameCenterViewController
{
	UIViewController* mainViewController =
		reinterpret_cast<UIViewController*>(application().renderingContextHandle());
	[mainViewController dismissViewControllerAnimated:YES completion:nil];
}

- (void)leaderboardViewControllerDidFinish:(GKLeaderboardViewController*)viewController
{
	[self gameCenterViewControllerDidFinish:viewController];
}

- (void)achievementViewControllerDidFinish:(GKAchievementViewController*)viewController
{
	[self gameCenterViewControllerDidFinish:viewController];
}

- (void)showAchievements
{
	UIViewController* mainViewController =
		reinterpret_cast<UIViewController*>(application().renderingContextHandle());
	
	GKAchievementViewController* gcController = [[GKAchievementViewController alloc] init];
	if (gcController != nil)
	{
		gcController.gameCenterDelegate = [SharedGameCenterDelegate instance];
		gcController.achievementDelegate = [SharedGameCenterDelegate instance];
		[mainViewController presentViewController:gcController animated:YES completion:nil];
	}
	[gcController release];
}

- (void)showLeaderboard:(NSString*)leaderboardId
{
	UIViewController* mainViewController =
		reinterpret_cast<UIViewController*>(application().renderingContextHandle());
	
	[GKLeaderboard loadLeaderboardsWithCompletionHandler:^(NSArray *leaderboards, NSError *error)
	{
		if (error != nil)
		{
			NSLog(@"Unable to load leaderboards\n%@", error);
		}
		else if (leaderboards.count == 0)
		{
			NSLog(@"There is no leaderboards for this application");
		}
		else
		{
			GKLeaderboardViewController* gcController = [[GKLeaderboardViewController alloc] init];
			if (gcController != nil)
			{
				for (GKLeaderboard* leaderboard in leaderboards)
				{
					NSString* lbId = [leaderboard respondsToSelector:@selector(identifier)] ?
						leaderboard.identifier : leaderboard.category;
										
					if ([lbId isEqualToString:leaderboardId])
					{
						gcController.leaderboardCategory = leaderboard.category;
						
						gcController.leaderboardTimeScope = leaderboard.timeScope;
						
						if ([gcController respondsToSelector:@selector(setLeaderboardIdentifier:)])
							gcController.leaderboardIdentifier = leaderboardId;
						
						break;
					}
				}
				
				gcController.gameCenterDelegate = [SharedGameCenterDelegate instance];
				gcController.leaderboardDelegate = [SharedGameCenterDelegate instance];
				[mainViewController presentViewController:gcController animated:YES completion:nil];
			}
			[gcController release];
		}
	}];
}

@end
