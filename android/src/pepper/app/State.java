// Copyright (C) 2012 Mihai Preda

package pepper.app;

import android.graphics.*;

public class State {
    Canvas canvas;    

    native void draw();
    
    void background(int r, int g, int b) {
        canvas.drawARGB(255, r, g, b);
    }
}
