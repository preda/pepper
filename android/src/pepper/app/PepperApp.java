// Copyright (C) 2012 - 2013 Mihai Preda

package pepper.app;

import android.app.*;
import android.os.*;
import android.view.*;
import android.util.Log;

public class PepperApp extends Activity {
    public void onCreate(Bundle b) {
        super.onCreate(b);
        setContentView(new CanvasView(this));
        State state = new State();
        int ret = state.run("func f(x) { return x*x; }; return f(9);");
        Log.d("**** Pepper", "ret " + ret);
    }

    public void onDestroy() {
        super.onDestroy();
    }
}
