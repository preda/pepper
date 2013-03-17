// Copyright (C) 2012 - 2013 Mihai Preda

package pepper.app;

import android.app.*;
import android.os.*;
import android.view.*;

public class PepperApp extends Activity {
    public void onCreate(Bundle b) {
        super.onCreate(b);
        setContentView(new CanvasView(this));
    }

    public void onDestroy() {
        super.onDestroy();
    }
}
