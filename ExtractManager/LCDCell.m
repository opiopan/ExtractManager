//
//  LCDCell.m
//  ExtractManager
//

#import "LCDCell.h"
#import "NSString+NormalizedString.h"

@implementation LCDTaskStatistics

@synthesize succeededCount;
@synthesize errorCount;
@synthesize suspendedCount;
@synthesize runnningCount;
@synthesize preparedCount;

//----------------------------------------------------------------------
// LCDTaskStatistcs: タスク統計情報
//----------------------------------------------------------------------
- (id) initWithSucceededTaskCount:(NSInteger)suceeded 
                   errorTaskCount:(NSInteger)error 
               suspendedTaskCount:(NSInteger)suspended 
                 runningTaskCount:(NSInteger)running 
                preparedTaskCount:(NSInteger)prepared
{
    self = [self init];
    if (self){
        succeededCount = suceeded;
        errorCount = error;
        suspendedCount = suspended;
        runnningCount = running;
        preparedCount = prepared;
    }
    return self;
}

- (BOOL) equalToTaskStatistics:(LCDTaskStatistics *)stat
{
    return stat.succeededCount == succeededCount &&
           stat.errorCount == errorCount &&
           stat.suspendedCount == suspendedCount &&
           stat.runnningCount == runnningCount &&
           stat.preparedCount == preparedCount;
}

@end

//----------------------------------------------------------------------
// LCDTaskProgress: タスク進捗
//----------------------------------------------------------------------
@implementation LCDTaskProgress

@synthesize name;
@synthesize currentPhase;
@synthesize totalSize;
@synthesize completeSize;

- (id) initWithName:(NSString*)taskName 
       currentPhase:(NSString*)phase 
          totalSize:(NSInteger)total 
       completeSize:(NSInteger)complete
{
    self = [self init];
    if (self){
        name = taskName;
        currentPhase = phase;
        totalSize = total;
        completeSize = complete;
    }
    return self;
}

@end


//----------------------------------------------------------------------
// LCDCell: LCD描画クラス
//----------------------------------------------------------------------
@implementation LCDCell
{
    LCDTaskStatistics*  statistics;
    LCDTaskProgress*    progress;
}

// 統計情報設定
- (void) setTaskStatistics:(LCDTaskStatistics*)stat withProgress:(LCDTaskProgress*)p
{
    statistics = stat;
    progress = p;
    [[self controlView] display];
}

// タスク進捗更新
- (void) updateTaskProgress:(LCDTaskProgress*)p
{
    progress = p;
    statistics.runnningCount = 1;
    [[self controlView] display];
}

// 描画
- (void) drawWithFrame:(NSRect)cellFrame inView:(NSView *)controlView
{
    [super drawWithFrame:cellFrame inView:controlView];

    // 出力文字列生成
    NSString* title = nil;
    NSString* info1 = nil;
    NSString* info2 = nil;
    if (statistics.runnningCount > 0){
        title = [NSString stringWithFormat:NSLocalizedString(@"Running a task : %@", nil), progress.name];
        info1 = progress.currentPhase;
        if (progress.totalSize > 0){
            info2 = [NSString stringWithFormat:NSLocalizedString(@"%d %% [ %@ / %@ ] complete", nil),
                     (int)(progress.completeSize * 100 / progress.totalSize),
                     [NSString normalizedStringWithIntegerInByte:progress.completeSize], 
                     [NSString normalizedStringWithIntegerInByte:progress.totalSize]];
        }
    }else if (statistics.suspendedCount > 0){
        title = NSLocalizedString(@"Task scheduling is suspending", nil);
        info2 = [NSString stringWithFormat:NSLocalizedString(@"pause:%ld   succeeded:%ld   error:%ld", nil),
                 statistics.suspendedCount, statistics.succeededCount, statistics.errorCount];
    }else if (statistics.succeededCount > 0 || statistics.errorCount > 0){
        title = NSLocalizedString(@"All tasks finished", nil);
        info2 = [NSString stringWithFormat:NSLocalizedString(@"succeeded:%ld   error:%ld", nil),
                 statistics.succeededCount, statistics.errorCount];
    }else{
        title = NSLocalizedString(@"No tasks are registerd", nil);
        info2 = NSLocalizedString(@"Drag archive files to this window", nil);
    }

    // クリッピングリージョンの設定
    NSSize imageSize = [[self image] size];
    double clipdw = cellFrame.size.width - imageSize.width;
    cellFrame.origin.x += clipdw / 2 + 2;
    cellFrame.size.width -= clipdw + 4;
    NSBezierPath* clipPath = [NSBezierPath bezierPath];
    [clipPath appendBezierPathWithRect:cellFrame];
    [clipPath addClip];
    
    // タイトル行描画
    NSDictionary* attr = nil;
    NSSize strSize;
    NSRect frame;
    double dw;
    attr = [NSDictionary dictionaryWithObjectsAndKeys:
            [NSFont boldSystemFontOfSize:11], NSFontAttributeName,
            [NSColor blackColor], NSForegroundColorAttributeName,
            nil];
    strSize = [title sizeWithAttributes:attr];
    dw = (cellFrame.size.width - strSize.width) / 2.0;
    frame = cellFrame;
    if (dw > 0){
        frame.origin.x += dw;
    }else{
        frame.origin.x += 5;
    }
    frame.size.width -= dw * 2.0;
    frame.origin.y -= 5;
    [title drawInRect:frame withAttributes:attr];
    
#if 0
    // 情報行1描画
    attr = [NSDictionary dictionaryWithObjectsAndKeys:
            [NSFont systemFontOfSize:9], NSFontAttributeName,
            [NSColor blackColor], NSForegroundColorAttributeName,
            nil];
    strSize = [info1 sizeWithAttributes:attr];
    frame = cellFrame;
    frame.origin.x += (cellFrame.size.width - strSize.width) / 2;
    frame.origin.y -= 15;
    [info1 drawInRect:frame withAttributes:attr];
#endif

    // 情報行2描画
    attr = [NSDictionary dictionaryWithObjectsAndKeys:
            [NSFont systemFontOfSize:10], NSFontAttributeName,
            [NSColor blackColor], NSForegroundColorAttributeName,
            nil];
    strSize = [info2 sizeWithAttributes:attr];
    dw = (cellFrame.size.width - strSize.width) / 2.0;
    frame = cellFrame;
    if (dw > 0){
        frame.origin.x += dw;
    }
    frame.size.width -= dw * 2.0;
    frame.origin.y -= 24;
    [info2 drawInRect:frame withAttributes:attr];    
}

@end
