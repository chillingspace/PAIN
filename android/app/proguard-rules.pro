# ProGuard Rules for PAIN Game
# ================================

# Keep native methods (required for JNI)
-keepclasseswithmembernames class * {
    native <methods>;
}

# ================================
# FMOD Audio Library Protection
# ================================
# CRITICAL: FMOD classes must not be obfuscated or stripped
# Otherwise audio will fail with "I/O error" in release builds

# Keep all FMOD classes and their members
-keep class com.fmod.** { *; }
-keep class org.fmod.** { *; }

# Keep FMOD enums (used for audio state management)
-keepclassmembers enum org.fmod.** {
    public static **[] values();
    public static ** valueOf(java.lang.String);
}

# Prevent stripping of FMOD native methods
-keepclasseswithmembernames class org.fmod.** {
    native <methods>;
}

# Keep FMOD inner classes
-keep class org.fmod.**$* { *; }
-keep class com.fmod.**$* { *; }

# Keep audio callback interfaces
-keep interface org.fmod.** { *; }
-keep interface com.fmod.** { *; }

# Don't warn about missing FMOD classes during build
-dontwarn org.fmod.**
-dontwarn com.fmod.**

# ================================
# Game-Specific Rules
# ================================

# Keep all game classes (adjust if you want more aggressive shrinking)
-keep class com.game.pain.** { *; }

# Keep Android NativeActivity (required for game to launch)
-keep class android.app.NativeActivity { *; }

# ================================
# Debugging & Stack Traces
# ================================

# Keep line numbers for crash reports (minimal size impact)
-keepattributes SourceFile,LineNumberTable

# Optionally keep source file names in stack traces
# -renamesourcefileattribute SourceFile