//
//  AppDelegate.h
//  ExtractManager
//

#import <Cocoa/Cocoa.h>
#import "TaskScheduler.h"
#import "LCDCell.h"

@interface AppDelegate : NSObject <NSApplicationDelegate>

// メインウィンドウ用アウトレット
@property (assign) IBOutlet NSWindow *window;
@property (weak) IBOutlet NSTableView *taskTable;
@property (weak) IBOutlet NSButton *runButton;
@property (weak) IBOutlet NSButton *pauseButton;
@property (weak) IBOutlet NSSegmentedControl *addOrEditSegment;
@property (weak) IBOutlet NSSegmentedControl *deleteButton;
@property (weak) IBOutlet LCDCell *indicator;

// パスワードシート用アウトレット
@property (weak) IBOutlet NSView *passwordSheet;
@property (weak) IBOutlet NSTextField *passwordField;
@property (weak) IBOutlet NSButton *passwordOKButton;

// タスク操作ボタン応答
- (IBAction)OnRun:(id)sender;
- (IBAction)OnDelete:(id)sender;
- (IBAction)OnPause:(id)sender;
- (IBAction)OnAddOrEdit:(id)sender;
- (IBAction)OnAddTask:(id)sender;
- (IBAction)OnEditTask:(id)sender;

// コントロール状態更新
- (void)reflectSchedulingStateToControls;
- (void)reflectSchedulingStateToLCD;

// タスク生成 & 編集
- (BOOL)newTaskAndEdit:(std::vector<std::string>*)files password:(const char*)password;

// タスク編集応答処理
- (void) didEndEditTaskWith:(NSInteger)result kind:(NSInteger)kind;

// パスワード入力シート応答処理
- (IBAction)passwordOK:(id)sender;
- (IBAction)passwordCancel:(id)sender;


// エラーメッセージシート表示
- (void)beginErrorSheetWithTitle:(NSString *)title message:(NSString *)message;

@end
