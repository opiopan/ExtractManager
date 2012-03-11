//
//  UnrarTaskController.h
//  ExtractManager
//

#import "TaskController.h"
#import "UnrarTask.h"

@interface UnrarTaskController : NSObject <TaskController>

@property (strong) IBOutlet NSPanel *panel;
@property (weak) IBOutlet NSTextField *targetDir;
@property (weak) IBOutlet NSTextField *password;
@property (unsafe_unretained) IBOutlet NSTextView *comment;
@property (weak) IBOutlet NSTextField *volumes;
@property (weak) IBOutlet NSButton *toBeDeleted;
@property (weak) IBOutlet NSButton *toUpdateTimestamp;
@property (weak) IBOutlet NSTableView *nodeTable;

// アクション応答
- (IBAction)onOK:(id)sender;
- (IBAction)onCancel:(id)sender;
- (IBAction)onDeleteNode:(id)sender;
- (IBAction)onDeleteNodeRecurce:(id)sender;
- (IBAction)onResetTree:(id)sender;
- (IBAction)onSetPassword:(id)sender;
- (void)onNodeTableDouble:(id)sender;

// ノードテーブルの編集操作
- (BOOL)tableView:(NSTableView *)aTableView textShouldEndEditing:(NSText *)textObject;

@end
