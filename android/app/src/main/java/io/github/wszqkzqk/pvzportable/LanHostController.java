package io.github.wszqkzqk.pvzportable;

import android.content.*;
import android.os.*;

/** Activity-side state is a cache of explicit replies from the service process. */
final class LanHostController {
    private static volatile int state;
    private static volatile String error = "";
    private static Context context;
    private static Messenger remote;
    private static boolean bound;
    private static long lastQuery;
    private static final Messenger replies = new Messenger(new Handler(Looper.getMainLooper(), message -> {
        if (message.what == LanHostService.QUERY) {
            state = message.arg1;
            error = message.getData().getString("error", "");
            if (state == 0 && !error.isEmpty()) release(false);
        }
        return true;
    }));
    private static final ServiceConnection connection = new ServiceConnection() {
        @Override public void onServiceConnected(ComponentName name, IBinder binder) { remote = new Messenger(binder); query(); }
        @Override public void onServiceDisconnected(ComponentName name) {
            remote = null; state = 0; error = "Hosting stopped. Create a new room.";
        }
        @Override public void onBindingDied(ComponentName name) { onServiceDisconnected(name); release(false); }
    };
    static void start(Context owner, int port) {
        context = owner.getApplicationContext(); error = ""; state = -1;
        try {
            Intent intent = new Intent(context, LanHostService.class).putExtra("port", port);
            if (Build.VERSION.SDK_INT >= 26) context.startForegroundService(intent); else context.startService(intent);
            if (!bound) bound = context.bindService(intent, connection, Context.BIND_AUTO_CREATE);
            else query();
            if (!bound) throw new IllegalStateException("Cannot connect to the local host service.");
        } catch (RuntimeException problem) { error = problem.getMessage(); state = 0; release(false); }
    }
    private static void query() {
        if (remote == null) return;
        Message message = Message.obtain(null, LanHostService.QUERY); message.replyTo = replies;
        try { remote.send(message); } catch (RemoteException problem) { state = 0; error = "Hosting stopped. Create a new room."; }
    }
    static int poll() {
        long now = SystemClock.elapsedRealtime();
        if (now - lastQuery > 250) { lastQuery = now; new Handler(Looper.getMainLooper()).post(LanHostController::query); }
        return state;
    }
    static String error() { return error; }
    private static void release(boolean clearError) {
        if (context != null) {
            context.stopService(new Intent(context, LanHostService.class));
            if (bound) { try { context.unbindService(connection); } catch (IllegalArgumentException ignored) { } }
        }
        bound = false; remote = null; state = 0;
        if (clearError) error = "";
    }
    static void stop() { release(true); }
}
