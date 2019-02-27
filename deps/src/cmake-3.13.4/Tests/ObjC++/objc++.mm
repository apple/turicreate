#import <Cocoa/Cocoa.h>
#import <iostream>
using namespace std;

int main()
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  NSMutableSet *mySet = [NSMutableSet set];
  cout<<"Adding values to the set."<<endl;
  [mySet addObject:[NSNumber numberWithInt:356]];
  [mySet addObject:[NSNumber numberWithInt:550]];
  [mySet addObject:[NSNumber numberWithInt:914]];

  cout<<"The set contains "<<[mySet count]<<" objects."<<endl;
  if ([mySet containsObject:[NSNumber numberWithInt:911]])
    cout<<"It's there!"<<endl;
  else
    cout<<"It's not there."<<endl;

  [pool release];
  return 0;
}
