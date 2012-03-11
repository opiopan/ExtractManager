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

- (BOOL) textShouldEndEditing:(NSText *)textObject
{
    id delegate = [self delegate];
    return [delegate tableView:self textShouldEndEditing:textObject];
}

@end
