import java.util.Properties

plugins {
    id("com.android.application")
}

// The game's root: the version, the packed assets and the frame's art.
val gameRoot: File = rootDir.resolve("../..").canonicalFile
val pcCmake: File = gameRoot.resolve("platform/pc/CMakeLists.txt")

// One version for every build: project(frostline_hold_pc VERSION x.y.z).
// Google Play needs a larger versionCode on every upload: raise the version.
val gameVersion: String = Regex("""project\(frostline_hold_pc VERSION ([0-9]+\.[0-9]+\.[0-9]+)""")
    .find(pcCmake.readText())?.groupValues?.get(1)
    ?: error("no version in $pcCmake")
val gameVersionCode: Int = gameVersion.split(".").map(String::toInt)
    .let { (major, minor, patch) -> major * 10000 + minor * 100 + patch }

// Release signing, from keystore.properties beside settings.gradle.kts
// (kept out of git). Without it the release build comes out unsigned.
val keystore = Properties().apply {
    rootDir.resolve("keystore.properties").takeIf { it.exists() }?.inputStream()?.use(::load)
}

android {
    namespace = "com.hkgood.frostlinehold"
    compileSdk = 36
    ndkVersion = "28.2.13676358"

    defaultConfig {
        applicationId = "com.hkgood.frostlinehold"
        minSdk = 26
        targetSdk = 36
        versionCode = gameVersionCode
        versionName = gameVersion
        ndk {
            // Phones (64 and 32 bit), and x86_64 for Chromebooks and the emulator.
            abiFilters += listOf("arm64-v8a", "armeabi-v7a", "x86_64")
        }
        externalNativeBuild {
            cmake {
                arguments += "-DANDROID_STL=c++_static"
            }
        }
    }

    signingConfigs {
        if (!keystore.isEmpty) {
            create("release") {
                storeFile = rootProject.file(keystore.getProperty("storeFile"))
                storePassword = keystore.getProperty("storePassword")
                keyAlias = keystore.getProperty("keyAlias")
                keyPassword = keystore.getProperty("keyPassword")
            }
        }
    }

    buildTypes {
        release {
            // SDL's Java side is called from C by name; shrinking would break it.
            isMinifyEnabled = false
            signingConfig = signingConfigs.findByName("release")
            // Symbol tables in the bundle, so Play Console can read native crashes.
            ndk { debugSymbolLevel = "SYMBOL_TABLE" }
        }
    }

    // The native library is the PC shell built for Android (FROSTLINE_FRAME on).
    externalNativeBuild {
        cmake {
            path = pcCmake
            version = "3.31.6"
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }

    sourceSets["main"].assets.srcDir(layout.buildDirectory.dir("generated/frostline_assets"))
}

// The files the game opens, as the PC builds stage them: assets/ holds the
// packed atlases and sounds, assets/frame the frame's art.
val stageAssets = tasks.register<Sync>("stageAssets") {
    into(layout.buildDirectory.dir("generated/frostline_assets"))
    into("assets") {
        from(gameRoot.resolve("assets/generated")) {
            include("*.atlas", "*.sound")
        }
        into("frame") {
            from(gameRoot.resolve("platform/pc/frame")) {
                include("*.png")
            }
        }
    }
}
tasks.named("preBuild") {
    dependsOn(stageAssets)
}
