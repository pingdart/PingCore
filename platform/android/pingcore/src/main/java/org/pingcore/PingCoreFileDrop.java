package org.pingcore;

import android.content.ClipData;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;

import java.util.ArrayList;
import java.util.concurrent.Future;
import java.util.concurrent.atomic.AtomicBoolean;

final class PingCoreFileDrop {
    private static final int ERROR_PERMISSION_DENIED = 1;
    private static final int ERROR_IO = 3;

    static final class Access implements AutoCloseable {
        private final AutoCloseable permission;
        private final AtomicBoolean closed = new AtomicBoolean();

        Access(AutoCloseable permission) {
            this.permission = permission;
        }

        @Override
        public void close() {
            if (closed.compareAndSet(false, true)) {
                try {
                    permission.close();
                } catch (Exception ignored) {
                }
            }
        }
    }

    static final class Operation {
        final Context context;
        final Access access;
        private final ArrayList<Uri> uris = new ArrayList<>();
        private final AtomicBoolean finished = new AtomicBoolean();
        private volatile Future<?> worker;
        private volatile long nativeHandle;

        Operation(Context context, ClipData data, Access access) {
            this.context = context.getApplicationContext();
            this.access = access;
            if (data == null || data.getItemCount() == 0) {
                throw new IllegalArgumentException("PingCore file drop has no files");
            }
            for (int index = 0; index < data.getItemCount(); ++index) {
                Uri uri = data.getItemAt(index).getUri();
                if (uri == null || !"content".equalsIgnoreCase(uri.getScheme())) {
                    throw new IllegalArgumentException("PingCore Android file drop requires content URIs");
                }
                uris.add(uri);
            }
        }

        void start() {
            try {
                worker = PingCoreFileReference.submit(() -> {
                    PingCoreFileReference.Metadata[] values = new PingCoreFileReference.Metadata[uris.size()];
                    try {
                        for (int index = 0; index < uris.size(); ++index) {
                            if (finished.get()) {
                                return;
                            }
                            values[index] = PingCoreFileReference.describe(
                                    context.getContentResolver(), uris.get(index),
                                    Intent.FLAG_GRANT_READ_URI_PERMISSION);
                        }
                        finish(values, ERROR_IO);
                    } catch (SecurityException exception) {
                        finish(null, ERROR_PERMISSION_DENIED);
                    } catch (RuntimeException exception) {
                        finish(null, ERROR_IO);
                    }
                });
                if (finished.get()) {
                    worker.cancel(true);
                }
            } catch (RuntimeException exception) {
                finish(null, ERROR_IO);
            }
        }

        void cancel() {
            finish(null, ERROR_IO);
            Future<?> current = worker;
            if (current != null) {
                current.cancel(true);
            }
        }

        private void finish(PingCoreFileReference.Metadata[] values, int errorCode) {
            if (nativeHandle != 0L && finished.compareAndSet(false, true)) {
                nativeComplete(nativeHandle, values, errorCode);
            }
        }
    }

    private static native void nativeComplete(
            long nativeHandle, PingCoreFileReference.Metadata[] references, int errorCode);
}
