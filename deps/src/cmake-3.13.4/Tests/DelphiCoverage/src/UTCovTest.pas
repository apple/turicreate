//---------------------------------------------------------------------------
// Copyright 2012 The Open Source Electronic Health Record Agent
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//---------------------------------------------------------------------------
unit UTCovTest;
interface
uses UnitTest, TestFrameWork,SysUtils,Windows;

implementation
type
UTCovTestTests=class(TTestCase)
  public
  procedure SetUp; override;
  procedure TearDown; override;

  published
    procedure TestCov1;
    procedure TestCov2;
    procedure TestCov3;
  end;

procedure NotRun;
begin
    WriteLn('This line will never run');
end;
procedure UTCovTestTests.SetUp;
begin
end;

procedure UTCovTestTests.TearDown;
begin
end;

procedure UTCovTestTests.TestCov1;
begin
  {
  Block comment lines
  }
  CheckEquals(1,2-1);
end;

procedure UTCovTestTests.TestCov2;
var
  i:DWORD;
begin
  for i := 0 to 1 do
    WriteLn( IntToStr(i));
  // Comment
  CheckEquals(i,2);
end;

procedure UTCovTestTests.TestCov3;
var
 i : DWORD;
begin
  i := 0;
  while i < 5 do
   i := i+1;
  CheckEquals(i,5);
end;

begin
  UnitTest.addSuite(UTCovTestTests.Suite);
end.