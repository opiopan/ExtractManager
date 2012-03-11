//
//  TaskController.h
//  ExtractManager
//

#import <cocoa/Cocoa.h>
#import "Task.h"

#define taskOK              0
#define taskNG              -1
#define taskNeedPassword    100

#define taskNew             1
#define taskModify          2

@protocol TaskController <NSWindowDelegate>

- (NSInteger) beginEditTask:(TaskBasePtr *)task 
             modalForWindow:(NSWindow*)window 
              modalDelegate:(id)delegate
                endSelector:(SEL)selector 
                    editType:(NSInteger)type;

@end
