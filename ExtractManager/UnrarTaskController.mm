//
//  UnrarTaskController.m
//  ExtractManager
//

#import "UnrarTaskController.h"
#import "AppDelegate.h"
#import "NSString+NormalizedString.h"
#import "UnrarTreeNameCell.h"

@implementation UnrarTaskController
{
    UnrarTask*                  task;
    NSWindow*                   modalWindow;
    id                          delegate;
    SEL                         selectorForNotify;
    NSInteger                   editType;
    
    UnrarTask::TaskProperties*  taskProps;
    
    NSInteger                   editingRow;
}

@synthesize panel;
@synthesize targetDir;
@synthesize password;
@synthesize comment;
@synthesize volumes;
@synthesize toBeDeleted;
@synthesize toUpdateTimestamp;
@synthesize nodeTable;
@synthesize language;

//----------------------------------------------------------------------
// オブジェクト初期化
//----------------------------------------------------------------------
- (id) init
{
    self = [super init];
    
    if (self){
        [NSBundle loadNibNamed:@"UnrarTask" owner:self];
        editingRow = -1;
    }

    [nodeTable setTarget:self];
    [nodeTable setDoubleAction:@selector(onNodeTableDouble:)];
    
    return self;
}

- (void) allocCppContext
{
    taskProps = new UnrarTask::TaskProperties;
}

- (void) destroyCppContext
{
    delete taskProps;
    taskProps = NULL;
}

//----------------------------------------------------------------------
// シート表示
//----------------------------------------------------------------------
- (void) modifyCommentAttribute
{
    NSTextStorage* storage = [comment textStorage];
    if ([storage length] > 0){
        NSRange range;
        NSFont* font = [storage attribute:NSFontAttributeName atIndex:1 effectiveRange:&range];
        NSFont* newFont = [NSFont fontWithDescriptor:[font fontDescriptor] size:11.0];
        NSDictionary* attr = [NSDictionary dictionaryWithObjectsAndKeys:
                              newFont, NSFontAttributeName, 
                              nil];
        range.location = 0;
        range.length = [storage length];
        [storage addAttributes:attr range:range];
    }
}

- (NSInteger) beginEditTask:(TaskBasePtr *)t 
             modalForWindow:(NSWindow*)window 
              modalDelegate:(id)del
                endSelector:(SEL)selector 
                    editType:(NSInteger)type
{
    // C++用コンテキスト域割当
    try {
        [self allocCppContext];
    } catch (std::bad_alloc) {
        return taskNG;
    }
    editingRow = -1;
    
    // 完了通知用コンテキストを保持
    task = reinterpret_cast<UnrarTask*>(t->operator ->());
    modalWindow = window;
    delegate = del;
    selectorForNotify = selector;
    editType = type;
    
    // 編集用タスク属性を取得
    task->getProperties(*taskProps);
    
    // シート初期値を設定
    [targetDir setStringValue:[[NSString alloc] initWithUTF8String:taskProps->baseDir.c_str()]];
    [password setStringValue:[[NSString alloc] initWithUTF8String:taskProps->password.c_str()]];
    [comment setEditable:true];
    [comment selectAll:self];
    [comment insertText:[[NSString alloc] initWithUTF8String:task->getComment()]];
    [comment setSelectedRange:NSMakeRange(0,0)];
    [comment scrollToBeginningOfDocument:self];
    [self modifyCommentAttribute];
    [comment setEditable:false];
    NSMutableString *vols = [[NSMutableString alloc] initWithUTF8String:""];
    for (std::vector<std::string>::iterator i = task->getVolumes().begin();
         i != task->getVolumes().end(); i++){
        unsigned long pos = i->find_last_of("/");
        if (pos == std::string::npos){
            [vols appendString:[[NSString alloc] initWithUTF8String:i->c_str()]];
        }else{
            [vols appendString:[[NSString alloc] initWithUTF8String:i->substr(pos + 1).c_str()]];
        }
        [vols appendString:@"\n"];
    }
    [volumes setStringValue:vols];
    [toBeDeleted setState:taskProps->flagToBeDeleted ? NSOnState : NSOffState];
    [toUpdateTimestamp setState:taskProps->flagToUpdateTimestamp ? NSOnState : NSOffState];
    [language removeAllItems];
    for (int lid = 0; lid < task->getSupportedLanguageNum(); lid++){
        [language addItemWithTitle:NSLocalizedString([NSString stringWithUTF8String:task->getLanguageName(lid)], nil)];
        [[language itemAtIndex:lid] setTag:lid];
    }
    [language selectItemAtIndex:taskProps->languageID];
    
    // コントロール状態設定
    [nodeTable reloadData];
    [nodeTable deselectAll:self];
    
    // タスク編集シートを開始
    [[NSApplication sharedApplication] beginSheet:panel
                                   modalForWindow:modalWindow 
                                    modalDelegate:self 
                                   didEndSelector:@selector(didEndEditSheet:returnCode:contextInfo:) 
                                      contextInfo:nil];
    
    return taskOK;
}

