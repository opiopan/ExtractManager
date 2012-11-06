//
//  AppDelegate.m
//  ExtractManager
//

#import "AppDelegate.h"
#import "UnrarTaskController.h"
#import "TaskStateCell.h"
#import "NotificationController.h"
#import "UnzipTask.h"

static void gatewayForUnrarNotification(int taskIndex, void* context);

@implementation AppDelegate {
    NSDictionary*           taskControlers;
    TaskBasePtr*            taskHolder;
    NSProgressIndicator*    progressView;
    NSOpenPanel*            openPanel;
    NotificationController* notifier;
    bool                    skipFistDeepSleep;
}

@synthesize window = _window;
@synthesize taskTable = _taskTable;
@synthesize runButton = _runButton;
@synthesize pauseButton = _pauseButton;
@synthesize addOrEditSegment = _addOrEditSegment;
@synthesize deleteButton = _deleteButton;
@synthesize indicator = _indicator;

@synthesize passwordSheet = _passwordSheet;
@synthesize passwordField = _passwordField;
@synthesize passwordOKButton = _passwordOKButton;

//----------------------------------------------------------------------
// アプリケーション起動・停止応答
//----------------------------------------------------------------------
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    [self performSelectorInBackground:@selector(executeTask) withObject:nil];

    // タスクリストスクロール
    [_taskTable scrollToEndOfDocument:self];
    
    // コントロール初期状態設定
    [self reflectSchedulingStateToControls];
    [self reflectSchedulingStateToLCD];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
    TaskScheduler::getInstance().shutdown();
}

//----------------------------------------------------------------------
// ウインドウ終了時動作
//----------------------------------------------------------------------
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication
{
    return true;
}

  
//----------------------------------------------------------------------
// オブジェクト初期化
//----------------------------------------------------------------------
- (void) awakeFromNib
{
    taskHolder = new TaskBasePtr;

    // unzipコマンドのディレクトリ（バンドル内）を設定
    NSString* executableDir = [[[NSBundle mainBundle] executablePath] stringByDeletingLastPathComponent];
    UnzipTask::setBinDir([executableDir UTF8String]);
    
    // ドラッグ受付対象の登録
    NSArray *parrTypes = [NSArray arrayWithObject:NSFilenamesPboardType];
    [_taskTable registerForDraggedTypes:parrTypes];
    
    // タスクコントローラーの定義
    taskControlers = [NSDictionary dictionaryWithObjectsAndKeys:
                      [[UnrarTaskController alloc] init], @"UnRAR task",
                      [[UnrarTaskController alloc] init], @"UnZIP task",
                      nil];
    
    // タスクリストアクション登録
    [_taskTable setTarget:self];
    [_taskTable setDoubleAction:@selector(onTaskTableDouble:)];
    
    // タスク追加用オープンパネル作成
    openPanel = [NSOpenPanel openPanel];
    [openPanel allowsMultipleSelection];
    
    // 通知コントローラー作成
    NSImage* icon = [[NSBundle mainBundle] imageForResource:@"ExtractManager.icns"];
    [icon setSize:NSMakeSize(100, 100)];
    notifier = [[NotificationController alloc] initWithIcon:icon 
                                                    caption:@"All extracting tasks finished" 
                                                       font:nil visiblePeriod:2];
    
    // 実行可能ジョブがない存在しない場合は初回完了通知をスキップ
    skipFistDeepSleep = true;
    for (int i = 0; i < TaskScheduler::getInstance().getTaskNum(); i++){
        if (TaskScheduler::getInstance().getTask(i)->getState() == TaskBase::TASK_PREPARED){
            skipFistDeepSleep = false;
        }
    }

    // ウインドウ位置自動保存
    [_window setFrameAutosaveName:@"MainWindowFrame"];
}


