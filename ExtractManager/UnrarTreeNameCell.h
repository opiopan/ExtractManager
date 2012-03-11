//
//  UnrarTreeNameCell.h
//  ExtractManager
//
//    NSTextFieldCellにFinderのファイル名編集と同等の機能を付加する。
//    ・編集開始時の選択範囲が、拡張子部分を除いた部分(basename部分）となる
//

#import <AppKit/AppKit.h>

@interface UnrarTreeNameCell : NSTextFieldCell <NSTextDelegate, NSTextViewDelegate>

@end
