//
//  VegaRenderer.h
//  VegaRenderer
//
//  Created by Zachary Nation on 12/16/18.
//  Copyright © 2018 Apple Inc. All rights reserved.
//

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

@interface VegaRenderer : NSObject

- (instancetype)initWithSpec:(NSString*)spec;
- (instancetype)initWithSpec:(NSString*)spec
                     context:(CGContextRef)parentContext;

-(NSUInteger)width;
-(NSUInteger)height;
-(CGImageRef)CGImage;
-(CGLayerRef)CGLayer;

// JS dependencies
+(NSString *)vegaJS;
+(NSString *)vegaliteJS;

@end