//----------------------------------------------------------------------
// コントロール状態更新
//----------------------------------------------------------------------
- (void)reflectSchedulingStateToControls
{
    NSIndexSet* indexes = [_taskTable selectedRowIndexes];
    NSInteger indexNum = [indexes count];
    int64_t running = 0;
    int64_t suspended = 0;
    int64_t prepared = 0;
    int64_t error = 0;
    int64_t succeeded = 0;
    if (indexNum == 0){
        for (int64_t i = 0; i < TaskScheduler::getInstance().getTaskNum(); i++){
            TaskBasePtr task = TaskScheduler::getInstance().getTask(i);
            switch (task->getState()){
                case TaskBase::TASK_PREPARED:
                    prepared++;
                    break;
                case TaskBase::TASK_RUNNING:
                    running++;
                    break;
                case TaskBase::TASK_SUSPENDED:
                    suspended++;
                    break;
                case TaskBase::TASK_SUCCEED:
                    succeeded++;
                    break;
                case TaskBase::TASK_ERROR:
                    error++;
                    break;
                default:
                    break;
            }
        }
        [_runButton setEnabled:suspended > 0];
        [_pauseButton setEnabled:running > 0 || prepared > 0];
        [_addOrEditSegment setEnabled:YES forSegment:0]; // add segment
        [_addOrEditSegment setEnabled:NO forSegment:1];  //edit segment
        [_deleteButton setEnabled:succeeded > 0 forSegment:0];
    }else if (indexNum > 0){
        for (NSInteger i = [indexes firstIndex]; i != NSNotFound; i = [indexes indexGreaterThanIndex:i]){
            TaskBasePtr task = TaskScheduler::getInstance().getTask(i);
            switch (task->getState()){
                case TaskBase::TASK_PREPARED:
                    prepared++;
                    break;
                case TaskBase::TASK_RUNNING:
                    running++;
                    break;
                case TaskBase::TASK_SUSPENDED:
                    suspended++;
                    break;
                case TaskBase::TASK_SUCCEED:
                    succeeded++;
                    break;
                case TaskBase::TASK_ERROR:
                    error++;
                    break;
                default:
                    break;
            }
        }
        [_runButton setEnabled:suspended > 0 || error > 0];
        [_pauseButton setEnabled:running > 0 || prepared > 0];
        [_addOrEditSegment setEnabled:YES forSegment:0]; // add segment
        [_addOrEditSegment setEnabled:indexNum == 1 forSegment:1];  //edit segment
        [_deleteButton setEnabled:succeeded > 0 || error > 0 || prepared > 0 || suspended > 0 forSegment:0];
    }
}

//----------------------------------------------------------------------
// LCDパネル更新
//----------------------------------------------------------------------
- (void)reflectSchedulingStateToLCD
{
    int64_t running = 0;
    int64_t suspended = 0;
    int64_t prepared = 0;
    int64_t error = 0;
    int64_t succeeded = 0;
    int64_t current = -1;

    for (int64_t i = 0; i < TaskScheduler::getInstance().getTaskNum(); i++){
        TaskBasePtr task = TaskScheduler::getInstance().getTask(i);
        switch (task->getState()){
            case TaskBase::TASK_PREPARED:
                prepared++;
                break;
            case TaskBase::TASK_RUNNING:
                running++;
                current = i;
                break;
            case TaskBase::TASK_SUSPENDED:
                suspended++;
                break;
            case TaskBase::TASK_SUCCEED:
                succeeded++;
                break;
            case TaskBase::TASK_ERROR:
                error++;
                break;
            default:
                break;
        }
    }
    
    LCDTaskProgress* progress = nil;
    if (running > 0){
        TaskBase::TaskProgress cppProgress;
        TaskBasePtr task = TaskScheduler::getInstance().getTask(current);
        task->getProgress(cppProgress);
        progress = [[LCDTaskProgress alloc] initWithName:[NSString stringWithUTF8String:task->getName()] 
                                            currentPhase:[NSString stringWithUTF8String:cppProgress.phase] 
                                               totalSize:cppProgress.total completeSize:cppProgress.complete];
    }
    
    [_indicator setTaskStatistics:[[LCDTaskStatistics alloc] initWithSucceededTaskCount:succeeded 
                                                                         errorTaskCount:error 
                                                                     suspendedTaskCount:suspended 
                                                                       runningTaskCount:running 
                                                                      preparedTaskCount:prepared] 
                     withProgress:progress];
}

