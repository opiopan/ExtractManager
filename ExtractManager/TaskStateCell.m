//
//  TaskStateCell.m
//  ExtractManager
//
//    実行中タスクの場合、タスクステート列にプログレスインジケータ（グルグル回転する
//    インジケータ）を表示するために、NSTextFieldCellに機能追加。
//

#import "TaskStateCell.h"

@implementation TaskStateCell
{
    NSProgressIndicator __weak* progress;
}

- (void)drawInteriorWithFrame:(NSRect)aRect inView:(NSView *)controlView
{
	[super drawInteriorWithFrame:aRect inView:controlView];
    
    if (progress){
        NSDictionary* attr = [[self attributedStringValue] attributesAtIndex:0 effectiveRange:nil];
        NSSize strSize = [[self stringValue] sizeWithAttributes:attr];
        
        NSRect progressRect = NSMakeRect(aRect.origin.x, aRect.origin.y, aRect.size.width, aRect.size.height);
        progressRect.origin.x += strSize.width + 5;
        progressRect.origin.y += 0;
        progressRect.size.width -= 24;
        [progress setFrame:progressRect];
        [progress sizeToFit];
    }
}

- (void)setProgress:(NSProgressIndicator *)newProgress
{
	progress = newProgress;
	[progress sizeToFit];
}

@end
