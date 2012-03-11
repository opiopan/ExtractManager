//
//  main.m
//  ExtractManager
//
//  Created by opiopan on 11/12/29.
//  Copyright (c) 2011年 opiopan. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <locale.h>
#import <stdlib.h>
#import "TaskScheduler.h"

int main(int argc, char *argv[])
{
    setlocale(LC_CTYPE, "ja_JP.UTF-8");
    
    // initialize the model layer befor creating view objects
    NSString* config = [[NSString alloc] 
                        initWithFormat:@"%@/Library/Application Support/ExtractManager/default.db",
                        [[NSString alloc] initWithUTF8String:getenv("HOME")]];
    TaskScheduler::getInstance().initialize([config UTF8String]);
    
    int rc = NSApplicationMain(argc, (const char **)argv);
    
    return rc;
}