//----------------------------------------------------------------------
// タスク操作ボタン応答
//----------------------------------------------------------------------
- (IBAction)OnRun:(id)sender {
    NSIndexSet* indexes = [_taskTable selectedRowIndexes];
    if ([indexes count]){
        for (NSUInteger index = [indexes firstIndex]; 
             index != NSNotFound; 
             index = [indexes indexGreaterThanIndex:index]){
            TaskScheduler::getInstance().getTask(index)->resume();
        }
    }else{
        for (int64_t i = TaskScheduler::getInstance().getTaskNum() - 1; i >= 0; i--){
            if (TaskScheduler::getInstance().getTask(i)->getState() == TaskBase::TASK_SUSPENDED){
                TaskScheduler::getInstance().getTask(i)->resume();
            }
        }
    }
    TaskScheduler::getInstance().commit();
    [_taskTable reloadData];
    [self reflectSchedulingStateToControls];
    [self reflectSchedulingStateToLCD];
}

- (IBAction)OnDelete:(id)sender {
    NSIndexSet* indexes = [_taskTable selectedRowIndexes];
    if ([indexes count]){
        std::vector<TaskBase*> targets;
        for (NSUInteger index = [indexes firstIndex]; 
             index != NSNotFound; 
             index = [indexes indexGreaterThanIndex:index]){
            targets.push_back(TaskScheduler::getInstance().getTask(index).operator->());
        }
        for (std::vector<TaskBase*>::iterator i = targets.begin(); i != targets.end(); i++){
            for (int j = 0; j < TaskScheduler::getInstance().getTaskNum(); j++){
                if (*i == TaskScheduler::getInstance().getTask(j).operator->()){
                    TaskScheduler::getInstance().removeTask(j);
                }
            }
        }
    }else{
        for (int64_t i = TaskScheduler::getInstance().getTaskNum() - 1; i >= 0; i--){
            if (TaskScheduler::getInstance().getTask(i)->getState() == TaskBase::TASK_SUCCEED){
                TaskScheduler::getInstance().removeTask(i);
            }
        }
    }
    TaskScheduler::getInstance().persist();
    [_taskTable reloadData];
    [self reflectSchedulingStateToControls];
    [self reflectSchedulingStateToLCD];
}

- (IBAction)OnPause:(id)sender {
    NSIndexSet* indexes = [_taskTable selectedRowIndexes];
    if ([indexes count]){
        for (NSUInteger index = [indexes firstIndex]; 
             index != NSNotFound; 
             index = [indexes indexGreaterThanIndex:index]){
            TaskScheduler::getInstance().getTask(index)->cancel();
        }
    }else{
        for (int64_t i = TaskScheduler::getInstance().getTaskNum() - 1; i >= 0; i--){
            TaskScheduler::getInstance().getTask(i)->cancel();
        }
    }
    
    [_taskTable reloadData];
    [self reflectSchedulingStateToControls];
}

- (void)invokeAddTask:(id)dummy
{
    NSArray* URLs = [openPanel URLs];
    std::vector<std::string>* files = NULL;
    try {
        // ファイル一覧をNSArrayからstd::vectorに変換
        files = new std::vector<std::string>;
        NSUInteger items = [URLs count];
        for (int i = 0; i < items; i++){
            NSString* path = [[URLs objectAtIndex:i] path];
            files->push_back([path UTF8String]);
        }
        
        if (TaskFactory::getInstance().isAcceptableFiles(*files)){                    
            // タスクオブジェクト生成 & 編集モーダルシート起動
            [self newTaskAndEdit:files password:NULL];
        }else{
            [self beginErrorSheetWithTitle:@"Open error" 
                                   message:@"Specified files are not supported"];
            delete files;
        }
    }catch (std::bad_alloc){
        NSLog(@"bad_alloc detected");
    }
}

