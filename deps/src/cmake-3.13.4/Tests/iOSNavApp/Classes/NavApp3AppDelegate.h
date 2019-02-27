//
//  NavApp3AppDelegate.h
//  NavApp3
//
//  Created by David Cole on 6/23/11.
//  Copyright 2011 Kitware, Inc. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface NavApp3AppDelegate : NSObject<UIApplicationDelegate> {

  UIWindow* window;
  UINavigationController* navigationController;
}

@property (nonatomic, retain) IBOutlet UIWindow* window;
@property (nonatomic, retain)
  IBOutlet UINavigationController* navigationController;

@end
