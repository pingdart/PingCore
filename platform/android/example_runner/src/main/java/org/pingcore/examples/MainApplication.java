package org.pingcore.examples;

import android.app.Application;

public final class MainApplication extends Application {
    static {
        System.loadLibrary(BuildConfig.PINGCORE_APP_LIBRARY);
    }
}
