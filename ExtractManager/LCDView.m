//
//  LCDView.m
//  ExtractManager
//
//    LCDパネル部分をクリックしてもドラッグ可能とするためにNSImageViewの
//    派生クラスを作成。
//

#import "LCDView.h"

@implementation LCDView

- (BOOL)mouseDownCanMoveWindow
{
    return YES;
}

@end
