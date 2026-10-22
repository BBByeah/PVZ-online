package io.github.wszqkzqk.pvzportable;

import android.app.*;
import android.content.*;
import android.net.wifi.WifiManager;
import android.os.*;

/** Runs the common six-seat relay separately from SDL's paused Activity. */
public final class LanHostService extends Service {
    static final int QUERY = 1;
    static final String STOP = "pvz.lan.STOP";
    private static final String CHANNEL = "lan_host";
    private boolean loaded;
    private int port;
    private String error = "";
    private PowerManager.WakeLock cpu;
    private WifiManager.WifiLock wifi;
    private void releaseLocks() {
        if (wifi != null && wifi.isHeld()) wifi.release();
        if (cpu != null && cpu.isHeld()) cpu.release();
        wifi = null; cpu = null;
    }
    private void endHosting() {
        if (loaded) nativeStop();
        port = 0;
        releaseLocks();
    }
    private static native String nativeStart(int port);
    private static native void nativeStop();
    private static native int nativePort();
    private final Messenger messenger = new Messenger(new Handler(Looper.getMainLooper(), message -> {
        if (message.what == QUERY && message.replyTo != null) {
            Message response = Message.obtain(null, QUERY, port, 0);
            Bundle data = new Bundle(); data.putString("error", error); response.setData(data);
            try { message.replyTo.send(response); } catch (RemoteException ignored) { }
        }
        return true;
    }));

    @Override public void onCreate() {
        super.onCreate();
        if (Build.VERSION.SDK_INT >= 26) getSystemService(NotificationManager.class).createNotificationChannel(
            new NotificationChannel(CHANNEL, getString(R.string.lan_host_title), NotificationManager.IMPORTANCE_LOW));
        startForeground(27777, notification(getString(R.string.lan_host_starting)));
    }
    private Notification notification(String text) {
        PendingIntent open = PendingIntent.getActivity(this, 0,
            new Intent(this, PvZPortableActivity.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK), PendingIntent.FLAG_IMMUTABLE | PendingIntent.FLAG_UPDATE_CURRENT);
        PendingIntent stop = PendingIntent.getService(this, 1, new Intent(this, LanHostService.class).setAction(STOP), PendingIntent.FLAG_IMMUTABLE | PendingIntent.FLAG_UPDATE_CURRENT);
        Notification.Builder builder = Build.VERSION.SDK_INT >= 26 ? new Notification.Builder(this, CHANNEL) : new Notification.Builder(this);
        return builder.setSmallIcon(android.R.drawable.stat_notify_sync).setContentTitle(getString(R.string.lan_host_title))
            .setContentText(text).setContentIntent(open).setOngoing(true)
            .addAction(new Notification.Action.Builder(null, getString(R.string.lan_host_end), stop).build()).build();
    }
    @Override public int onStartCommand(Intent intent, int flags, int startId) {
        if (intent == null || STOP.equals(intent.getAction())) {
            endHosting(); error = "Hosting ended. Create a new room to play again.";
            stopForeground(true); stopSelf(); return START_NOT_STICKY;
        }
        int wanted = intent.getIntExtra("port", 27777);
        try {
            if (!loaded) { System.loadLibrary("pvzrelay"); loaded = true; }
            String problem = nativeStart(wanted);
            if (problem != null) throw new IllegalStateException(problem);
            port = nativePort(); error = "";
            if (cpu == null) {
                cpu = ((PowerManager)getSystemService(POWER_SERVICE)).newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "pvz:lanhost");
                cpu.setReferenceCounted(false); cpu.acquire();
                WifiManager manager = (WifiManager)getApplicationContext().getSystemService(WIFI_SERVICE);
                if (manager != null) { wifi = manager.createWifiLock(WifiManager.WIFI_MODE_FULL_HIGH_PERF, "pvz:lanhost"); wifi.setReferenceCounted(false); wifi.acquire(); }
            }
            getSystemService(NotificationManager.class).notify(27777, notification(getString(R.string.lan_host_running, port)));
        } catch (RuntimeException | UnsatisfiedLinkError problem) {
            endHosting(); error = problem.getMessage();
            // Keep the bound error available until the UI receives it, then the
            // controller stops this service; never return a fake successful port.
            getSystemService(NotificationManager.class).notify(27777, notification(getString(R.string.lan_host_failed)));
        }
        return START_NOT_STICKY; // a killed process cannot resurrect old rooms
    }
    @Override public IBinder onBind(Intent intent) { return messenger.getBinder(); }
    @Override public void onTaskRemoved(Intent intent) {
        endHosting(); error = "Hosting ended. Create a new room to play again.";
        stopForeground(true); stopSelf();
    }
    @Override public void onDestroy() {
        endHosting();
        stopForeground(true); super.onDestroy();
    }
}
