package org.pingcore.examples.platformmodule;

import android.content.Context;

import java.util.Objects;
import java.util.concurrent.Executors;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

import org.pingcore.PingCorePlatformChannel;
import org.pingcore.PingCorePlatformModule;
import org.pingcore.PlatformPayload;

public final class PlatformTimer implements PingCorePlatformModule.Factory {
    public PlatformTimer() {}

    @Override
    public PingCorePlatformModule create(
            Context context, PlatformPayload options, PingCorePlatformChannel.Events events) {
        options.requireNull();
        return new Instance(context, events);
    }

    private static final class Instance implements PingCorePlatformModule {
        private final ScheduledExecutorService executor;
        private final PingCorePlatformChannel.Events events;
        private long generation;
        private long tick;
        private ScheduledFuture<?> task;
        private PingCorePlatformChannel.Result pendingStart;
        private boolean disposed;

        Instance(Context context, PingCorePlatformChannel.Events events) {
            String threadName = "PingCore-" + Objects.requireNonNull(context).getPackageName() + "-Timer";
            executor = Executors.newSingleThreadScheduledExecutor(runnable -> {
                Thread thread = new Thread(runnable, threadName);
                thread.setDaemon(true);
                return thread;
            });
            this.events = events;
        }

        @Override
        public PingCorePlatformChannel.Cancellation invoke(
                String method, PlatformPayload arguments, PingCorePlatformChannel.Result result) {
            switch (method) {
                case "start":
                    return start(arguments.requireInt64(), result);
                case "stop":
                    arguments.requireNull();
                    stop(result);
                    return null;
                default:
                    result.fail("pingcore/unsupported-method", "PingCore example timer does not support " + method,
                            PlatformPayload.nullValue());
                    return null;
            }
        }

        @Override
        public void dispose() {
            synchronized (this) {
                if (disposed) {
                    return;
                }
                disposed = true;
                generation++;
                pendingStart = null;
                stopLocked();
            }
            executor.shutdownNow();
        }

        private PingCorePlatformChannel.Cancellation start(
                long intervalMilliseconds, PingCorePlatformChannel.Result result) {
            if (intervalMilliseconds <= 0L) {
                result.fail("example/timer-interval", "The Android timer interval must be positive",
                        PlatformPayload.nullValue());
                return null;
            }

            PingCorePlatformChannel.Result replaced;
            long nextGeneration;
            boolean started = true;
            synchronized (this) {
                if (disposed) {
                    result.fail("example/timer-closed", "The Android timer is closed", PlatformPayload.nullValue());
                    return null;
                }
                replaced = pendingStart;
                stopLocked();
                nextGeneration = ++generation;
                tick = 0L;
                pendingStart = result;
                try {
                    task = executor.scheduleAtFixedRate(()
                                                                -> tick(nextGeneration),
                            intervalMilliseconds, intervalMilliseconds, TimeUnit.MILLISECONDS);
                } catch (RejectedExecutionException exception) {
                    pendingStart = null;
                    started = false;
                }
            }
            if (replaced != null) {
                replaced.fail("example/timer-replaced", "The timer was replaced by a newer start call",
                        PlatformPayload.nullValue());
            }
            if (!started) {
                result.fail(
                        "example/timer-java", "The Android timer could not be started", PlatformPayload.nullValue());
                return null;
            }
            return () -> cancelStart(nextGeneration);
        }

        private void stop(PingCorePlatformChannel.Result result) {
            PingCorePlatformChannel.Result pending;
            synchronized (this) {
                if (disposed) {
                    result.fail("example/timer-closed", "The Android timer is closed", PlatformPayload.nullValue());
                    return;
                }
                generation++;
                pending = pendingStart;
                pendingStart = null;
                stopLocked();
            }
            if (pending != null) {
                pending.fail("example/timer-stopped", "The timer stopped before its first tick",
                        PlatformPayload.nullValue());
            }
            result.complete(PlatformPayload.nullValue());
        }

        private synchronized void cancelStart(long cancelledGeneration) {
            if (!disposed && generation == cancelledGeneration) {
                generation++;
                pendingStart = null;
                stopLocked();
            }
        }

        private void tick(long scheduledGeneration) {
            PingCorePlatformChannel.Result firstResult;
            long nextTick;
            synchronized (this) {
                if (disposed || generation != scheduledGeneration || task == null) {
                    return;
                }
                nextTick = ++tick;
                firstResult = pendingStart;
                pendingStart = null;
            }
            if (firstResult != null) {
                firstResult.complete(PlatformPayload.int64(nextTick));
            }
            events.emit("tick", PlatformPayload.int64(nextTick));
        }

        private void stopLocked() {
            if (task != null) {
                task.cancel(false);
                task = null;
            }
        }
    }
}
