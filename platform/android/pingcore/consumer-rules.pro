-keep class org.pingcore.PingCoreApplication { *; }
-keep class org.pingcore.PingCoreApplication$* { *; }
-keep class org.pingcore.PingCoreSystemTasks { *; }
-keep class org.pingcore.PingCoreHttpRequest { *; }
-keep class org.pingcore.PingCoreFilePicker { *; }
-keep class org.pingcore.PingCoreFilePicker$* { *; }
-keep class org.pingcore.PingCoreFileReference { *; }
-keep class org.pingcore.PingCoreFileReference$* { *; }
-keep class org.pingcore.PingCorePlatformChannel { *; }
-keep class org.pingcore.PingCorePlatformChannel$* { *; }
-keep class org.pingcore.PingCoreExternalTexture { *; }
-keep class org.pingcore.PingCoreBufferReference { *; }
-keep class org.pingcore.PingCoreBufferReference$* { *; }
-keep class org.pingcore.PlatformPayload { *; }
-keep class org.pingcore.PlatformPayload$* { *; }
-keep class org.pingcore.PingCorePlatformModule { *; }
-keep class org.pingcore.PingCorePlatformModule$* { *; }
-keep class * implements org.pingcore.PingCorePlatformModule$Factory { public <init>(); *; }
-keep class * implements org.pingcore.PingCorePlatformView$Factory { public <init>(); *; }
-keep class org.pingcore.PingCoreActivityResultLauncher { *; }
-keep class org.pingcore.PingCoreActivityResultLauncher$* { *; }
-keep class * implements org.pingcore.PingCorePlatformModule$WindowFactory { public <init>(); *; }
-keepclassmembers class org.pingcore.PingCoreView {
    org.pingcore.PingCoreFilePicker filePicker();
    private void requestTextInputKeyboard(long, boolean);
    private java.lang.Object createTextLayout(byte[], float, float, int, byte[], int, int, int, int, int, int, byte[],
        byte[]);
    private void drawPreparedText(android.graphics.Canvas, java.lang.Object, float, float, float, float, float, float,
        int);
}
-keepclassmembers class org.pingcore.PingCoreTextLayout {
    float[] measure();
    double[] lineMetrics();
    long hitTest(float, float);
    float[] caret(long, boolean);
    float[] range(long, long);
    float[] inlineObjectPlacement(long);
    long previous(long);
    long next(long);
}
