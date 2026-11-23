package org.pingcore;

import android.app.job.JobParameters;
import android.app.job.JobService;
import android.util.Log;

/** Receives scheduled background tasks, including launches without an Activity. */
public final class PingCoreBackgroundJobService extends JobService {
    @Override
    public boolean onStartJob(JobParameters parameters) {
        try {
            return PingCoreApplication.initialize(this).systemTasks().beginJob(this, parameters);
        } catch (RuntimeException | LinkageError exception) {
            Log.e("PingCore", "PingCore could not initialize the application for its background job", exception);
            return false;
        }
    }

    @Override
    public boolean onStopJob(JobParameters parameters) {
        PingCoreApplication application = PingCoreApplication.current();
        if (application != null) application.systemTasks().stopJob(this, parameters);
        return false;
    }
}
