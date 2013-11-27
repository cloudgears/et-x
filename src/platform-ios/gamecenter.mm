/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <GameKit/GameKit.h>
#include <et/app/application.h>
#include <et/app/applicationnotifier.h>
#include <et-ext/ios/gamecenter.h>

using namespace et;

@interface SharedGameCenterDelegate : NSObject <GKGameCenterControllerDelegate>

+ (instancetype)instance;

- (void)showAchievements;
- (void)showLeaderboard:(NSString*)leaderboardId;

@end

class et::GameCenterPrivate
{
public:
	bool authenticated = false;
	bool authenticationViewControllerDisplayed = false;
};

GameCenter::GameCenter() :
	_private(new GameCenterPrivate)
{
	authenticate();
}

void GameCenter::authenticate()
{
	GKLocalPlayer* player = [GKLocalPlayer localPlayer];
	player.authenticateHandler = ^(UIViewController* viewController, NSError* error)
	{
		if (viewController != nil)
		{
			UIViewController* mainViewController =
				reinterpret_cast<UIViewController*>(application().renderingContextHandle());
			
			[mainViewController presentViewController:viewController animated:YES completion:^
			{
				ApplicationNotifier notifier;
				notifier.notifyDeactivated();
				_private->authenticationViewControllerDisplayed = true;
			}];
		}
		else
		{
			if (_private->authenticationViewControllerDisplayed)
			{
				_private->authenticationViewControllerDisplayed = false;
				ApplicationNotifier notifier;
				notifier.notifyActivated();
			}
			
			if (player.authenticated)
			{
				_private->authenticated = true;
				NSLog(@"Player authenticated");
			}
			else
			{
				_private->authenticated = false;
				NSLog(@"Unable to authenticate to Game Center. Error: %@", error);
			}
		}
	};
}

void GameCenter::showLeaderboard(const std::string& ldId)
{
	if ([[GKLocalPlayer localPlayer] isAuthenticated])
	{
		[[SharedGameCenterDelegate instance] performSelectorOnMainThread:@selector(showLeaderboard:)
			withObject:[NSString stringWithUTF8String:ldId.c_str()] waitUntilDone:NO];
	}
	else
	{
		authenticate();
	}
}

void GameCenter::reportScoreForLeaderboard(const std::string& lId, int64_t value)
{
	GKScore* score = [[GKScore alloc] initWithCategory:[NSString stringWithUTF8String:lId.c_str()]];
	score.context = 0;
	score.value = value;
	
	[score reportScoreWithCompletionHandler:^(NSError *error)
	{
		if (error != nil)
			NSLog(@"Unable to report score to %s: %@", lId.c_str(), error);
	}];
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

void GameCenter::unlockAchievement(const std::string& aId)
{
	if ([[GKLocalPlayer localPlayer] isAuthenticated])
	{
		GKAchievement* ach = [[GKAchievement alloc] initWithIdentifier:[NSString stringWithUTF8String:aId.c_str()]];
		ach.percentComplete = 100.0;
		ach.showsCompletionBanner = YES;
		
		[ach reportAchievementWithCompletionHandler:^(NSError *error)
		{
			if (error != nil)
				NSLog(@"Unable to report achievement %s: %@", aId.c_str(), error);
		}];
	}
	else
	{
		authenticate();
	}
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
	ApplicationNotifier notifier;
	notifier.notifyActivated();
	
	UIViewController* mainViewController =
		reinterpret_cast<UIViewController*>(application().renderingContextHandle());
	
	[mainViewController dismissViewControllerAnimated:YES completion:nil];
}

- (void)showAchievements
{
	UIViewController* mainViewController =
		reinterpret_cast<UIViewController*>(application().renderingContextHandle());
	
	GKAchievementViewController* gcController = [[GKAchievementViewController alloc] init];
	if (gcController != nil)
	{
		gcController.gameCenterDelegate = [SharedGameCenterDelegate instance];
		
		[mainViewController presentViewController:gcController animated:YES completion:^
		 {
			 ApplicationNotifier notifier;
			 notifier.notifyDeactivated();
		 }];
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
			NSLog(@"Unable to load leaderboards: %@", error);
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
					if ([leaderboard.identifier isEqualToString:leaderboardId])
					{
						gcController.leaderboardCategory = leaderboard.category;
						gcController.leaderboardTimeScope = leaderboard.timeScope;
						gcController.leaderboardIdentifier = leaderboardId;
						break;
					}
				}
				
				gcController.gameCenterDelegate = [SharedGameCenterDelegate instance];
				[mainViewController presentViewController:gcController animated:YES completion:^
				 {
					 ApplicationNotifier notifier;
					 notifier.notifyDeactivated();
				 }];
			}
			[gcController release];
		}
	}];
}

@end
