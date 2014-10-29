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
#include <et/platform-apple/apple.h>
#include <et-ext/platform-apple/gamecenter.h>
#include <et-ext/json/json.h>

#if (TARGET_OS_IPHONE)
#	define ViewControllerClass UIViewController
#else
#	define ViewControllerClass NSViewController
#endif

const std::string kReportedScores = "reported-scores";
const std::string kUnlockedAchievements = "unlocked-achievements";
const std::string kLeaderboardId = "leaderboard-id";
const std::string kAchievementId = "archievement-id";
const std::string kValue = "value";
const std::string kValueLow = "value-low";
const std::string kValueHigh = "value-high";
const std::string kCompleted = "completed";

using namespace et;

@interface SharedGameCenterDelegate : NSObject <GKGameCenterControllerDelegate>

+ (instancetype)instance;

- (void)showAchievements;
- (void)showLeaderboards;
- (void)showLeaderboard:(NSString*)leaderboardId;

@end

class et::GameCenterPrivate
{
public:
	std::string optionsFileName;
	
	Dictionary options;
	GameCenter::AuthorizationStatus status = GameCenter::AuthorizationStatus_NotAuthorized;
	
public:
	std::string hashForScore(const Dictionary& score);
	
	void setScore(const std::string& lbUd, int64_t value, bool completed);
	void setAchievement(const std::string& aId, bool completed);
	
	void saveOptions();
};

