package org.pingcore;

import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;

final class PingCoreApplicationActivation {
    static final int URL = 1;
    static final int FILE = 2;
    static final int NOTIFICATION = 3;

    final int kind;
    final String value;
    final String name;
    final long size;
    final String contentType;
    final boolean writable;
    final byte[] data;

    private PingCoreApplicationActivation(
            int kind, String value, String name, long size, String contentType, boolean writable, byte[] data) {
        this.kind = kind;
        this.value = value;
        this.name = name;
        this.size = size;
        this.contentType = contentType;
        this.writable = writable;
        this.data = data == null ? null : data.clone();
    }

    static PingCoreApplicationActivation fromIntent(Context context, Intent intent) {
        if (intent == null) {
            return null;
        }
        String action = intent.getAction();
        if (PingCoreLocalNotification.ACTIVATION_ACTION.equals(action)) {
            String identifier = intent.getStringExtra(PingCoreLocalNotification.IDENTIFIER_EXTRA);
            if (identifier == null || identifier.isEmpty()
                    || !PingCoreLocalNotification.isActivationIntent(context, intent, identifier)) {
                return null;
            }
            try {
                byte[] data = PingCoreLocalNotification.readData(intent);
                PingCoreLocalNotification.decodeData(data);
                return new PingCoreApplicationActivation(NOTIFICATION, identifier, null, -1L, null, false, data);
            } catch (RuntimeException exception) {
                return null;
            }
        }
        if (!Intent.ACTION_VIEW.equals(action) && !Intent.ACTION_EDIT.equals(action)) {
            return null;
        }
        Uri uri = intent.getData();
        if (uri == null) {
            return null;
        }
        String value = uri.toString();
        if (value.isEmpty()) {
            return null;
        }
        String scheme = uri.getScheme();
        if (!"content".equalsIgnoreCase(scheme) && !"file".equalsIgnoreCase(scheme)) {
            return new PingCoreApplicationActivation(URL, value, null, -1L, null, false, null);
        }

        Context applicationContext = context.getApplicationContext();
        ContentResolver resolver = applicationContext.getContentResolver();
        int grantFlags =
                intent.getFlags() & (Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        PingCoreFileReference.Metadata metadata = PingCoreFileReference.describe(resolver, uri, grantFlags);
        String contentType = metadata.contentType;
        if (contentType == null) {
            contentType = PingCoreFileReference.sanitizeContentType(intent.getType());
        }
        return new PingCoreApplicationActivation(
                FILE, value, metadata.name, metadata.size, contentType, metadata.writable, null);
    }
}
