import org.gradle.api.tasks.Copy

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

        ndk { abiFilters += listOf("arm64-v8a", "x86_64") }
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
    sourceSets["main"].jniLibs.srcDirs("src/main/jniLibs")
    //sourceSets["main"].assets.srcDirs("../../../assets")
}

dependencies {

    implementation(libs.appcompat)
    implementation(libs.material)
    implementation(libs.constraintlayout)
    testImplementation(libs.junit)
    androidTestImplementation(libs.ext.junit)
    androidTestImplementation(libs.espresso.core)
}

// Copies vendor .so's into app/src/main/jniLibs/<ABI>/ on each build
val syncVendorJniLibs by tasks.register<Copy>("syncVendorJniLibs") {
    // vendor dir that contains ABI subfolders
    from("$rootDir/vendor/fmod/android/core/lib") {
        include("arm64-v8a/*.so")
        include("x86_64/*.so")
    }
    into("$projectDir/src/main/jniLibs")
}

// Make packaging depend on our sync step (works across AGP tasks)
tasks.matching { it.name.contains("merge") && it.name.contains("JniLibFolders") }
    .configureEach { dependsOn(syncVendorJniLibs) }