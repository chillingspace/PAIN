import org.apache.tools.ant.taskdefs.condition.Os
import java.util.Properties
import java.io.FileInputStream

// Load keystore properties
val keystoreProperties = Properties()
val keystorePropertiesFile = rootProject.file("key.properties")
if (keystorePropertiesFile.exists()) {
    keystoreProperties.load(FileInputStream(keystorePropertiesFile))
}

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
        
        // DON'T set ABI filters here - KTX library can't compile for x86
        // Instead, we'll set them per build type below
        
        externalNativeBuild {
            cmake {
                cppFlags.addAll(listOf("-std=c++17"))
                arguments.addAll(listOf("-DANDROID_STL=c++_shared",
					"-DPAIN_ENABLE_LUA_SMOKETEST=ON"
		))
            }
        }
    }

    // Add signingConfigs here
    signingConfigs {
        create("release") {
            val storeFilePath = keystoreProperties["storeFile"] as String?
            if (!storeFilePath.isNullOrBlank()) {
                // Use rootProject.file() to resolve from android/ folder
                storeFile = rootProject.file(storeFilePath)
            }
            storePassword = keystoreProperties["storePassword"] as String?
            keyAlias = keystoreProperties["keyAlias"] as String?
            keyPassword = keystoreProperties["keyPassword"] as String?
        }
    }

    ndkVersion = "27.0.12077973"

    java { toolchain { languageVersion.set(JavaLanguageVersion.of(17)) } }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    
    buildTypes {
        debug { 
            isJniDebuggable = true
            
            // DUAL BUILD SUPPORT: Different app ID so debug and release can coexist
            applicationIdSuffix = ".debug"  // Makes it: com.game.pain.debug
            versionNameSuffix = "-DEBUG"    // Version shows as "1.0-DEBUG"
            
            // Different app name in launcher to distinguish them
            manifestPlaceholders["appName"] = "PAIN (Debug)"
            
            // For debug/emulator: Only ARM to avoid KTX x86 compilation error
            // Modern emulators can use ARM via translation
            ndk {
                abiFilters.addAll(listOf("arm64-v8a", "armeabi-v7a"))
            }
        }
        
        release { 
            isMinifyEnabled = true  // Enable code shrinking
            isShrinkResources = true  // Remove unused resources
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
            signingConfig = signingConfigs.getByName("release")
            
            // Production app name
            manifestPlaceholders["appName"] = "PAIN"
            
            // Optimize native builds
            externalNativeBuild {
                cmake {
                    cppFlags.addAll(listOf("-O3", "-DNDEBUG"))  // Maximum optimization
                    arguments.add("-DCMAKE_BUILD_TYPE=Release")
                }
            }
            
            // ARM only for release
            ndk {
                abiFilters.addAll(listOf("arm64-v8a", "armeabi-v7a"))
            }
        }
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