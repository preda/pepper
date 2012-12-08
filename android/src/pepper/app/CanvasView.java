// Copyright (C) 2012 Mihai Preda

package pepper.app;

import android.app.*;
import android.os.*;
import android.view.*;
import android.graphics.*;
import android.content.*;

class CanvasView extends View {
    State state = new State();

    CanvasView(Context context) {
        super(context);
    }

    protected void onDraw(Canvas canvas) {
        state.canvas = canvas;
        state.draw();
        // canvas.drawColor(0xff00ff00);
        state.canvas = null;
    }

    protected void onSizeChanged(int w, int h, int ow, int oh) {

    }


}
