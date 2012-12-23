//
//  TaskTableView.m
//  ExtractManager
//
//  Created by opiopan on 2012/12/18.
//
//

#import "TaskTableView.h"

@implementation TaskTableView

- (void)keyDown:(NSEvent *)theEvent
{
    NSString* key = [theEvent charactersIgnoringModifiers];
    if([key isEqual:@"\r"]) {
        if ([self selectedRow] != -1){
            [_delegate performSelector:@selector(OnEditTask:) withObject:self];
        }
    } else {
        [super keyDown:theEvent];
    }
}

@end
