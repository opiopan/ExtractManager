//
//  LCDCell.h
//  ExtractManager
//

#import <AppKit/AppKit.h>

enum LCDSchedulerState{
    SCHEDULER_RUNNING = 1,
    SCHEDULER_SUSPENDING,
    SCHEDULER_FINISH
};

//----------------------------------------------------------------------
// LCDTaskStatistcs: タスク統計情報
//----------------------------------------------------------------------
@interface LCDTaskStatistics : NSObject

@property (assign) NSInteger succeededCount;
@property (assign) NSInteger errorCount;
@property (assign) NSInteger suspendedCount;
@property (assign) NSInteger runnningCount;
@property (assign) NSInteger preparedCount;

- (id) initWithSucceededTaskCount:(NSInteger)suceeded
                   errorTaskCount:(NSInteger)error
               suspendedTaskCount:(NSInteger)suspended
                 runningTaskCount:(NSInteger)running
                preparedTaskCount:(NSInteger)prepared;
- (BOOL) equalToTaskStatistics:(LCDTaskStatistics*)stat;

@end

//----------------------------------------------------------------------
// LCDTaskProgress: タスク進捗
//----------------------------------------------------------------------
@interface LCDTaskProgress : NSObject

@property (strong, readonly) NSString* name;
@property (strong, readonly) NSString* currentPhase;
@property (assign, readonly) NSInteger totalSize;
@property (assign, readonly) NSInteger completeSize;

- (id) initWithName:(NSString*)taskName 
       currentPhase:(NSString*)phase 
          totalSize:(NSInteger)total 
       completeSize:(NSInteger)complete;

@end

//----------------------------------------------------------------------
// LCDCell: LCD描画クラス
//----------------------------------------------------------------------
@interface LCDCell : NSImageCell

- (void) setTaskStatistics:(LCDTaskStatistics*)stat withProgress:(LCDTaskProgress*)p;
- (void) updateTaskProgress:(LCDTaskProgress*)progress;

@end
