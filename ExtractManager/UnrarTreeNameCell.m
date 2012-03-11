//
//  UnrarTreeNameCell.m
//  ExtractManager
//
//    NSTextFieldCellにFinderのファイル名編集と同等の機能を付加する。
//    ・編集開始時の選択範囲が、拡張子部分を除いた部分(basename部分）となる
//

#import "UnrarTreeNameCell.h"

@implementation UnrarTreeNameCell
{
    NSInteger selectionChangeCount;
}

- (NSText *)setUpFieldEditorAttributes:(NSText *)textObj
{
    [textObj setDelegate:self];
    selectionChangeCount = 2;
    return textObj;
}

- (NSRange)textView:(NSTextView *)aTextView willChangeSelectionFromCharacterRange:(NSRange)oldSelectedCharRange
   toCharacterRange:(NSRange)newSelectedCharRange
{
    NSString* str = [aTextView string];
    if (selectionChangeCount > 0 && newSelectedCharRange.location == 0 && 
        newSelectedCharRange.length == [str length]){
        NSInteger extlen = [[str pathExtension] length];
        if (extlen > 0){
            newSelectedCharRange.length = [str length] - extlen -1;
        }
        selectionChangeCount--;
    }
    
    return newSelectedCharRange;
}

@end
