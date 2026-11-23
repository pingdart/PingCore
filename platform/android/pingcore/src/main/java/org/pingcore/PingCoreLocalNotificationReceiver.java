package org.pingcore;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

/** Delivers application-declared, one-shot PingCore local-notification alarms. */
public final class PingCoreLocalNotificationReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context context, Intent intent) {
        PingCoreLocalNotification.deliverScheduled(context, intent);
    }
}
