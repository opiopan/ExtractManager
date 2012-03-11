//
//  NotificationController.m
//  TransparentWindow
//
//  Created by opiopan on 12/01/21.
//  Copyright (c) 2012年 opiopan. All rights reserved.
//

#import "NotificationController.h"
#import "CoreGraphicsHelper.h"

#define FRAME_ROUND             20
#define FRAME_ALIGNMENT         25
#define FRAME_ALPHA             .3
#define CAPTION_ALIGNMENT       10
#define CAPTION_FONT_SIZE       18
#define CAPTION_SHADOW_WAIT     24
#define CAPTION_SHADOW_DARKNESS 4
#define VISIBLE_PERIOD          2.0
#define CLOSING_PERIOD          0.2

@implementation NotificationController
{
    NSWindow*   popupWindow;
    NSImage*    icon;
    NSString*   caption;
    NSFont*     font;
    bool        visible;
    NSTimer*    timer;
    double      baseTime;
    double      period;
}

- (id)initWithIcon:(NSImage*)icn caption:(NSString*)cap font:(NSFont*)fnt visiblePeriod:(double)p
{
    self = [super init];
    
    if (self){
        icon = icn;
        caption = cap;
        if (fnt){
            font = fnt;
        }else{
            font = [NSFont systemFontOfSize:CAPTION_FONT_SIZE];
        }
    }
    visible = false;
    popupWindow = nil;
    period = p;
    
    return self;
}

- (id)init
{
    return [self initWithIcon:nil caption:nil font:nil visiblePeriod:VISIBLE_PERIOD];
}

- (void)notify
{
    [self performSelectorOnMainThread:@selector(startToPopup) withObject:nil waitUntilDone:NO];
}

- (void)notifyForSecond:(double)p withIcon:(NSImage*)icn caption:(NSString*)cap font:(NSFont*)fnt
{
    period = p;
    icon = icn;
    caption = cap;
    if (fnt){
        font = fnt;
    }
    [self notify];
}

-(NSImage*)convretGrayScaleImage:(NSImage*)image
{
    CGSize size = CGSizeMake([image size].width, [image size].height);
    CGRect rect = CGRectMake(0, 0, size.width, size.height);
    ECGColorSpaceRef colorSpace;
    colorSpace = CGColorSpaceCreateDeviceGray();
    
    // create alpha chanel
    ECGContextRef alphaContext(CGBitmapContextCreate(nil, size.width, size.height, 8, 0,
                                                     colorSpace, kCGImageAlphaOnly));
    CGContextDrawImage(alphaContext, rect, [image CGImageForProposedRect:NULL context:nil hints:nil]);
    ECGImageRef alphaImage;
    alphaImage = CGBitmapContextCreateImage(alphaContext);
    
    // create gray scale image
    ECGContextRef context;
    context = CGBitmapContextCreate(nil, size.width, size.height, 8, 0,
                                    colorSpace, kCGImageAlphaNone);
    CGContextDrawImage(context, rect, [image CGImageForProposedRect:NULL context:nil hints:nil]);
    ECGImageRef grayScaleImage(CGBitmapContextCreateImage(context));
    
    // composite images
    NSImage* grayScaleNSImage = [[NSImage alloc] initWithCGImage:CGImageCreateWithMask(grayScaleImage, alphaImage)
                                                            size:size];    
    return grayScaleNSImage;
}

