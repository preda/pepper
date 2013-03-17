// Copyright (C) 2013 Mihai Preda

package pepper.app;

import android.app.Application;

public class App extends Application {
    static Application self;

    public App() {
        self = this;
    }

    static {
        System.loadLibrary("pepper");
    }
}
