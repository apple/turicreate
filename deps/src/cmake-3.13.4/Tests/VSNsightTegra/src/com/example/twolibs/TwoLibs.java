/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.example.twolibs;

import android.app.Activity;
import android.widget.TextView;
import android.os.Bundle;

public class TwoLibs extends Activity
{
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        TextView  tv = new TextView(this);
        int       x  = 1000;
        int       y  = 42;

        // here, we dynamically load the library at runtime
        // before calling the native method.
        //
        System.loadLibrary("twolib-second");

        int  z = add(x, y);

        tv.setText( "The sum of " + x + " and " + y + " is " + z );
        setContentView(tv);
    }

    public native int add(int  x, int  y);
}