- (void)startToPopup
{
    if (!visible && !popupWindow){
        //----------------------------------------------------------------------
        // calculate window and parts geometry
        //----------------------------------------------------------------------
        NSSize iconSize = [icon size];
        NSShadow* shadow = [[NSShadow alloc] init];
        [shadow setShadowOffset:NSMakeSize(0, 0)];
        [shadow setShadowColor:[NSColor blackColor]];
        [shadow setShadowBlurRadius:CAPTION_SHADOW_WAIT];
        NSDictionary* stringAttr = [NSDictionary dictionaryWithObjectsAndKeys:
                                    font, NSFontAttributeName,
                                    [NSColor whiteColor], NSForegroundColorAttributeName,
                                    shadow, NSShadowAttributeName,
                                    nil];
        NSString* string = caption;
        NSSize stringSize = [string sizeWithAttributes:stringAttr];
        NSSize frameSize = NSMakeSize(MAX(iconSize.width, stringSize.width) + FRAME_ALIGNMENT * 2,
                                      iconSize.height + stringSize.height + FRAME_ALIGNMENT * 2 + CAPTION_ALIGNMENT);
        NSArray* screens = [NSScreen screens];
        NSScreen* screen = [screens objectAtIndex:0];
        NSRect frameRect = [screen frame];
        frameRect.origin.x = frameRect.size.width / 2 - frameSize.width / 2;
        frameRect.origin.y = frameRect.size.height / 2 - frameSize.height / 2;
        frameRect.size = frameSize;
        NSRect stringRect = NSMakeRect(frameRect.size.width / 2 - stringSize.width / 2,
                                       FRAME_ALIGNMENT, 
                                       stringSize.width, 
                                       stringSize.height);
        NSRect iconRect = NSMakeRect(frameRect.size.width / 2 - iconSize.width / 2,
                                     FRAME_ALIGNMENT + CAPTION_ALIGNMENT + stringSize.height,
                                     iconSize.width,
                                     iconSize.height);
        
        //----------------------------------------------------------------------
        // create a transparent window
        //----------------------------------------------------------------------
        popupWindow = [[NSWindow alloc] initWithContentRect:frameRect
                                                  styleMask:NSBorderlessWindowMask
                                                    backing:NSBackingStoreBuffered 
                                                      defer:YES screen:screen];    
        [popupWindow setCollectionBehavior:NSWindowCollectionBehaviorMoveToActiveSpace];
        [popupWindow setReleasedWhenClosed:NO];
        [popupWindow setBackgroundColor:[NSColor clearColor]];
        [popupWindow setOpaque:NO];
        [popupWindow setIgnoresMouseEvents:YES];        
        [popupWindow orderWindow:NSWindowAbove relativeTo:0];
        [popupWindow setLevel:NSScreenSaverWindowLevel + 1];
        
        //----------------------------------------------------------------------
        // set up graphics context
        //----------------------------------------------------------------------
        NSGraphicsContext* gc = [popupWindow graphicsContext];
        [NSGraphicsContext setCurrentContext:gc];
        [gc setShouldAntialias:YES];
        
        //----------------------------------------------------------------------
        // draw translucent frame
        //----------------------------------------------------------------------
        NSColor* color = [NSColor colorWithDeviceRed:.0 green:.0 blue:.0 alpha:FRAME_ALPHA];
        [color setFill];
        NSBezierPath* path = [NSBezierPath bezierPathWithRoundedRect:NSMakeRect(0, 0, 
                                                                                frameSize.width, frameSize.height)
                                                             xRadius:FRAME_ROUND yRadius:FRAME_ROUND];
        [path fill];
        
        //----------------------------------------------------------------------
        // draw icon
        //----------------------------------------------------------------------
        [[self convretGrayScaleImage:icon] compositeToPoint:iconRect.origin 
                                                  operation:NSCompositeSourceOver];
        
        //----------------------------------------------------------------------
        // draw notification string
        //----------------------------------------------------------------------
        for (int i = 0; i < CAPTION_SHADOW_DARKNESS; i++){
            [string drawInRect:stringRect withAttributes:stringAttr];
        }
        
        //----------------------------------------------------------------------
        // flush window
        //----------------------------------------------------------------------
        [gc flushGraphics];        
        visible = true;
        [self performSelector:@selector(startToClose) withObject:nil afterDelay:period];
    }
}

- (void)startToClose
{
    timer = [NSTimer scheduledTimerWithTimeInterval:1.0/30.0 
                                             target:self 
                                           selector:@selector(animateFlame) 
                                           userInfo:nil 
                                            repeats:YES];
    [[NSRunLoop currentRunLoop] addTimer:timer forMode:NSDefaultRunLoopMode];
    baseTime = [NSDate timeIntervalSinceReferenceDate];
}

- (void)animateFlame
{
    if (visible){
        double dt = [NSDate timeIntervalSinceReferenceDate] - baseTime;
        double alpha = 1.0 - dt / CLOSING_PERIOD;
        if (alpha < 0.0){
            alpha = 0.0;
        }
        [popupWindow setAlphaValue:alpha];
        if (alpha == 0.0){
            [timer invalidate];
            timer = nil;
            [popupWindow close];
            visible = false;
            [self performSelectorOnMainThread:@selector(releaseWindow) withObject:nil waitUntilDone:NO];
        }
    }
}


- (void)releaseWindow
{
    popupWindow = nil;
}

@end
