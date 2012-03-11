//
//  TaskProgressCell.m
//  ExtractManager
//
//    NSLevelIndicatorCellのデフォルト表示は、セルいっぱいの高さとなり
//    美しくない。少し隙間を空けるために派生クラスを定義。
//

#import "TaskProgressCell.h"

@implementation TaskProgressCell

- (void)drawWithFrame:(NSRect)cellFrame inView:(NSView *)controlView
{
    cellFrame.origin.y +=2;
    cellFrame.size.height -=4;
    [super drawWithFrame:cellFrame inView:controlView];
}

@end