- (IBAction)OnAddOrEdit:(id)sender {
    switch ([sender selectedSegment]){
        case 0:{
            // add task
            
            [openPanel beginSheetModalForWindow:_window
                               completionHandler:^(NSInteger result){
                                   if (result == NSFileHandlingPanelOKButton){
                                       [self performSelectorOnMainThread:@selector(invokeAddTask:) 
                                                              withObject:nil 
                                                           waitUntilDone:NO];
                                   }
                               }];
            break;
        }
        case 1:{
            // edit task
            *taskHolder = TaskScheduler::getInstance().getTask([_taskTable selectedRow]);
            NSString* type = [[NSString alloc] initWithUTF8String:(*taskHolder)->getType()];
            [[taskControlers objectForKey:type] beginEditTask:taskHolder 
                                               modalForWindow:_window 
                                                modalDelegate:self
                                                  endSelector:@selector(didEndEditTask:context:) 
                                                     editType:taskModify];
            break;
        }
    }
}

//----------------------------------------------------------------------
// タスクリストアクション
//----------------------------------------------------------------------
- (void)onTaskTableDouble:(id)sender
{
    try{
        if ([_taskTable clickedRow] >=0){
            *taskHolder = TaskScheduler::getInstance().getTask([_taskTable clickedRow]);
            NSString* type = [[NSString alloc] initWithUTF8String:(*taskHolder)->getType()];
            [[taskControlers objectForKey:type] beginEditTask:taskHolder 
                                               modalForWindow:_window 
                                                modalDelegate:self
                                                  endSelector:@selector(didEndEditTask:context:) 
                                                     editType:taskModify];
        }
    }catch (std::bad_alloc){
        NSLog(@"bad_alloc detected");
    }
}

