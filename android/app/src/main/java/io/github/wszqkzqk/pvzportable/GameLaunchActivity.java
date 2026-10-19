package io.github.wszqkzqk.pvzportable;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;
import java.io.File;
import java.io.IOException;

/** @pvz-online: first launch is automatic; keep large resource copies off the UI thread. */
public class GameLaunchActivity extends Activity {
    private TextView status;
    private ProgressBar progress;
    private Button retry;

    @Override protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        File dir = getExternalFilesDir(null);
        if (BundledGameResources.hasResources(dir)) { launchGame(); return; }
        if (!BundledGameResources.isBundled(path -> getAssets().open(path))) {
            startActivity(new Intent(this, ResourceImportActivity.class));
            finish();
            return;
        }
        LinearLayout panel = new LinearLayout(this);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setGravity(Gravity.CENTER);
        int padding = (int)(24 * getResources().getDisplayMetrics().density);
        panel.setPadding(padding, padding, padding, padding);
        status = new TextView(this);
        status.setGravity(Gravity.CENTER);
        status.setText(R.string.bundled_preparing);
        status.setTextSize(18);
        panel.addView(status);
        progress = new ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal);
        panel.addView(progress, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, padding));
        retry = new Button(this);
        retry.setText(R.string.bundled_retry);
        retry.setVisibility(View.GONE);
        retry.setOnClickListener(v -> prepare(dir));
        panel.addView(retry);
        setContentView(panel);
        prepare(dir);
    }

    private void prepare(File dir) {
        retry.setVisibility(View.GONE);
        status.setText(R.string.bundled_preparing);
        new Thread(() -> {
            try {
                BundledGameResources.install(dir, path -> getAssets().open(path), (completed, total) ->
                    runOnUiThread(() -> { progress.setMax(total); progress.setProgress(completed); }));
                Log.i("PvZBundled", "Bundled resources verified; starting game");
                runOnUiThread(() -> { if (!isFinishing() && !isDestroyed()) launchGame(); });
            } catch (IOException e) {
                Log.e("PvZBundled", "Resource preparation failed", e);
                runOnUiThread(() -> {
                    if (isFinishing() || isDestroyed()) return;
                    status.setText(R.string.bundled_failed);
                    retry.setVisibility(View.VISIBLE);
                });
            }
        }, "PvZ-resource-setup").start();
    }

    private void launchGame() {
        startActivity(new Intent(this, PvZPortableActivity.class));
        finish();
    }
}
