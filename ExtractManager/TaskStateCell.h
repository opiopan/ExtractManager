//
//  TaskStateCell.h
//  ExtractManager
//
//    実行中タスクの場合、タスクステート列にプログレスインジケータ（グルグル回転する
//    インジケータ）を表示するために、NSTextFieldCellに機能追加。
//

#import <AppKit/AppKit.h>

@interface TaskStateCell : NSTextFieldCell

- (void)setProgress:(NSProgressIndicator *)newProgress;

@end