//----------------------------------------------------------------------
// タスクリストのdata source実装
//----------------------------------------------------------------------
- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView
{
    return TaskScheduler::getInstance().getTaskNum();
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn 
            row:(NSInteger)rowIndex
{
    TaskBasePtr task = TaskScheduler::getInstance().getTask(rowIndex); 
    if ([[aTableColumn identifier] isEqualToString:@"icon"]){
        NSString* imgname = nil;
        switch (task->getState()){
            case TaskBase::TASK_INITIAL:
            case TaskBase::TASK_PREPARED:
                imgname = @"task_prepared.png";
                break;
            case TaskBase::TASK_RUNNING:
                imgname = @"task_running.png";
                break;
            case TaskBase::TASK_SUCCEED:
                imgname = @"icon_yes.png";
                break;
            case TaskBase::TASK_ERROR:
                imgname = @"icon_no.png";
                break;
            case TaskBase::TASK_SUSPENDED:
                imgname = @"task_suspend.png";
                break;
            default:
                return nil;
        }
        return [[NSImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForImageResource:imgname]];
    }else if([[aTableColumn identifier] isEqualToString:@"name"]){
        return [[NSString alloc] initWithUTF8String:task->getName()];
    }else if ([[aTableColumn identifier] isEqualToString:@"type"]){
        return [[NSString alloc] initWithUTF8String:task->getType()];
    }else if ([[aTableColumn identifier] isEqualToString:@"state"]){
        switch (task->getState()){
            case TaskBase::TASK_INITIAL:
            case TaskBase::TASK_PREPARED:
                return @"PREPARED";
            case TaskBase::TASK_RUNNING:
                return @"RUNNING";
            case TaskBase::TASK_SUCCEED:
                return @"SUCCEEDED";
            case TaskBase::TASK_ERROR:
                return @"ERROR";
            case TaskBase::TASK_SUSPENDED:
                return @"PAUSE";
            default:
                return nil;
        }
    }else if ([[aTableColumn identifier] isEqualToString:@"progress"]){
        return [NSNumber numberWithInt:task->getProgress()];
    }else if ([[aTableColumn identifier] isEqualToString:@"message"]){
        return [[NSString alloc] initWithUTF8String:task->getResultMessage()];
    }
    
    return nil;
}
//----------------------------------------------------------------------
// タスクリスト表示フック
//----------------------------------------------------------------------
#define BASELINE_OFFSET -10.0
- (void)tableView:(NSTableView *)tableView 
  willDisplayCell:(id)cell 
   forTableColumn:(NSTableColumn *)tableColumn 
              row:(NSInteger)row
{
    TaskBasePtr task = TaskScheduler::getInstance().getTask(row);
    
    if (TaskScheduler::getInstance().getCurrent() < 0){
        [progressView removeFromSuperview];
        progressView = nil;
    }else if (!progressView){
        progressView = [[NSProgressIndicator alloc] init];
        [progressView setStyle:NSProgressIndicatorSpinningStyle];
		[progressView setControlSize:NSSmallControlSize];
		[progressView startAnimation:nil];
		[progressView setHidden:NO];
    }

    if ([[tableColumn identifier] isEqualToString:@"progress"]){
        if (task->getState() == TaskBase::TASK_ERROR){
            [cell setWarningValue:0.0];
            [cell setCriticalValue:0.5];
        }else{
            [cell setWarningValue:100.0];
            [cell setCriticalValue:100.0];
        }
    }else if (![[tableColumn identifier] isEqualToString:@"icon"]){
        NSMutableAttributedString* str = 
        [[NSMutableAttributedString alloc] initWithAttributedString:[cell attributedStringValue]];
        NSRange range = {0, [str length]};
        NSDictionary* attr = nil;
        if (task->getState() == TaskBase::TASK_ERROR && ![tableView isRowSelected:row]){
            attr = [NSDictionary dictionaryWithObjectsAndKeys:
                    [NSColor colorWithSRGBRed:0.7 green:0.0 blue:0.0 alpha:1.0], NSForegroundColorAttributeName,
                    [NSNumber numberWithDouble:BASELINE_OFFSET], NSBaselineOffsetAttributeName,
                    nil];
        }else if (task->getState() == TaskBase::TASK_RUNNING && ![tableView isRowSelected:row]){
            attr = [NSDictionary dictionaryWithObjectsAndKeys:
                    [NSColor colorWithSRGBRed:0.0 green:0.0 blue:0.8 alpha:1.0], NSForegroundColorAttributeName,
                    [NSNumber numberWithDouble:BASELINE_OFFSET], NSBaselineOffsetAttributeName,
                    nil];
        }else if ((task->getState() == TaskBase::TASK_PREPARED || task->getState() == TaskBase::TASK_SUSPENDED) && 
                  ![tableView isRowSelected:row]){
            attr = [NSDictionary dictionaryWithObjectsAndKeys:
                    [NSColor grayColor], NSForegroundColorAttributeName,
                    [NSNumber numberWithDouble:BASELINE_OFFSET], NSBaselineOffsetAttributeName,
                    nil];
        }else{
            attr = [NSDictionary dictionaryWithObjectsAndKeys:
                    [NSNumber numberWithDouble:BASELINE_OFFSET], NSBaselineOffsetAttributeName,
                    nil];            
        }
        [str addAttributes:attr range:range];
        [cell setAttributedStringValue:str];
        
        if ([[tableColumn identifier] isEqualToString:@"state"]){
            if (task->getState() == TaskBase::TASK_RUNNING){
                [cell setProgress:progressView];
                [tableView addSubview:progressView];
            }else{
                [cell setProgress:nil];                
            }
        }
    }
}

//----------------------------------------------------------------------
// タスクリストの選択範囲変更
//----------------------------------------------------------------------
- (void)tableViewSelectionDidChange:(NSNotification *)aNotification
{
    [self reflectSchedulingStateToControls];
}

//----------------------------------------------------------------------
// タスクリストへのドラッグアンドドロップ処理
//----------------------------------------------------------------------
- (NSDragOperation) tableView:(NSTableView *)aTableView 
                 validateDrop:(id < NSDraggingInfo >)info 
                  proposedRow:(NSInteger)row 
        proposedDropOperation:(NSTableViewDropOperation)operation
{
    NSPasteboard *pboard = [info draggingPasteboard];    
    NSArray *nsFiles = [pboard propertyListForType:NSFilenamesPboardType];
    
    std::vector<std::string> files;
    NSUInteger items = [nsFiles count];
    for (int i = 0; i < items; i++){
        NSString* path = [nsFiles objectAtIndex:i];
        files.push_back([path UTF8String]);
    }
    
    return TaskFactory::getInstance().isAcceptableFiles(files) ? NSDragOperationCopy : NSDragOperationNone;
}

- (BOOL)tableView:(NSTableView *)aTableView 
       acceptDrop:(id < NSDraggingInfo >)info 
              row:(NSInteger)row 
    dropOperation:(NSTableViewDropOperation)operation
{
    NSPasteboard *pboard = [info draggingPasteboard];    
    NSArray *nsFiles = [pboard propertyListForType:NSFilenamesPboardType];
    
    int rc = true;
    std::vector<std::string>* files = NULL;
    try {
        // ファイル一覧をNSArrayからstd::vectorに変換
        files = new std::vector<std::string>;
        NSUInteger items = [nsFiles count];
        for (int i = 0; i < items; i++){
            NSString* path = [nsFiles objectAtIndex:i];
            files->push_back([path UTF8String]);
        }
        
        // タスクオブジェクト生成 & 編集モーダルシート起動
        rc = [self newTaskAndEdit:files password:NULL];
    }catch (std::bad_alloc){
        NSLog(@"bad_alloc detected");
        rc = false;
    }

    return rc;
}

//----------------------------------------------------------------------
// タスク生成 & 編集
//----------------------------------------------------------------------
- (BOOL)newTaskAndEdit:(std::vector<std::string>*)files password:(const char*)password
{
    bool rc = true;
    
    try {
        // タスクオブジェクト生成
        *taskHolder = TaskScheduler::getInstance().newTask(*files, password);

        // 編集モーダルシート起動
        NSString* type = [[NSString alloc] initWithUTF8String:(*taskHolder)->getType()];
        rc = [[taskControlers objectForKey:type] beginEditTask:taskHolder 
                                                modalForWindow:_window 
                                                 modalDelegate:self
                                                   endSelector:@selector(didEndEditTask:context:) 
                                                       editType:taskNew] == taskOK;
    }catch (TaskFactory::NeedPasswordException){
        // パスワードが必要なファイルの場合：パスワード入力モーダルシートを起動
        [_passwordField setStringValue:@""];
        [[NSApplication sharedApplication] beginSheet:[_passwordSheet window]
                                       modalForWindow:_window 
                                        modalDelegate:self 
                                       didEndSelector:@selector(didEndPasswordSheet:returnCode:contextInfo:)
                                          contextInfo:files];
        return true;
    }catch (TaskFactory::OtherException& e){
        rc = false;
        [self beginErrorSheetWithTitle:@"File open error" 
                               message:[[NSString alloc] initWithUTF8String:e.message.c_str()]];
    }catch (std::bad_alloc){
        NSLog(@"bad_alloc detected");
        rc = false;
    }
    
    delete files;
    return rc;
}

//----------------------------------------------------------------------
// タスク編集応答処理
//----------------------------------------------------------------------
- (void) didEndEditTaskWith:(NSInteger)result kind:(NSInteger)kind
{
    try {
        if (result == taskOK){
            if (kind == taskNew){
                TaskScheduler::getInstance().addTask(*taskHolder);
                [_taskTable scrollToEndOfDocument:self];                
            }else{
                TaskScheduler::getInstance().commit();
            }
        }
        [_taskTable reloadData];
        [_taskTable scrollToEndOfDocument:self];
        [self reflectSchedulingStateToControls];
        [self reflectSchedulingStateToLCD];
    } catch (TaskFactory::OtherException& e) {
        [self beginErrorSheetWithTitle:@"An error occurred until editing a task" 
                               message:[[NSString alloc] initWithUTF8String:e.message.c_str()]];
    }
    
    taskHolder->setNull();
}


//----------------------------------------------------------------------
// パスワードシート応答処理
//----------------------------------------------------------------------
- (void)didEndPasswordSheet:(NSWindow *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
    std::vector<std::string>* files = reinterpret_cast<std::vector<std::string>*>(contextInfo);
    
    if (returnCode == NSOKButton) {
        // タスクオブジェクト生成 & 編集モーダルシート起動
        [self newTaskAndEdit:files password:[[_passwordField stringValue] UTF8String]];
	} else {
        delete files;
	}
}

- (IBAction)passwordOK:(id)sender 
{
	[[_passwordSheet window] close];
	[[NSApplication sharedApplication] endSheet:[_passwordSheet window] returnCode:NSOKButton];
}

- (IBAction)passwordCancel:(id)sender 
{
	[[_passwordSheet window] close];
	[[NSApplication sharedApplication] endSheet:[_passwordSheet window] returnCode:NSCancelButton];
}

//----------------------------------------------------------------------
// エラーメッセージシート
//----------------------------------------------------------------------
- (void)beginErrorSheetWithTitle:(NSString *)title message:(NSString *)message
{
     NSBeginCriticalAlertSheet(title, @"", @"", @"", _window, self,
                       nil, nil,
                       nil, message);
}

//----------------------------------------------------------------------
// タスク実行スレッド
//----------------------------------------------------------------------
- (void)executeTask
{
    [[NSThread currentThread] setThreadPriority:0.0];
    @autoreleasepool {
        TaskScheduler::getInstance().schedule(gatewayForUnrarNotification, (__bridge void*)self);
    }
}

//----------------------------------------------------------------------
// タスク変更通知
//----------------------------------------------------------------------
- (void)changeTask:(NSNumber*)taskIndex
{
    if ([taskIndex intValue] >= 0){
        [_taskTable reloadDataForRowIndexes:[NSIndexSet indexSetWithIndex:[taskIndex intValue]]
                              columnIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, 5)]];
        TaskBasePtr task = TaskScheduler::getInstance().getTask([taskIndex intValue]);
        TaskBase::TaskProgress p;
        task->getProgress(p);
        [_indicator updateTaskProgress:[[LCDTaskProgress alloc] 
                                        initWithName:[[NSString alloc] initWithUTF8String:task->getName()] 
                                        currentPhase:[[NSString alloc] initWithUTF8String:p.phase] 
                                        totalSize:p.total completeSize:p.complete]];
    }else{
        [_taskTable reloadData];
        [self reflectSchedulingStateToControls];
        if ([taskIndex intValue] == -2){
            // scheduler will be state in deep-sleep
            [self reflectSchedulingStateToLCD];
            if (skipFistDeepSleep){
                skipFistDeepSleep = false;
            }else{
                [notifier notify];
            }
        }
    }
}

//----------------------------------------------------------------------
// C++のタスクスケジューラから通知されるタスク変更通知をObjectiveCのメソッドにゲートウェイ
//----------------------------------------------------------------------
static void gatewayForUnrarNotification(int taskIndex, void* context)
{
    AppDelegate* ad = (__bridge AppDelegate*)context;
    NSNumber* index = [NSNumber numberWithInt:taskIndex];
    [ad performSelectorOnMainThread:@selector(changeTask:) withObject:index waitUntilDone:false];
}

@end