- (void)didEndEditSheet:(NSWindow *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
    if (returnCode == NSOKButton){
        taskProps->baseDir = [[targetDir stringValue] UTF8String];
        taskProps->password = [[password stringValue] UTF8String];
        taskProps->flagToBeDeleted = ([toBeDeleted state] == NSOnState);
        taskProps->flagToUpdateTimestamp = ([toUpdateTimestamp state] == NSOnState);
        
        task->setProperties(*taskProps);
    }

    [self destroyCppContext];
        
    NSInteger rc = (returnCode == NSOKButton) ? taskOK : taskNG;    
    [delegate didEndEditTaskWith:rc kind:editType];
}

//----------------------------------------------------------------------
// アクション応答
//----------------------------------------------------------------------

// OKボタン押下
- (IBAction)onOK:(id)sender {
	[panel close];
	[[NSApplication sharedApplication] endSheet:panel returnCode:NSOKButton];
}

// Cancelボタン押下
- (IBAction)onCancel:(id)sender {
	[panel close];
	[[NSApplication sharedApplication] endSheet:panel returnCode:NSCancelButton];
}

// ノード削除（単独）
- (IBAction)onDeleteNode:(id)sender {
    std::vector<UnrarTreeNode*> targets;
    NSIndexSet* indexes = [nodeTable selectedRowIndexes];
    for (NSUInteger index = [indexes firstIndex]; 
         index != NSNotFound; 
         index = [indexes indexGreaterThanIndex:index]){
        if (taskProps->currentNode->getChild(index)->getType() != UnrarTreeNode::NODE_PARENT_REF){
            targets.push_back(taskProps->currentNode->getChild(index));
        }
    }
    
    for (std::vector<UnrarTreeNode*>::iterator i = targets.begin(); i != targets.end(); i++){
        for (int j = 0; j < taskProps->currentNode->getChildNum(); j++){
            UnrarTreeNode* node = taskProps->currentNode->getChild(j);
            if (*i == node){
                if (node->getType() == UnrarTreeNode::NODE_FILE){
                    taskProps->currentNode->deleteChild(j);
                }else if (node->getType() == UnrarTreeNode::NODE_FOLDER){
                    taskProps->currentNode->pullFolder(j);
                }
            }
        }
    }
    
    [nodeTable reloadData];
    [nodeTable deselectAll:self];
}

// ノード削除（再帰）
- (IBAction)onDeleteNodeRecurce:(id)sender {
    std::vector<UnrarTreeNode*> targets;
    NSIndexSet* indexes = [nodeTable selectedRowIndexes];
    for (NSUInteger index = [indexes firstIndex]; 
         index != NSNotFound; 
         index = [indexes indexGreaterThanIndex:index]){
        if (taskProps->currentNode->getChild(index)->getType() != UnrarTreeNode::NODE_PARENT_REF){
            targets.push_back(taskProps->currentNode->getChild(index));
        }
    }
    
    for (std::vector<UnrarTreeNode*>::iterator i = targets.begin(); i != targets.end(); i++){
        for (int j = 0; j < taskProps->currentNode->getChildNum(); j++){
            if (*i == taskProps->currentNode->getChild(j)){
                taskProps->currentNode->deleteChild(j);
            }
        }
    }
    
    [nodeTable reloadData];
    [nodeTable deselectAll:self];
}

// ノードツリーリセット
- (IBAction)onResetTree:(id)sender {
    taskProps->tree = taskProps->initialTree->clone();
    taskProps->currentNode = taskProps->tree.operator->();
    [nodeTable reloadData];
    [nodeTable deselectAll:self];
}