GameCenter::GameCenter()
{
	ET_PIMPL_INIT(GameCenter)
	
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

GameCenter::~GameCenter()
{
	ET_PIMPL_FINALIZE(GameCenter)
}

GameCenter::AuthorizationStatus GameCenter::status() const
{
	return _private->status;
}

void GameCenter::authenticate()
{
	[[GKLocalPlayer localPlayer] setAuthenticateHandler:^(ViewControllerClass* viewController, NSError*)
	{
		if (viewController != nil)
		{
			_private->status = AuthorizationStatus_Authorizing;
			
#if (TARGET_OS_IPHONE)
			UIViewController* mainViewController = (__bridge UIViewController*)
				reinterpret_cast<void*>(application().renderingContextHandle());
			[mainViewController presentViewController:viewController animated:YES completion:nil];
#endif
		}
		else
		{
			GKLocalPlayer* player = [GKLocalPlayer localPlayer];
			if ([player isAuthenticated])
			{
				_player.alias = std::string([[player alias] UTF8String]);
				_player.displayName = std::string([[player displayName] UTF8String]);
				_player.playerId = std::string([[player playerID] UTF8String]);
				_private->status = AuthorizationStatus_Authorized;
				authenticationCompleted();
			}
			else
			{
				_private->status = AuthorizationStatus_NotAuthorized;
				// NSLog(@"Unable to authenticate to Game Center\n%@", error);
			}
		}
		
		authorizationStatusChanged.invokeInMainRunLoop(_private->status);
	}];
}

void GameCenter::showLeaderboard(const std::string& ldId)
{
	if ([[GKLocalPlayer localPlayer] isAuthenticated])
	{
		[[SharedGameCenterDelegate instance] performSelectorOnMainThread:@selector(showLeaderboard:)
			withObject:[NSString stringWithUTF8String:ldId.c_str()] waitUntilDone:NO];
	}
}

void GameCenter::showLeaderboards()
{
	if ([[GKLocalPlayer localPlayer] isAuthenticated])
	{
		[[SharedGameCenterDelegate instance] performSelectorOnMainThread:@selector(showLeaderboards)
			withObject:nil waitUntilDone:NO];
	}
}

void GameCenter::reportScoreForLeaderboard(const std::string& lId, int64_t value)
{
	if ([[GKLocalPlayer localPlayer] isAuthenticated])
	{
#	if (__IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_7_0) || (__MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_10_8)
		GKScore* score = [[GKScore alloc] initWithLeaderboardIdentifier:[NSString stringWithUTF8String:lId.c_str()]];
#	else
		GKScore* score = [[GKScore alloc] initWithCategory:[NSString stringWithUTF8String:lId.c_str()]];
#	endif
		score.value = value;
		score.context = 0;
		
#	if (__IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_7_0) || (__MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_10_8)
		[GKScore reportScores:@[score] withCompletionHandler:^(NSError *error)
#	else
		[score reportScoreWithCompletionHandler:^(NSError *error)
#	endif
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
		GKAchievement* ach = [[GKAchievement alloc] initWithIdentifier:[NSString stringWithUTF8String:achId.c_str()]];
		ach.percentComplete = 100.0;
		ach.showsCompletionBanner = YES;
		
		std::string localId = achId;
		
#if (__IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_7_0)
		[GKAchievement reportAchievements:@[ach] withCompletionHandler:^(NSError *error)
#else
		[ach reportAchievementWithCompletionHandler:^(NSError *error)
#endif
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
		int64_t completed = IntegerValue(p.second)->content;
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
		achievements.setIntegerForKey(aId, 0ll);
	
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

std::string GameCenterPrivate::hashForScore(const Dictionary& score)
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
	(void)gameCenterViewController;
	
#if (TARGET_OS_IPHONE)
	UIViewController* mainViewController = (__bridge UIViewController*)
		reinterpret_cast<void*>(application().renderingContextHandle());
	[mainViewController dismissViewControllerAnimated:YES completion:nil];
#endif
}

- (void)leaderboardViewControllerDidFinish:(GKGameCenterViewController*)viewController
{
	[self gameCenterViewControllerDidFinish:viewController];
}

- (void)achievementViewControllerDidFinish:(GKGameCenterViewController*)viewController
{
	[self gameCenterViewControllerDidFinish:viewController];
}

- (void)showAchievements
{
#if (TARGET_OS_IPHONE)
	UIViewController* mainViewController = (__bridge UIViewController*)
		reinterpret_cast<void*>(application().renderingContextHandle());
	
	GKGameCenterViewController* gcController = [[GKGameCenterViewController alloc] init];
	if (gcController != nil)
	{
		gcController.viewState = GKGameCenterViewControllerStateAchievements;
		gcController.gameCenterDelegate = [SharedGameCenterDelegate instance];
		[mainViewController presentViewController:gcController animated:YES completion:nil];
	}
	ET_OBJC_RELEASE(gcController)
#endif
}

- (void)showLeaderboards
{
#if (TARGET_OS_IPHONE)
	UIViewController* mainViewController = (__bridge UIViewController*)
	reinterpret_cast<void*>(application().renderingContextHandle());
	
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
			 GKGameCenterViewController* gcController = [[GKGameCenterViewController alloc] init];
			 if (gcController != nil)
			 {
				 gcController.viewState = GKGameCenterViewControllerStateLeaderboards;
				 gcController.gameCenterDelegate = [SharedGameCenterDelegate instance];
				 [mainViewController presentViewController:gcController animated:YES completion:nil];
			 }
			 ET_OBJC_RELEASE(gcController)
		 }
	 }];
#endif
}

- (void)showLeaderboard:(NSString*)leaderboardId
{
#if (TARGET_OS_IPHONE)
	UIViewController* mainViewController = (__bridge UIViewController*)
		reinterpret_cast<void*>(application().renderingContextHandle());
	
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
			GKGameCenterViewController* gcController = [[GKGameCenterViewController alloc] init];
			if (gcController != nil)
			{
				gcController.viewState = GKGameCenterViewControllerStateLeaderboards;
				for (GKLeaderboard* leaderboard in leaderboards)
				{
#				if (__IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_7_0)
					if ([leaderboard.identifier isEqualToString:leaderboardId]) {
						gcController.leaderboardIdentifier = leaderboard.identifier;
#				else
					if ([leaderboard.category isEqualToString:leaderboardId]) {
						gcController.leaderboardCategory = leaderboard.category;
#				endif
						if ([gcController respondsToSelector:@selector(setLeaderboardIdentifier:)])
							gcController.leaderboardIdentifier = leaderboardId;
						
						break;
					}
				}
				
				gcController.gameCenterDelegate = [SharedGameCenterDelegate instance];
				[mainViewController presentViewController:gcController animated:YES completion:nil];
			}
			ET_OBJC_RELEASE(gcController)
		}
	}];
#else
	(void)leaderboardId;
#endif
}

@end
