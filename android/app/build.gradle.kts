import org.apache.tools.ant.taskdefs.condition.Os

tasks.register<Exec>("runAssetCompiler") {
    val assetCompilerExe = File(rootDir.parentFile, "build/Tools/AssetCompilerTool.exe").absolutePath
    val assetInputDir = File(rootDir.parentFile, "assets").absolutePath
    val assetOutputDir = File(rootDir, "app/src/main/assets").absolutePath

    onlyIf {
        Os.isFamily(Os.FAMILY_WINDOWS) && File(assetCompilerExe).exists()
    }
    commandLine(assetCompilerExe,
        "--input", assetInputDir,
        "--output", assetOutputDir,
        "--target", "android")
}

tasks.named("preBuild").configure {
    dependsOn("runAssetCompiler")
}

tasks.register<Exec>("buildAssetCompilerTool") {
    val buildassetcompilerBat = File(rootDir.parentFile, "build-assetcompiler.bat").absolutePath
    commandLine("cmd", "/c", buildassetcompilerBat)
}
tasks.named("runAssetCompiler").configure { dependsOn("buildAssetCompilerTool") }

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
        val abiEnv = System.getenv("CI_ABI")
        ndk {
            abiFilters.addAll(if (abiEnv != null) listOf(abiEnv) else 
            listOf("armeabi-v7a", "arm64-v8a", "x86", "x86_64"))
        }
        externalNativeBuild {
            cmake {
                cppFlags.addAll(listOf("-std=c++17"))
                arguments.addAll(listOf("-DANDROID_STL=c++_shared"))
            }
        }
    }

    ndkVersion = "27.0.12077973"

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