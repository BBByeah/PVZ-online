package io.github.wszqkzqk.pvzportable;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.List;

/** @pvz-online: a complete player APK prepares its own resources without touching saves. */
final class BundledGameResources {
    interface Assets { InputStream open(String path) throws IOException; }
    interface Progress { void update(int completed, int total); }
    private static final String PREFIX = "bundled-game/";
    private static final String MARKER = ".pvz-bundled-install";

    static boolean hasResources(File dir) {
        return dir != null && !new File(dir, MARKER).exists()
            && new File(dir, "main.pak").isFile() && new File(dir, "main.pak").length() > 0
            && new File(dir, "properties").isDirectory();
    }

    static boolean isBundled(Assets assets) {
        try (InputStream ignored = assets.open(PREFIX + "manifest.tsv")) { return true; }
        catch (IOException e) { return false; }
    }

    static void install(File dir, Assets assets, Progress progress) throws IOException {
        // @pvz-online: existing imports and saves must survive an APK update unchanged.
        if (hasResources(dir)) return;
        if (dir == null || (!dir.isDirectory() && !dir.mkdirs()))
            throw new IOException("Resource directory unavailable");
        List<String[]> entries = new ArrayList<>();
        boolean hasMain = false, hasProperties = false;
        try (BufferedReader reader = new BufferedReader(new InputStreamReader(
                assets.open(PREFIX + "manifest.tsv"), "UTF-8"))) {
            String line;
            while ((line = reader.readLine()) != null) {
                String[] fields = line.split("\t", -1);
                if (fields.length != 3 || !fields[0].matches("[0-9a-f]{64}")
                    || !fields[2].matches("[0-9]+")) throw new IOException("Invalid resource manifest");
                String name = fields[1];
                if (name.contains("..") || name.contains("\\") || name.startsWith("/")
                    || !(name.equals("main.pak") || name.equals("pvz_zh.pak") || name.startsWith("properties/")))
                    throw new IOException("Invalid resource path");
                hasMain |= name.equals("main.pak");
                hasProperties |= name.startsWith("properties/");
                entries.add(fields);
            }
        }
        if (!hasMain || !hasProperties) throw new IOException("Incomplete resource manifest");
        // @pvz-online: an interrupted copy must never look like a completed installation.
        File marker = new File(dir, MARKER);
        if (!marker.exists() && !marker.createNewFile()) throw new IOException("Cannot mark preparation");
        int completed = 0;
        progress.update(0, entries.size());
        for (String[] entry : entries) {
            File destination = new File(dir, entry[1]);
            File parent = destination.getParentFile();
            if (!parent.isDirectory() && !parent.mkdirs()) throw new IOException("Cannot create resource directory");
            File temporary = new File(parent, destination.getName() + ".pvz-part");
            MessageDigest sha;
            try { sha = MessageDigest.getInstance("SHA-256"); }
            catch (NoSuchAlgorithmException e) { throw new IOException(e); }
            long size = 0;
            try (InputStream input = assets.open(PREFIX + entry[1]);
                 FileOutputStream output = new FileOutputStream(temporary)) {
                byte[] buffer = new byte[128 * 1024];
                int count;
                while ((count = input.read(buffer)) != -1) {
                    output.write(buffer, 0, count);
                    sha.update(buffer, 0, count);
                    size += count;
                }
                output.getFD().sync();
            }
            StringBuilder hex = new StringBuilder();
            for (byte b : sha.digest()) hex.append(String.format(java.util.Locale.ROOT, "%02x", b & 255));
            if (!hex.toString().equals(entry[0]) || !Long.toString(size).equals(entry[2])) {
                temporary.delete();
                throw new IOException("Resource verification failed: " + entry[1]);
            }
            // @pvz-online: retry must replace files from an interrupted preparation on every filesystem.
            if (destination.exists() && !destination.delete()) throw new IOException("Cannot replace resource: " + entry[1]);
            if (!temporary.renameTo(destination)) throw new IOException("Cannot finish resource: " + entry[1]);
            progress.update(++completed, entries.size());
        }
        if (!marker.delete()) throw new IOException("Cannot finish resource preparation");
        if (!hasResources(dir)) throw new IOException("Prepared resources unavailable");
    }
}
