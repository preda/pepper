// Copyright (C) 2012 Mihai Preda

package pepper.app;

import android.graphics.*;
import android.util.Log;

public class State {
    private long handle;

    Canvas canvas;  

    State() {
        handle = init();
    }

    public void background(int r, int g, int b) {
        Log.d("Pepper", "back " + r + ' ' + g + ' ' + b);
        canvas.drawARGB(255, r, g, b);
    }

    void deinit() { deinit(handle); }
    void draw()   { draw(handle); }

    native long init();
    native void deinit(long handle);
    native void draw(long handle);   
}
