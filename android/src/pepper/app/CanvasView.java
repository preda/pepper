// Copyright (C) 2012 Mihai Preda

package pepper.app;

import android.app.*;
import android.os.*;
import android.view.*;
import android.graphics.*;
import android.content.*;

class CanvasView extends View {
    CanvasView(Context context) {
        super(context);
    }

    protected void onDraw(Canvas canvas) {
        canvas.drawColor(0xff00ff00);
    }

    protected void onSizeChanged(int w, int h, int ow, int oh) {

    }


}
