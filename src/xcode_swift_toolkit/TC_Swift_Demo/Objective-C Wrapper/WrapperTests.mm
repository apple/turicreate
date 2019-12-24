//
//  WrapperTests.m
//  TC_Swift_DemoTests
//
//  Created by Hoyt Koepke on 12/23/19.
//  Copyright © 2019 Hoyt Koepke. All rights reserved.
//

#import <XCTest/XCTest.h>

#include <model_server_v2/model_server.hpp>

using namespace turi;
using namespace turi::v2;

@interface WrapperTests : XCTestCase

@end

@implementation WrapperTests



- (void)testWrapping {
    // This is an example of a functional test case.
    // Use XCTAssert and related functions to verify your tests produce the correct results.
    
    auto m = model_server().create_model("SwiftWrapperTest");
    
    m->call_method("append_string", "This");
    m->call_method("append_string", " is");
    m->call_method("append_string", " Fun!");
    
    std::string result = variant_get_value<std::string>(m->call_method("get_string"));
    
    XCTAssert(result == "This is Fun!");
}


@end