// パスワードセットボタン押下
- (IBAction)onSetPassword:(id)sender {
    if ([comment selectedRange].length > 0){
        NSAttributedString* astr = [comment attributedSubstringFromRange:[comment selectedRange]];
        taskProps->password = [[astr string] UTF8String];
        [password setStringValue:[astr string]];
    }
}

// ノードテーブルビュー上でのダブルクリック（フォルダ遷移）
- (void)onNodeTableDouble:(id)sender
{
    if ([nodeTable clickedRow] >= 0){
        UnrarTreeNode* node = taskProps->currentNode->getChild([nodeTable clickedRow]);
        if (node->getType() == UnrarTreeNode::NODE_FOLDER){
            taskProps->currentNode = node;
            [nodeTable reloadData];
            [nodeTable deselectAll:self];
        }else if (node->getType() == UnrarTreeNode::NODE_PARENT_REF){
            taskProps->currentNode = taskProps->currentNode->getParent();
            [nodeTable reloadData];
            [nodeTable deselectAll:self];
        }
    }
}

// Langage Popup Button の値変更
- (IBAction)onLanguageButton:(id)sender {
    long index = [language selectedTag];
    if (taskProps->languageID != index){
        try {
            int32_t lid = static_cast<int32_t>(index);
            int32_t eid = -1;
            std::vector<UnrarElement> elm;
            taskProps->initialTree = task->getTreeWithEncoding(lid, eid, elm);
            taskProps->languageID = lid;
            taskProps->encodingID = eid;
            taskProps->elements = elm;
            taskProps->tree = taskProps->initialTree->clone();
            taskProps->currentNode = taskProps->tree.operator->();
            [nodeTable reloadData];
            [nodeTable deselectAll:self];
        }catch (TaskFactory::OtherException e){
        }
        [language selectItemAtIndex:taskProps->languageID];
    }
}

//----------------------------------------------------------------------
// ノードテーブルの編集操作
//----------------------------------------------------------------------
- (BOOL)tableView:(NSTableView *)aTableView shouldEditTableColumn:(NSTableColumn *)aTableColumn 
              row:(NSInteger)rowIndex
{
    UnrarTreeNode* node = taskProps->currentNode->getChild(rowIndex);

    if (node->getType() == UnrarTreeNode::NODE_PARENT_REF){
        return false;
    }else{
        editingRow = rowIndex;
        return true;
    }
}

- (BOOL)tableView:(NSTableView *)aTableView textShouldEndEditing:(NSText *)textObject
{
    if (editingRow >= 0){
        UnrarTreeNode* node = taskProps->currentNode->getChild(editingRow);
        node->changeName([[textObject string] UTF8String]); 
        editingRow = -1;
    }
    
    return true;
}

//----------------------------------------------------------------------
// ノードテーブル用data source
//----------------------------------------------------------------------
- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView
{
    if (taskProps){
        return taskProps->currentNode->getChildNum();
    }else{
        return 0;
    }
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn 
            row:(NSInteger)rowIndex
{
    if (taskProps){
        UnrarTreeNode* node = taskProps->currentNode->getChild(rowIndex);
        
        if ([[aTableColumn identifier] isEqualToString:@"icon"]){
            if (node->getType() == UnrarTreeNode::NODE_FILE){
                NSString* name = [[NSString alloc] initWithUTF8String:node->getName()];
                NSImage* image = [[NSWorkspace sharedWorkspace] iconForFileType:[name pathExtension]];
                return image;
            }else{
                return [[NSWorkspace sharedWorkspace] iconForFile:@"/var"];
            }
        }else if ([[aTableColumn identifier] isEqualToString:@"name"]){
            UnrarTreeNameCell* cell = [aTableColumn dataCell];
            cell.isDirectory = (node->getType() != UnrarTreeNode::NODE_FILE);
            return [[NSString alloc] initWithUTF8String:node->getName()];
        }else if ([[aTableColumn identifier] isEqualToString:@"size"]){
            if (node->getType() == UnrarTreeNode::NODE_FILE){
                return [NSString normalizedStringWithIntegerInByte:node->getSize()];
            }else{
                return @"---";
            }
        }

    }
    
    return nil;
}

@end
