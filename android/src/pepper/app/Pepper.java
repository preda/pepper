// Copyright (C) 2012 Mihai Preda

package pepper.app;

import android.app.*;
import android.os.*;
import android.view.*;

public class Pepper extends Activity {
    static {
        System.loadLibrary("pepper");
    }

    public void onCreate(Bundle b) {
        super.onCreate(b);
        setContentView(new CanvasView(this));
    }

    public void onDestroy() {
        super.onDestroy();
    }
}
