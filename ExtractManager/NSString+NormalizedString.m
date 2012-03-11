//
//  NSString+NormalizedString.m
//  ExtractManager

#import "NSString+NormalizedString.h"

@implementation NSString (NormalizedString)

+ (NSString *)normalizedStringWithIntegerInByte:(NSInteger)numInByte
{
    static const NSInteger KB = 1024;
    static const NSInteger MB = KB * 1024;
    static const NSInteger GB = MB * 1024;
    static const NSInteger TB = GB * 1024;
    
    if (numInByte >= TB * 100){
        double normal = numInByte / (double)TB;
        return [NSString stringWithFormat:@"%.0f TB", normal];        
    }else if (numInByte >= TB * 10){
        double normal = numInByte / (double)TB;
        return [NSString stringWithFormat:@"%.1f TB", normal];        
    }else if (numInByte >= TB * 0.9){
        double normal = numInByte / (double)TB;
        return [NSString stringWithFormat:@"%.2f TB", normal];        
    }else if (numInByte >= GB * 100){
        double normal = numInByte / (double)GB;
        return [NSString stringWithFormat:@"%.0f GB", normal];        
    }else if (numInByte >= GB * 10){
        double normal = numInByte / (double)GB;
        return [NSString stringWithFormat:@"%.1f GB", normal];        
    }else if (numInByte >= GB * 0.9){
        double normal = numInByte / (double)GB;
        return [NSString stringWithFormat:@"%.2f GB", normal];        
    }else if (numInByte >= MB * 100){
        double normal = numInByte / (double)MB;
        return [NSString stringWithFormat:@"%.0f MB", normal];        
    }else if (numInByte >= MB * 10){
        double normal = numInByte / (double)MB;
        return [NSString stringWithFormat:@"%.1f MB", normal];        
    }else if (numInByte >= MB * 0.9){
        double normal = numInByte / (double)MB;
        return [NSString stringWithFormat:@"%.2f MB", normal];        
    }else if (numInByte >= KB * 100){
        double normal = numInByte / (double)KB;
        return [NSString stringWithFormat:@"%.0f KB", normal];        
    }else if (numInByte >= KB * 10){
        double normal = numInByte / (double)KB;
        return [NSString stringWithFormat:@"%.1f KB", normal];        
    }else if (numInByte >= KB * 0.9){
        double normal = numInByte / (double)KB;
        return [NSString stringWithFormat:@"%.2f KB", normal];        
    }else{
        return [NSString stringWithFormat:@"%ld B", numInByte];
    }
}

@end
