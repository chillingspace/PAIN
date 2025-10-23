plugins {
    alias(libs.plugins.android.application)
}

android {
    namespace = "com.game.pain"
    compileSdk = 36

    defaultConfig {
        applicationId = "com.game.pain"
        minSdk = 24
        targetSdk = 36
        versionCode = 1
        versionName = "1.0"

        ndk {
            abiFilters.addAll(listOf("armeabi-v7a", "arm64-v8a", "x86", "x86_64"))
        }
        externalNativeBuild {
            cmake {
                cppFlags.addAll(listOf("-std=c++17"))
                arguments.addAll(listOf("-DANDROID_STL=c++_shared"))
            }
        }
    }
    java { toolchain { languageVersion.set(JavaLanguageVersion.of(17)) } }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    buildTypes {
        debug { isJniDebuggable = true }
        release { isMinifyEnabled = false }
    }
    externalNativeBuild {
        cmake {
            path = file("../../CMakeLists.txt")
            version = "3.22.1"
        }
    }
    packaging {
        jniLibs.useLegacyPackaging = true
    }

    sourceSets.getByName("main") {
        jniLibs.srcDirs(
            "$rootDir/../vendor/FMOD/android/api/core/lib"
        )

        assets.srcDirs(
            "$rootDir/../assets"
        )
    }
}

// ============ testing!! copy baked assets into Android assets folder ============
val androidAssetsDir = file("$rootDir/../assets")   // where app reads assets

// Where the baker writes 
val bakeOutputDebug   = file("$rootDir/../../bin/Debug/Assets")
val bakeOutputRelease = file("$rootDir/../../bin/Release/Assets")

// One copy task that chooses the right source based on the requested task(s)
val copyBakedAssets = tasks.register<org.gradle.api.tasks.Copy>("copyBakedAssets") {
    // Pick a source dir: if any Release task is in the task graph, prefer Release output
    val wantsRelease = gradle.startParameter.taskNames.any { it.contains("Release", ignoreCase = true) }
    val fromDir = if (wantsRelease) bakeOutputRelease else bakeOutputDebug

    onlyIf { fromDir.exists() }
    from(fromDir)
    into(androidAssetsDir)
    include("**/*.json", "**/*.astc", "**/*.dds")   // add more patterns if needed
    duplicatesStrategy = org.gradle.api.file.DuplicatesStrategy.INCLUDE
}

// After all tasks are created, make every merge*Assets depend on our copy
gradle.projectsEvaluated {
    tasks.matching { it.name.startsWith("merge", ignoreCase = true) && it.name.endsWith("Assets", ignoreCase = true) }
        .configureEach { dependsOn(copyBakedAssets) }
}
// ==============================================================================

dependencies {
    implementation(libs.appcompat)
    implementation(libs.material)
    implementation(libs.constraintlayout)
    testImplementation(libs.junit)
    androidTestImplementation(libs.ext.junit)
    androidTestImplementation(libs.espresso.core)
    implementation(files("$rootDir/../vendor/FMOD/android/api/core/lib/fmod.jar"))
}