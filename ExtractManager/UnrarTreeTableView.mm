//
//  UnrarTreeTableView.mm
//  ExtractManager
//
//    ファイル名編集完了イベントをコントローラ(delegate)に伝えるために
//    NSTableViewの派生クラスを定義。
//    編集開始ははNSTableViewのdelegateメソッドとして定義されているが
//    編集完了はNSTableView自身にしか通知されないため、本クラスでイベントを
//    回送する。
//    本クラスによって、delegateオブジェクトの下記メソッドが呼び出される。
//      - (BOOL)tableView:(NSTableView *)aTableView textShouldEndEditing:(NSText *)textObject
//

#import "UnrarTreeTableView.h"
#import "UnrarTaskController.h"

@implementation UnrarTreeTableView

// ファイル名編集完了イベントのdelegate
- (BOOL) textShouldEndEditing:(NSText *)textObject
{
    id delegate = [self delegate];
    return [delegate tableView:self textShouldEndEditing:textObject];
}

// デフォルキー操作の変更
- (void)keyDown:(NSEvent *)theEvent
{
    NSUInteger selectedRowsCount = [[self selectedRowIndexes] count];
    NSInteger selectedRow = [self selectedRow];
    NSString* key = [theEvent charactersIgnoringModifiers];
    NSUInteger modifierFlags = [theEvent modifierFlags];
    
    if ([key isEqual:@"\r"] && selectedRowsCount == 1) {
        if (![_delegate tableView:self shouldEditTableColumn:nil row:selectedRow]){
            [_delegate performSelector:@selector(onNodeTableEnter:) withObject:self];
            return;
        }
    }else if ([key characterAtIndex:0] == NSDownArrowFunctionKey &&
              modifierFlags & NSCommandKeyMask && selectedRowsCount == 1){
        [_delegate performSelector:@selector(onNodeTableEnter:) withObject:self];
        return ;
    }else if ([key characterAtIndex:0] == NSUpArrowFunctionKey &&
              modifierFlags & NSCommandKeyMask){
        [_delegate performSelector:@selector(onNodeTableUp:) withObject:self];
        return;
    }else if ([key characterAtIndex:0] == 0x7f &&
              !(modifierFlags & NSCommandKeyMask) && selectedRowsCount > 0){
        [_delegate performSelector:@selector(onDeleteNode:) withObject:self];
        return;
    }else if ([key characterAtIndex:0] == 0x7f &&
              modifierFlags & NSCommandKeyMask && selectedRowsCount > 0){
        [_delegate performSelector:@selector(onDeleteNodeRecurce:) withObject:self];
        return;
    }
    
    [super keyDown:theEvent];
}


@end
