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


    // 👉 No copy: package JNI libs straight from vendor AND (optionally) local jniLibs
    sourceSets.getByName("main") {
        jniLibs.srcDirs(
            "$rootDir/../vendor/FMOD/android/api/core/lib"
        )

        // 👉 No copy: package assets straight from repo root (and keep local assets if any)
        assets.srcDirs(
            "$rootDir/../assets"
        )
    }
}

dependencies {
    implementation(libs.appcompat)
    implementation(libs.material)
    implementation(libs.constraintlayout)
    testImplementation(libs.junit)
    androidTestImplementation(libs.ext.junit)
    androidTestImplementation(libs.espresso.core)
    implementation(files("$rootDir/../vendor/FMOD/android/api/core/lib/fmod.jar"))
}
