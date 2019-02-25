#import <XCTest/XCTest.h>

#import "StaticLibExample/StaticLibExample.h"

@interface StaticLibExampleTests : XCTestCase

@end

@implementation StaticLibExampleTests

- (void)testFourtyFour {
    // This is an example of a functional test case.
    XCTAssertEqual(44, FourtyFour());
}

@end
