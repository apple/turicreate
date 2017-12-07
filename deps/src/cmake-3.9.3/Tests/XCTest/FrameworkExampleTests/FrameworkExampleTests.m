#import <XCTest/XCTest.h>

#import "FrameworkExample/FrameworkExample.h"

@interface FrameworkExampleTests : XCTestCase

@end

@implementation FrameworkExampleTests

- (void)testFourtyTwo {
    // This is an example of a functional test case.
    XCTAssertEqual(42, FourtyTwo());
}

@end
