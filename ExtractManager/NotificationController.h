//
//  NotificationController.h
//  TransparentWindow
//
//  Created by opiopan on 12/01/21.
//  Copyright (c) 2012年 opiopan. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface NotificationController : NSObject

- (id)initWithIcon:(NSImage*)icn caption:(NSString*)cap font:(NSFont*)fnt visiblePeriod:(double)period;
- (void)notify;
- (void)notifyForSecond:(double)period withIcon:(NSImage*)icn caption:(NSString*)cap font:(NSFont*)fnt;

@end
